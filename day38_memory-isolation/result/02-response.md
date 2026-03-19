哈哈，Rick！我們遭遇了作業系統開發史上最經典、最令人毛骨悚然的「奪舍事件」！👻

你提供的 Core Dump 簡直就是一份完美的法醫驗屍報告。請跟著我一起看這幾行關鍵記錄：

```text
  89: v=80 ... EAX=00000002  <-- 老爸 (Shell) 從 wait 醒來，立刻呼叫 sys_print (2) 準備印出 "Command not found" 或其他字串
  90: v=80 ... EAX=00000002 EBX=f000ff53 <-- 等等！老爸傳給 sys_print 的字串指標竟然是 f000ff53！
  91: v=0e ... CR2=f000ff53  <-- Kernel 試圖印出這個位址的字串，當場 Page Fault (0x0E) 暴斃！
```

這跟我們上一回合遇到的「老爸被奪舍」症狀**一模一樣**！

為什麼會這樣？我們明明已經把 `cat.elf` 和 `echo.elf` 編譯到 `0x080C0000` 和 `0x08080000` 了，不是嗎？
而且我們才剛剛寫好了 `create_page_directory`，為每個任務建立了「專屬的宇宙 (CR3)」啊！

**問題就出在這裡：我們建立的專屬宇宙，其實根本沒有起作用！**

### 🔍 案發現場解密：名存實亡的多元宇宙

請看你在 `lib/task.c` 的 `sys_fork` 裡是怎麼寫的：
```c
int sys_fork(registers_t *regs) {
    task_t *child = (task_t*) kmalloc(sizeof(task_t));
    // ...
    child->page_directory = current_task->page_directory; // [Day38][Add] (sys_fork 裡是 child->page_directory)
    // ...
```
在你的 `sys_fork` 裡，子行程依然直接繼承了老爸的 `page_directory`！這代表它們依然活在同一個虛擬宇宙裡！

當子行程呼叫 `sys_exec` 載入 `echo.elf` (或 `cat.elf`) 時，它依然無情地覆寫了這個共用宇宙裡對應的記憶體。這一次雖然 `echo` 編譯在 `0x08080000`，看似沒有覆蓋到 `0x08048000` 的 Shell 程式碼，但不要忘記：**BSS 段（未初始化的全域變數）、Heap 或某些字串常數，可能會被 ELF Loader 錯誤地覆寫或清空！**

更重要的是，我們寫好的 `create_page_directory()` 根本沒人去呼叫它！

### 🗡️ 終極修復：真正的開天闢地

這一次，我們不走半套。我們要讓 `sys_exec` 在載入 ELF 檔案的瞬間，**拋棄老爸的宇宙，為自己建立一個全新的宇宙！**

請打開 **`lib/task.c`**，對 `sys_exec` 進行真正的「大破大立」：

```c
int sys_exec(registers_t *regs) {
    char* filename = (char*)regs->ebx;
    char** argv = (char**)regs->ecx;

    fs_node_t* file_node = simplefs_find(filename);
    if (file_node == 0) { return -1; }

    uint8_t* buffer = (uint8_t*) kmalloc(file_node->length);
    vfs_read(file_node, 0, file_node->length, buffer);

    // =====================================================================
    // 【神聖分離】在載入程式碼之前，為這個 Process 建立專屬的新宇宙！
    // =====================================================================
    
    // 1. 建立新的 Page Directory
    uint32_t new_cr3 = create_page_directory();
    
    // 2. [黑魔法] 切換到新宇宙，這樣 elf_load 才會把程式碼寫進新宇宙的記憶體裡！
    uint32_t old_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(new_cr3));

    // =====================================================================

    uint32_t entry_point = elf_load((elf32_ehdr_t*)buffer);
    kfree(buffer);

    if (entry_point == 0) {
        // 如果載入失敗（例如不是合法的 ELF），必須退回老爸的宇宙，當作沒事發生
        __asm__ volatile("mov %0, %%cr3" : : "r"(old_cr3));
        return -1;
    }

    // 成功載入！正式把這張地契登記在這個任務名下
    current_task->page_directory = new_cr3;

    // 分配一個全新的實體分頁給 User Stack (在新的宇宙裡)
    // 雖然位址一樣是 0x083FF000，但背後的實體記憶體已經跟老爸完全不同了！
    uint32_t clean_user_stack_top = 0x083FF000 + 4096; // 這裡我們讓每個程式的 stack 都在同一個虛擬位址
    uint32_t ustack_phys = pmm_alloc_page();
    map_page(0x083FF000, ustack_phys, 7);

    // ------------------------------------------
    // 接下來的參數建構 (argc, argv) 邏輯完全不變
    // 只是現在 stack_ptr 是從統一的 0x083FF000 - 64 開始
    // ------------------------------------------
    uint32_t stack_ptr = clean_user_stack_top - 64; 

    int argc = 0;
    // 這裡讀取 argv 是安全的，因為它位於核心的 Heap 區，新舊宇宙都看得到！
    if (argv != 0 && (uint32_t)argv > 0x08000000) {
        while (argv[argc] != 0) argc++;
    }

    uint32_t argv_ptrs[16] = {0};

    if (argc > 0) {
        for (int i = argc - 1; i >= 0; i--) {
            int len = strlen(argv[i]) + 1;
            stack_ptr -= len;
            memcpy((void*)stack_ptr, argv[i], len);
            argv_ptrs[i] = stack_ptr;
        }

        stack_ptr = stack_ptr & ~3;

        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = 0;
        for (int i = argc - 1; i >= 0; i--) {
            stack_ptr -= 4;
            *(uint32_t*)stack_ptr = argv_ptrs[i];
        }
        uint32_t argv_base = stack_ptr;

        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = argv_base;
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = argc;
    } else {
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = 0;
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = 0;
    }

    stack_ptr -= 4;
    *(uint32_t*)stack_ptr = 0;

    regs->eip = entry_point;
    regs->user_esp = stack_ptr;

    return 0;
}
```

### 🎈 解除 Makefile 的枷鎖

既然每個程式都擁有了自己的宇宙，它們再也不會互相干擾了。
請打開你的 **`Makefile`**，把所有的程式都放回 `0x08048000` 這個光榮的起點！

```makefile
my_app.elf: crt0.o app.o
	@echo "==> 連結 Shell..."
	$(DOCKER_CMD) ld -m elf_i386 -Ttext 0x08048000 crt0.o app.o -o my_app.elf

echo.elf: crt0.o echo.o
	@echo "==> 連結 Echo 程式..."
	$(DOCKER_CMD) ld -m elf_i386 -Ttext 0x08048000 crt0.o echo.o -o echo.elf

cat.elf: crt0.o cat.o
	@echo "==> 連結 Cat 程式..."
	$(DOCKER_CMD) ld -m elf_i386 -Ttext 0x08048000 crt0.o cat.o -o cat.elf
```

存檔，執行 `make clean && make run`。

如果這一次順利執行，你就完成了作業系統開發中最艱難、最華麗的魔術：**讓無數個應用程式，在相同的虛擬位址上平行運作！** 試試看瘋狂輸入 `echo` 和 `cat`，感受一下沒有奪舍的世界吧！🚀
