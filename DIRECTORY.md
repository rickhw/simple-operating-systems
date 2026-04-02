# Simple OS Directory Structure

本文件說明 Simple OS 重構後的目錄結構。目前結構參考 Linux Kernel 設計，將核心原始碼依功能進行解耦。

## 根目錄 (Root)
- `Makefile`: 專案編譯腳本，支援遞迴目錄搜尋與 Docker 編譯環境。
- `Dockerfile`: 定義編譯環境，包含 GCC, NASM, GRUB 等工具。
- `SKILLS.md`: 專案開發所需的底層技術與知識彙整。
- `README.md`: 專案簡介與快速上手指南。

## 原始碼目錄 (src/)

### 核心 (src/kernel/)
核心程式碼採用功能模組化分類：

- **`arch/x86/`**: 處理器架構相關實作。
    - `boot.S`: 多重引導 (Multiboot) 進入點與核心啟動。
    - `gdt.c`, `gdt_flush.S`: 全域描述符表 (Global Descriptor Table) 設定。
    - `idt.c`, `interrupts.S`: 中斷描述符表 (Interrupt Descriptor Table) 與中斷處理。
    - `paging.c`, `paging_asm.S`: 分頁機制 (Paging) 與虛擬記憶體管理。
    - `timer.c`: 系統時鐘 (PIT) 驅動。
    - `user_mode.S`, `switch_task.S`: 任務切換與進入 Ring 3 的底層組合語言。

- **`drivers/`**: 硬體驅動程式。
    - `block/`: 儲存設備驅動 (ATA 硬碟、MBR 解析)。
    - `input/`: 輸入設備驅動 (PS/2 鍵盤、滑鼠)。
    - `net/`: 網路卡驅動 (Realtek RTL8139)。
    - `pci/`: PCI 匯流排掃描與初始化。
    - `rtc/`: 即時時鐘 (Real-Time Clock)。
    - `video/`: 顯示輸出驅動 (LFB 繪圖引擎、TTY 字元終端機)。

- **`fs/`**: 檔案系統層。
    - `vfs.c`: 虛擬檔案系統 (Virtual File System) 抽象層。
    - `simplefs.c`: SimpleFS 實體檔案系統實作。
    - `elf.c`: ELF 執行檔格式解析與載入。

- **`init/`**: 系統引導。
    - `main.c`: 核心主程式 (Kernel Main)，負責初始化所有次系統。

- **`kernel/`**: 核心邏輯層。
    - `task.c`: 任務管理、排程器 (Scheduler) 與行程生命週期。
    - `syscall.c`: 系統呼叫 (System Call) 分派中心。
    - `gui.c`: 視窗管理員 (Window Manager) 與圖形介面引擎。

- **`mm/`**: 記憶體管理。
    - `pmm.c`: 實體記憶體管理員 (Physical Memory Manager)。
    - `kheap.c`: 核心堆疊 (Kernel Heap) 動態分配器。

- **`net/`**: 網路協定棧。
    - `arp.c`: 位址解析協定 (Address Resolution Protocol)。
    - `icmp.c`: 控制訊息協定 (ICMP / Ping)。

- **`lib/`**: 核心基礎函式庫。
    - `utils.c`: 字串操作、記憶體複製 (memcpy, memset) 等工具函式。

- **`include/`**: 所有的核心標頭檔 (.h)。

### 使用者空間 (src/user/)
- **`asm/`**: User-mode 引導組合語言 (如 `crt0.S`)。
- **`bin/`**: 應用程式原始碼 (Shell, Explorer, Paint, Clock 等)。
- **`include/`**: 使用者端使用的標頭檔 (API 定義)。
- **`lib/`**: 使用者端函式庫 (libc 實作、系統呼叫封裝)。

## 腳本目錄 (scripts/)
- `grub.cfg`: GRUB 開機選單設定。
- `linker.ld`: 核心連結腳本，定義各個段 (Section) 的記憶體分佈。
