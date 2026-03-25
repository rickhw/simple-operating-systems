
# Q

用 qemu 啟動一個 vm 時，我沒有指定任何硬體資訊，那預設的 CPU / Memory / Disk / Network 會是什麼？像是 `qemu-system-i386 -cdrom myos.iso -d int -no-reboot -hda hdd.img -boot d`


# A

在使用 QEMU 啟動虛擬機時，如果你沒有明確指定參數（如 `-m` 或 `-cpu`），QEMU 會套用一組**保守且相容性極高**的預設值。這組預設值是為了確保即使在老舊的硬體上也能順利開機。

針對你的指令 `qemu-system-i386`，以下是預設的硬體規格拆解：

---

### 1. CPU (處理器)
* **預設型號：** `q486` (或 `pentium`)。
* **特性：** 雖然你的實體電腦可能是強大的 i9 或 Apple M3，但 QEMU 預設會模擬一個非常基礎的 32 位元 x86 處理器。它不支援現代的擴充指令集（如主機的變體功能），目的是為了最大化通用性。
* **核心數：** 預設為 **1 核心** (1 vCPU)。

### 2. Memory (記憶體)
* **預設大小：** **128 MB**。
* **開發體感：** 對於開發簡單的 Kernel 來說，128MB 非常充裕。但如果你未來開始實作複雜的圖形介面或載入大型檔案，記得使用 `-m 512M` 或 `-m 1G` 來手動增加。

### 3. Disk / Storage (磁碟與儲存)
根據你的指令：
* **`-hda hdd.img`**：QEMU 會將此檔案模擬為 **IDE 介面** 的第一個主硬碟（Primary Master）。
* **`-cdrom myos.iso`**：模擬為 **IDE 介面** 的第二個設備（通常是 Primary Slave 或 Secondary Master 的光碟機）。
* **控制器：** 預設模擬傳統的 **PIIX3 / PIIX4 IDE 控制器**。這也是為什麼你在寫 ATA 驅動程式時，通常會從 I/O Port `0x1F0` 開始操作的原因。



### 4. Network (網路)
* **預設網卡：** **Intel E1000** (或者在某些舊版本中是 `ne2k_pci`)。
* **網路模式：** **User-mode Networking (SLIRP)**。
    * 這就像是在虛擬機後方躲了一個虛擬的 NAT 路由器。
    * 虛擬機會自動透過 DHCP 取得 `10.0.2.15` 這個 IP。
    * 它不需要 root 權限就能上網，但外部實體機無法直接連進虛擬機。

### 5. 其他重要預設硬體
* **顯示卡 (VGA)：** 預設模擬 **Standard VGA** (或 Cirrus Logic GD5446)。這支援基本的 VESA 模式。
* **晶片組：** 預設模擬 **i440FX**（這是 1990 年代非常經典的 Intel 晶片組）。
* **韌體：** 預設使用 **SeaBIOS**（開源的 x86 BIOS 實作）。它負責在你的 `boot.S` 執行前，先進行 POST 並把光碟的啟動磁區載入記憶體。

---

### 你的指令參數解析補充

* **`-d int`**：這對 OS 開發極其重要！它會叫 QEMU 在螢幕或終端機印出所有的 **中斷 (Interrupts)** 紀錄。當你的 Kernel 發生 Page Fault 或 General Protection Fault 導致重開機時，看這裡的 dump 資訊（包含暫存器快照）是唯一的除錯手段。
* **`-no-reboot`**：預設情況下，如果 Kernel 崩潰觸發 Triple Fault，CPU 會自動重啟。加上這個參數可以讓 QEMU 停在崩潰那一刻，讓你有機會檢查日誌。
* **`-boot d`**：強制從 **光碟 (Drive D)** 啟動。

---

### 下一步建議
如果你發現虛擬機跑得有點慢，或者想測試更現代的 CPU 指令，可以嘗試加入：
* **`-cpu host`**：讓虛擬機使用跟你的實體電腦一模一樣的 CPU 指令集。
* **`-accel kvm`** (Linux) 或 **`-accel hvf`** (macOS)：開啟硬體加速，速度會提升 10 倍以上。

**既然提到了硬體資訊，需要我幫你整理一份常用的 QEMU 除錯指令清單（例如如何在運行中檢查暫存器狀態）嗎？**
