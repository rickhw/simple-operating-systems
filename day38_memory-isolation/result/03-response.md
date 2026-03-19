哈哈，Rick，你果然是個敏銳的 OS 開發者！這完全是預期中的結果。

你的系統在呼叫內建指令 (`help`, `about`) 時安然無恙，但在呼叫外部指令 (`echo`, `cat`) 時卻崩潰了，為什麼？

因為我們目前的狀態處於 **Day 37 與 Day 38 的「疊加態」**！
回想一下我們剛才做的事：
1. 我們教排程器 (`switch_task`) 怎麼切換宇宙 (`CR3`)。
2. 我們寫好了創造宇宙的函式 (`create_page_directory`)。
3. **但是，我們還沒把創造宇宙的邏輯放進 `sys_exec` 裡面！**

這代表什麼？
這代表當你在 Shell 裡輸入 `echo hello` 時，Shell 呼叫了 `sys_fork` 產生分身，接著分身呼叫 `sys_exec`。在 `sys_exec` 裡，它依然無情地覆蓋了當下宇宙（原始宇宙）的記憶體（因為我們昨天把 Makefile 裡的 offset 改回了 `0x08048000`）。
然後，當分身執行完畢死掉，老爸 Shell 醒來時... 老爸的記憶體又被覆蓋成了 `echo` 的形狀，於是發生了跟之前一模一樣的「被奪舍而引發 Page Fault」的慘劇！

這也就是為什麼我剛才會說：
> 只要這個基礎不崩潰，我們明天就能放心把 `create_page_directory()` 注入到 `sys_exec` 當中...

現在，你的基礎建設已經確認穩定運作了，是時候完成最後一擊，徹底解放 Makefile，讓真正的 MMU 記憶體隔離大發神威了！

### 🗡️ Day 38 完結篇：在 `sys_exec` 中創造宇宙

請打開 **`lib/task.c`**，覆蓋 `sys_exec` 函式。這段程式碼將會在載入 ELF 之前，為新程式創造一個純淨的平行宇宙：

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

還有，別忘了在 `switch_task.S` 中，你少抓了 `edx` 暫存器！你必須在修改 `esp` 前，把 `esp+48` 放進 `edx` 裡保管。
修改 `asm/switch_task.S` 的開頭：

```assembly
switch_task:
    pusha
    pushf

    ; =========================================================
    ; 【關鍵修復：拯救 CR3】
    ; 必須在修改 ESP「之前」，先從舊堆疊拿出第三個參數 next_cr3
    ; 我們把它暫存在 EDX 裡 (稍後 popa 會從新堆疊覆蓋掉 EDX，所以借放一下非常安全)
    ; =========================================================
    mov edx, [esp + 48]

    ; 保存 current_esp
    mov eax, [esp + 40]
    mov [eax], esp

    ; 載入 next_esp (這一步之後，我們就正式跨入新任務的堆疊了！)
    mov eax, [esp + 44]
    mov esp, [eax]

    ; 【跨越宇宙】堆疊切換完成，現在將剛才暫存的 CR3 寫入硬體！
    mov cr3, edx

    popf
    popa

    ret
```

存檔後，再執行一次 `make clean && make run`。這一次，當你輸入 `echo hello`，OS 就不會再崩潰了。因為 `echo.elf` 會在它專屬的新宇宙裡執行，而當它死亡後，老爸 Shell 會從睡夢中醒來，並發現自己的宇宙完好無缺！😎
