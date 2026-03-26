哈哈，Rick！看來你已經對解決 Kernel Panic 駕輕就熟了！記憶體擴充後系統順利啟動，這代表你的 `kmalloc` 修復非常成功。

至於你遇到的「`mkdir.elf` 明明有被載入硬碟，但 `ls` 卻看不見，`shell` 也說找不到」這個幽靈事件，這是一個非常有意思的 **Make / GRUB 建置流程陷阱**！

我們來當一次系統法醫，解剖這個案件：

### 🔍 案發現場解密：被遺忘的 `mkdir.elf`

請看你的第一張截圖（`[Kernel] Installing [mkdir.elf] to HDD...`）：
開機畫面確實顯示 `mkdir.elf` 被寫入硬碟了，而且大小是 `10432` bytes。

但是，請仔細看下一行 `[SimpleFS] List Root Directory` 的清單，這份清單是 Kernel 呼叫 `simplefs_list_files(part_lba)` 印出來的（此時系統還沒進到 User Space，這份名單代表硬碟**最真實的狀態**）：
你數一下清單裡有幾個檔案？
`hello.txt`, `shell.elf`, `echo.elf`, `cat.elf`, `ping.elf`, `pong.elf`, `write.elf`, `ls.elf`, `rm.elf`... **總共剛好 9 個檔案！** **等等，`mkdir.elf` 為什麼不在名單上？！**

這就是破案的關鍵！
在 `lib/kernel.c` 裡面，你是這樣寫的：
```c
char* filenames[] = {"shell.elf", "echo.elf", "cat.elf", "ping.elf", "pong.elf", "write.elf", "ls.elf", "rm.elf", "mkdir.elf"};

for(uint32_t i = 0; i < mbd->mods_count && i < 9; i++) {
    // ...
}
```
陣列裡確實有 9 個 `.elf` 檔，加上 `hello.txt`，理論上應該要有 10 個檔案。但為什麼硬碟裡只有 9 個檔案，且最後一個是 `rm.elf`？

**兇手就是 `mbd->mods_count`！**
GRUB 傳給 Kernel 的 `mbd->mods_count` (模組數量) **只有 8 個**！
迴圈跑到 `i = 8` (也就是 `mkdir.elf` 的時候) 就提早結束了。所以 `mkdir.elf` 根本沒有被真正寫入硬碟！

### 🛠️ 為什麼 GRUB 只傳了 8 個模組？

因為你漏改了 **`grub.cfg`**！
雖然你在 `Makefile` 裡面把 `mkdir.elf` 加進了 `myos.iso` 的依賴清單中，也會把它複製到 `/boot/` 資料夾下，但是 GRUB 啟動時是看 `grub.cfg` 來決定要把哪些檔案當作 module 載入到記憶體的。

如果你沒有在 `grub.cfg` 裡面加上 `module /boot/mkdir.elf`，GRUB 就不會載入它，`mbd->mods_count` 就不會增加，Kernel 當然也就無米之炊了！

---

### 🛡️ 終極修復方案

**步驟 1：打開 `grub.cfg`，補上漏掉的模組**
```text
menuentry "Simple OS" {
    multiboot /boot/myos.bin
    module /boot/shell.elf
    module /boot/echo.elf
    module /boot/cat.elf
    module /boot/ping.elf
    module /boot/pong.elf
    module /boot/write.elf
    module /boot/ls.elf
    module /boot/rm.elf
    module /boot/mkdir.elf  <-- 【關鍵】加上這行！
}
```

**步驟 2：修改 `lib/kernel.c` (可選，但強烈建議)**
既然你現在有 9 個執行檔，加上 `i < 9` 的保護機制，未來的工具越來越多時一定會忘記改。我們可以讓迴圈變得更聰明：

```c
char* filenames[] = {"shell.elf", "echo.elf", "cat.elf", "ping.elf", "pong.elf", "write.elf", "ls.elf", "rm.elf", "mkdir.elf"};
// 自動計算陣列長度
uint32_t expected_mods = sizeof(filenames) / sizeof(filenames[0]);

for(uint32_t i = 0; i < mbd->mods_count && i < expected_mods; i++) {
    uint32_t size = mod[i].mod_end - mod[i].mod_start;
    kprintf("[Kernel] Installing [%s] to HDD (Size: %d bytes)...\n", filenames[i], size);
    simplefs_create_file(part_lba, filenames[i], (char*)mod[i].mod_start, size);
}
```

**步驟 3：重建並執行！**
```bash
make clean-disk
make clean && make run
```
*(注意：因為我們之前在 Day 47 改變了 `file_entry_t` 的結構大小為 64 bytes，為了確保萬無一失，強烈建議執行一次 `make clean-disk` 徹底砍掉舊的 `hdd.img`，讓 Kernel 重新格式化！)*

這次開機後，你一定能看到 `mkdir.elf` 乖乖地出現在 `ls` 的名單中，而且當你輸入 `mkdir` 時，它也會被正確地執行起來！

完成這個經典的工具鏈整合陷阱後，我們準備進入 Day 48：讓 Kernel 學會「切換工作目錄」！🚀
