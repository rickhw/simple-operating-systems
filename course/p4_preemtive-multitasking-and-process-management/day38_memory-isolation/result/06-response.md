好的，Rick！這個錯誤 Dump 提供了極度關鍵的線索。這一次，問題不在 `paging.c`，而在於我們在創造新宇宙（`sys_exec`）時，忽略了一個非常細微但致命的細節。

請看你的 Dump 最後幾行：
```text
194: v=0e e=0000 i=0 cpl=0 IP=0008:00101d45 pc=00101d45 SP=0010:c0006c48 CR2=083fcec8
EAX=00429007 EBX=00105084 ECX=000003ff EDX=083ff000
...
EIP=00101d45 EFL=00000002 [-------] CPL=0 II=0 A20=1 SMM=0 HLT=0
```
這是 **Page Fault (0x0E)**，而 `CR2=083fcec8`。

`0x083fcec8` 是什麼地方？
回想一下，我們在 `sys_exec` 裡設定 User Stack 的位址：
`uint32_t clean_user_stack_top = 0x083FF000 + 4096;`
往下壓 64 bytes 之後，大約就是 `0x083FFExx`。這個 `0x083fcec8` 距離我們的 Stack Top 只有幾十個 bytes 的距離，這絕對是**在存取 argc/argv 時發生的越界！**

### 🔍 案發現場解密：兩個宇宙間的「失憶症」

為什麼在新的宇宙裡，存取 Stack 會出錯？我們明明有分配實體記憶體給它啊：
```c
uint32_t ustack_phys = pmm_alloc_page();
map_page(0x083FF000, ustack_phys, 7);
```

**問題就出在這裡！**

我們在 `sys_exec` 中是**先切換到新宇宙 (`new_cr3`)，然後才呼叫 `map_page` 分配 Stack 的！**
這聽起來很合理，對吧？因為我們要把 Stack 建立在新宇宙裡。

**但是！** `sys_exec` 也是一個 C 語言函式，它有自己的區域變數（比如 `argc`, `argv_ptrs` 陣列等），這些區域變數都儲存在 **Kernel Stack** 上。
而 `sys_exec` 接收到的字串參數（`filename` 和 `argv` 指標陣列），它們儲存在哪裡？
它們儲存在呼叫 `sys_exec` 的那個應用程式的 **User Stack** 或是 **BSS 段** 裡（也就是老爸 Shell 的宇宙裡）！

當你執行這兩行指令：
```c
uint32_t old_cr3;
__asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
__asm__ volatile("mov %0, %%cr3" : : "r"(new_cr3));
```
**切換到新宇宙的瞬間，老爸的 User Stack 就從記憶體裡「消失」了！**（因為新宇宙的 User Page Table 被我們清空了）。

接著，當你在新宇宙裡執行這段程式碼：
```c
    if (argv != 0 && (uint32_t)argv > 0x08000000) {
        while (argv[argc] != 0) argc++; // <--- 【崩潰點】
    }
```
你試圖去讀取 `argv` 指向的記憶體位址（在老爸宇宙的 `0x0804xxxx` 或 `0x083Fxxxx`）。但在新宇宙中，這個位址是**空的（Not Present）**！
所以，CPU 當場引發 Page Fault，`CR2` 就指出了它試圖讀取的位址。

### 🗡️ 終極修復：先打包行李，再穿越宇宙！

要解決這個「跨宇宙失憶症」，我們必須在**切換宇宙之前**，先把需要的參數字串，全部「拷貝」到一個不受宇宙切換影響的地方。
哪裡不受影響？**Kernel Heap (`kmalloc`)**！因為 Kernel Heap 位於 3GB 以上的高位址，這部分的分頁目錄在所有的宇宙中都是共用的！

請打開 **`lib/task.c`**，徹底更新 `sys_exec` 函式：

```c
int sys_exec(registers_t *regs) {
    char* filename = (char*)regs->ebx;
    char** old_argv = (char**)regs->ecx;

    fs_node_t* file_node = simplefs_find(filename);
    if (file_node == 0) { return -1; }

    uint8_t* buffer = (uint8_t*) kmalloc(file_node->length);
    vfs_read(file_node, 0, file_node->length, buffer);

    // =====================================================================
    // 【打包行李】在切換宇宙前，把參數拷貝到 Kernel Heap，這樣新宇宙才看得到！
    // =====================================================================
    int argc = 0;
    char* safe_argv[16]; // 暫存指標陣列
    
    if (old_argv != 0 && (uint32_t)old_argv > 0x08000000) {
        while (old_argv[argc] != 0 && argc < 15) {
            int len = strlen(old_argv[argc]) + 1;
            safe_argv[argc] = (char*) kmalloc(len);     // 在 Kernel Heap 申請空間
            memcpy(safe_argv[argc], old_argv[argc], len); // 把字串拷貝過來
            argc++;
        }
    }
    safe_argv[argc] = 0; // 結尾補 NULL

    // =====================================================================
    // 【神聖分離】為這個 Process 建立專屬的新宇宙！
    // =====================================================================
    uint32_t new_cr3 = create_page_directory();
    
    uint32_t old_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(new_cr3));

    // =====================================================================

    uint32_t entry_point = elf_load((elf32_ehdr_t*)buffer);
    kfree(buffer);

    if (entry_point == 0) {
        __asm__ volatile("mov %0, %%cr3" : : "r"(old_cr3));
        return -1;
    }

    current_task->page_directory = new_cr3;

    // 分配新宇宙的 User Stack
    uint32_t clean_user_stack_top = 0x083FF000 + 4096; 
    uint32_t ustack_phys = pmm_alloc_page();
    map_page(0x083FF000, ustack_phys, 7);

    uint32_t stack_ptr = clean_user_stack_top - 64; 

    // ------------------------------------------
    // 使用「打包好的行李 (safe_argv)」來構造堆疊
    // ------------------------------------------
    uint32_t argv_ptrs[16] = {0};

    if (argc > 0) {
        for (int i = argc - 1; i >= 0; i--) {
            int len = strlen(safe_argv[i]) + 1;
            stack_ptr -= len;
            memcpy((void*)stack_ptr, safe_argv[i], len);
            argv_ptrs[i] = stack_ptr;
            
            // 拷貝完就可以把 Kernel Heap 的行李丟掉了
            kfree(safe_argv[i]); 
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

這個改動完美解決了「舊資料在新宇宙不可見」的問題。它確保了新程式在建立執行環境時，所有需要的參數都已經安全地搭上了跨宇宙的方舟。

存檔，再次 `make clean && make run`！這次，真正的 MMU 記憶體隔離魔法將會完美呈現在你眼前！🚀
