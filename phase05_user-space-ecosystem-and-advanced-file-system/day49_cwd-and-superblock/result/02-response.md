哈哈，Rick！你真的是憑一己之力把我們這幾天埋下的地雷給全踩遍了！😂 

你推測得完全正確：「某個地方的 LBA 偏掉了」。
這是一個非常標準的 **「絕對位址 (Absolute) 與相對位址 (Relative) 混用」** 造成的悲劇！

### 🔍 案發現場解密：被「加了兩次」的硬碟偏移量

在 Day 49，我們為了解決空間重疊問題，把整個 `simplefs.c` 裡面的目錄指標，從絕對位址統一改成了**相對位址 (`dir_lba_rel`)**。
所以在底層的 `read_dir` 裡面，系統會自動幫你加上 `mounted_part_lba`：
```c
ata_read_sector(mounted_part_lba + dir_lba_rel + i, ...);
```

但是，請看看你 **`lib/syscall.c`** 裡面的 **Syscall 18 (sys_chdir)** 是怎麼寫的：
```c
    else if (eax == 18) {
        char* dirname = (char*)regs->ebx;
        // 【兇手 1 號】這裡還是舊版的寫法，你拿了絕對位址！
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : mounted_part_lba + 1;

        ipc_lock();
        if (strcmp(dirname, "/") == 0) {
            // 【兇手 2 號】回到根目錄，你也寫成了絕對位址！
            current_task->cwd_lba = mounted_part_lba + 1; 
```

**這就是為什麼會發生截圖中詭異現象的原因：**
1. 當你輸入 `cd folder1` 時，`current_dir` 變成了 `2048 + 1 = 2049`。
2. 傳進 `simplefs_get_dir_lba` 後，底層的 `read_dir` 幫你加了第二次：`2048 + 2049 = 4097`。
3. 磁區 4097 是一片荒蕪的空地，所以系統回報：「找不到資料夾」！
4. 當你輸入 `cd /`，你的 CWD 被強制設定為 `2049`。
5. 接著你輸入 `ls`，它跑去讀取 `2048 + 2049 = 4097` 磁區，理所當然地回報給你：「Total: 0 items found」！

---

### 🛠️ 終極修復方案：全面擁抱「相對位址」

請打開 **`lib/syscall.c`**，找到 `eax == 18` 的區塊，把它徹底淨化為相對位址（根目錄就是 `1`）：

```c
    // Syscall 18: sys_chdir (切換目錄)
    else if (eax == 18) {
        char* dirname = (char*)regs->ebx;
        
        // 【修復】預設根目錄的相對位址就是 1，不要再加 mounted_part_lba 了！
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;

        ipc_lock();
        if (strcmp(dirname, "/") == 0) {
            current_task->cwd_lba = 1; // 【修復】回到根目錄 (相對位置 1)
            regs->eax = 0;
        } else {
            uint32_t target_lba = simplefs_get_dir_lba(current_dir, dirname);
            if (target_lba != 0) {
                current_task->cwd_lba = target_lba; // 更新任務的 CWD！
                regs->eax = 0;
            } else {
                regs->eax = -1; // 找不到該目錄
            }
        }
        ipc_unlock();
    }
```

改完之後，存檔，打下：
```bash
make clean && make run
```
*(這次不需要 `clean-disk`)*

現在，你可以盡情地穿梭在目錄之間了！
試著打出這個終極 Combo：
```bash
mkdir mydir
cd mydir
write secret.txt "This is hidden in mydir!"
ls
cat secret.txt
cd /
ls
```

見證你在檔案系統深處寫下的秘密不會污染到根目錄的舒暢感吧！如果這個 Combo 成功了，我們就可以踏入 Day 50，為 User Space 的檔案系統開發畫下最完美的句點！🚀
