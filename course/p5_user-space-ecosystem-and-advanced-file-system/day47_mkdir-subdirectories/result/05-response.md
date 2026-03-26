哈哈，Rick！這是一個超級經典的「跨界失憶」問題！😂

還記得我在 Day 47 的步驟 3 偷偷給你留了一個小挑戰嗎？
> *(💡 你需要同步去修改 `unistd.c`、`simplefs.c` 和 `syscall.c` 裡的 `readdir`，讓它多傳遞一個 `type` 變數出來。這對你現在的功力來說絕對沒問題！)*

我看了一下你貼上來的 `lib/simplefs.c`，你非常完美的重構了 User Space 的 `unistd.c` 與 `ls.c`，**但是你忘記更新 Kernel Space 裡的函式啦！**

因為 Kernel 裡面的 `simplefs_readdir` 依然只接收 `out_name` 和 `out_size`，所以它根本沒有把目錄的 `type` 回傳給 User Space！導致你的 `ls.c` 裡面 `int type = 0;` 永遠都是 0 (也就是 `FS_FILE`)，所以不管怎樣它都印出 `[FILE]`。

來，我們把這條從 Kernel 通往 User Space 的「地下水管」給接通吧！

### 🛠️ 步驟 1：打通 Kernel 的 VFS 與 SimpleFS

請打開 **`lib/simplefs.c`**，找到這兩個函式，把 `out_type` 加進去，並把值塞進去：

```c
// 【修改】新增第五個參數 out_type
int simplefs_readdir(uint32_t part_lba, int index, char* out_name, uint32_t* out_size, uint32_t* out_type) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_root_dir(part_lba, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buf;

    int valid_count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] != '\0') {
            if (valid_count == index) {
                for(int j=0; j<32; j++) out_name[j] = entries[i].filename[j];
                *out_size = entries[i].file_size;
                
                // 【關鍵點】把地契上的 Type 取出來，還給 User Space！
                *out_type = entries[i].type; 
                
                kfree(dir_buf);
                return 1;
            }
            valid_count++;
        }
    }
    kfree(dir_buf);
    return 0;
}

// 【修改】同步更新 VFS 的封裝
int vfs_readdir(int index, char* out_name, uint32_t* out_size, uint32_t* out_type) {
    if (mounted_part_lba == 0) return -1;
    return simplefs_readdir(mounted_part_lba, index, out_name, out_size, out_type);
}
```

### 🛠️ 步驟 2：打通 Syscall 的暫存器橋樑

現在，Kernel 的水管接好了，我們要確保 Syscall 接線生有正確傳遞這個指標。
還記得你的 `libc/syscall.c` 怎麼定義的嗎？
`int syscall(int num, int p1, int p2, int p3, int p4, int p5)` 對應的暫存器是：
`p1(ebx), p2(ecx), p3(edx), p4(esi)`。

所以你的 `readdir` 傳入了四個參數，第四個參數 `out_type` 被放在了 **`ESI`** 暫存器裡！

請打開 **`lib/syscall.c`**，找到 `eax == 15` 的地方，把它更新成這樣：

```c
    // 記得在 syscall.c 頂部更新外部宣告
    // extern int vfs_readdir(int index, char* out_name, uint32_t* out_size, uint32_t* out_type);

    // Syscall 15: sys_readdir (讀取目錄內容)
    else if (eax == 15) {
        int index = (int)regs->ebx;
        char* out_name = (char*)regs->ecx;
        uint32_t* out_size = (uint32_t*)regs->edx;
        
        // 【新增】從 ESI 暫存器拿出 out_type 指標！
        uint32_t* out_type = (uint32_t*)regs->esi; 

        ipc_lock();
        regs->eax = vfs_readdir(index, out_name, out_size, out_type);
        ipc_unlock();
    }
```

---

完成這兩個修改後，因為我們沒有動到硬碟的實體結構，你這次 **不需要** `make clean-disk`，直接：
```bash
make clean && make run
```

進去系統後，再敲一次 `ls`，原本偽裝成 FILE 的 `folder1`，就會瞬間現出原形，變成帥氣的 **`[DIR]  folder1`** 啦！

快去試試看，這就是傳遞 Pointer 穿透 Ring 0 與 Ring 3 的醍醐味！😎
