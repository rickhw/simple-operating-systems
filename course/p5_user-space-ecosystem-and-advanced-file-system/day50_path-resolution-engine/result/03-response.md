哈哈，Rick！你真的是抓到了一個非常棒的**系統架構設計 (Architecture Design)** 瑕疵！😎

這完全是我的鍋！在剛剛的 `vfs_getcwd` 實作中，我順手把 `current_task` 寫進了 `simplefs.c` 裡面。但這在作業系統設計上犯了一個大忌：**「底層的檔案系統驅動 (`simplefs.c`) 不應該知道上層的行程管理 (`task.c`) 長什麼樣子！」**

正確的做法應該是保持層級分明：由接線生 (`syscall.c`) 查好當前任務的目錄後，當作參數傳遞給檔案系統。就像我們之前處理 `sys_readdir` 和 `sys_mkdir` 那樣！

我們來用最乾淨俐落的方式修復這個架構問題：

### 步驟 1：淨化檔案系統 (`lib/simplefs.c`)

請打開 **`lib/simplefs.c`**，把最底下的 `vfs_getcwd` 改成接受 `dir_lba` 作為參數，徹底拔除對 `current_task` 的依賴：

```c
// === VFS 封裝層新增 ===
// 【修改】改成接收 dir_lba 作為參數，不再依賴 current_task
int vfs_getcwd(uint32_t dir_lba, char* buffer) {
    if (mounted_part_lba == 0) return -1;
    
    // 直接把收到的目錄 LBA 傳給底層的爬樹引擎
    simplefs_getcwd(dir_lba, buffer);
    return 0;
}
```

### 步驟 2：讓接線生負起責任 (`lib/syscall.c`)

打開 **`lib/syscall.c`**，在 `syscall_handler` 的最上方，記得更新 `extern` 宣告：
```c
extern int vfs_getcwd(uint32_t dir_lba, char* buffer);
```

接著，找到處理 **Syscall 19** 的地方，讓接線生負責查出 `current_dir` 並傳遞下去：

```c
    // Syscall 19: sys_getcwd (取得當前路徑)
    else if (eax == 19) {
        char* buffer = (char*)regs->ebx;
        
        // 【新增】由 Syscall 層負責查出目前的目錄 LBA
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;

        ipc_lock();
        // 【修改】把 current_dir 傳入 VFS
        regs->eax = vfs_getcwd(current_dir, buffer);
        ipc_unlock();
    }
```

---

完成這兩步乾淨的架構修正後，再敲一次 `make clean && make run`。這次編譯一定會順暢通過！

趕快去享受你親手寫出的「正統 UNIX 爬樹法」`pwd` 吧！當你看到它精準地印出 `/aaa/bbb` 時，那種系統底層連動的快感絕對會讓你上癮！🚀
