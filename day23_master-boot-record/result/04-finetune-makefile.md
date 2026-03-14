太棒了！能夠順利讀出 `Start LBA: 2048`，代表你的 Kernel 已經具備了看懂真實硬碟幾何結構的能力。

你想到要把這個步驟寫進 `Makefile`，這個直覺非常精準！這正是 DevOps 的核心精神：「Infrastructure as Code (基礎架構即代碼)」。我們不應該依賴任何手動的「魔法點擊」，所有的建置步驟都應該能被一鍵重現。

為了不讓你還要麻煩地去重包 `os-builder` 的 Docker 映像檔，我們可以在 Makefile 裡面借用一個極度輕量級的 **Alpine Linux** 容器（下載不用 1 秒鐘），專門用來執行 `fdisk` 切分區的任務。

請打開你的 **`Makefile`**，進行以下調整：

### 1. 新增 `hdd.img` 的建置規則

在 Makefile 中找個合適的地方（通常在 `myos.iso` 規則的附近），加上這段自動產生並切分硬碟的指令：

```makefile
# --- 硬碟映像檔建置規則 ---
hdd.img:
	@echo "==> 建立 10MB 的空白虛擬硬碟..."
	dd if=/dev/zero of=hdd.img bs=1M count=10
	@echo "==> 使用 Alpine 容器與 fdisk 寫入 MBR 分區表..."
	docker run --rm -i --platform linux/amd64 -v $(CURDIR):/workspace -w /workspace alpine sh -c "apk add --no-cache util-linux && echo -e 'o\nn\np\n1\n\n\nw' | fdisk hdd.img"
	@echo "==> 硬碟 hdd.img 建立與分區完成！"

```

*💡 **亮點解釋**：我們用 `$(CURDIR)` 取得當前目錄掛載進去。`alpine` 是一個只有 5MB 的極小 Linux，我們讓它在一秒內啟動、安裝 `util-linux` (包含 fdisk)、透過 `echo` 模擬人類按鍵切好分區，然後自動銷毀容器，乾淨俐落！*

### 2. 更新 `run` (或 `qemu`) 指令

如果你有一個專門用來啟動 QEMU 的標籤（例如 `run:` 或 `test:`），請把 `hdd.img` 加入依賴條件，並補上昨天加的參數：

```makefile
# 確保每次啟動前，ISO 和硬碟都準備好了
run: myos.iso hdd.img
	qemu-system-i386 -cdrom myos.iso -hda hdd.img -boot d

```

### 3. ⚠️ 溫馨提醒：關於 `clean`

**強烈建議：不要把 `rm -f hdd.img` 放進你的 `clean` 規則裡！**
因為作業系統開發的常態是「頻繁修改 Kernel 程式碼，然後重新編譯測試」。如果你每次 `make clean` 都把硬碟刪掉重做，未來你存在硬碟裡的檔案系統和資料就會一直被清空。
硬碟是「持久化狀態」，應該獨立存在。如果你真的想重置硬碟，可以多寫一個 `clean-disk` 標籤：

```makefile
clean-disk:
	rm -f hdd.img
	@echo "==> 硬碟已刪除，下次 make run 將會產生一顆全新的硬碟。"

```

---

完成這些修改後，你可以試著在終端機輸入 `make clean-disk` 把舊的刪掉，然後直接輸入 `make run`。
你會看到 Makefile 自動幫你無中生有生出一顆硬碟、切好分區、編譯 Kernel、打包 ISO，最後啟動 QEMU 一氣呵成！

這種高度自動化的開發體驗，絕對會讓你在後續開發檔案系統時事半功倍。

確定 Makefile 運作順暢後，我們就可以進入 **Day 24：虛擬檔案系統 (VFS) 抽象層**，準備把這塊從 LBA 2048 開始的空地，蓋成一個有目錄、有檔名的漂亮社區了！準備好隨時出發！
