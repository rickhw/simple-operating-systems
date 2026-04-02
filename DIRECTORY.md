# Simple OS Directory Structure

本文件說明 Simple OS 的目錄結構。目前結構參考 Linux Kernel 設計，將核心原始碼依功能進行解耦。

## 專案整體結構 (Project Overview)

```text
/ (Root)
├── Makefile           # 專案編譯腳本 (支援 Docker 編譯環境)
├── Dockerfile         # 定義編譯環境 (GCC, NASM, GRUB 等工具)
├── README.md          # 專案簡介與快速上手指南
├── DIRECTORY.md       # 目錄結構說明 (本文件)
├── SKILLS.md          # 專案開發所需的底層技術與知識彙整
├── scripts/           # 系統組態與連結腳本
│   ├── grub.cfg       # GRUB 開機選單設定
│   └── linker.ld      # 核心連結腳本 (定義記憶體分佈)
├── src/               # 原始碼主目錄
│   ├── kernel/        # 核心 (Kernel) 原始碼
│   └── user/          # 使用者空間 (User Space) 原始碼
└── course/            # 課程開發進度 (Day 1 ~ Day 80+)
```

---

## 核心原始碼 (src/kernel/)

核心程式碼採用功能模組化分類，實現高度解耦：

```text
src/kernel/
├── arch/x86/          # 處理器架構相關實作
│   ├── boot.S         # 多重引導 (Multiboot) 進入點
│   ├── gdt.c          # 全域描述符表 (GDT) 設定
│   ├── idt.c          # 中斷描述符表 (IDT) 與處理
│   ├── paging.c       # 分頁機制與虛擬記憶體管理
│   ├── timer.c        # 系統時鐘 (PIT) 驅動
│   └── *.S            # 任務切換與 Ring 3 進入點的組合語言
├── drivers/           # 硬體驅動程式
│   ├── block/         # 儲存設備 (ATA, MBR 解析)
│   ├── input/         # 輸入設備 (PS/2 鍵盤、滑鼠)
│   ├── net/           # 網路卡 (RTL8139)
│   ├── pci/           # PCI 匯流排掃描
│   ├── rtc/           # 即時時鐘 (RTC)
│   └── video/         # 顯示輸出 (LFB 繪圖、TTY 終端)
├── fs/                # 檔案系統層
│   ├── vfs.c          # 虛擬檔案系統 (VFS) 抽象層
│   ├── simplefs.c     # SimpleFS 實體檔案系統
│   └── elf.c          # ELF 執行檔格式解析
├── init/              # 系統引導與入口
│   └── main.c         # 核心主程式 (初始化所有次系統)
├── kernel/            # 核心邏輯層
│   ├── task.c         # 任務管理、排程器 (Scheduler)
│   ├── syscall.c      # 系統呼叫 (System Call) 分派中心
│   └── gui.c          # 視窗管理員 (Window Manager)
├── mm/                # 記憶體管理
│   ├── pmm.c          # 實體記憶體管理員 (PMM)
│   └── kheap.c        # 核心堆疊 (Kernel Heap) 分配
├── net/               # 網路協定棧
│   ├── arp.c          # 位址解析協定 (ARP)
│   └── icmp.c         # 控制訊息協定 (ICMP / Ping)
├── lib/               # 核心基礎函式庫 (utils, string, memory)
└── include/           # 核心標頭檔 (.h)
```

---

## 使用者空間 (src/user/)

包含應用程式、函式庫與系統呼叫介面：

```text
src/user/
├── bin/               # 應用程式原始碼
│   ├── shell/         # 命令列介面
│   ├── explorer/      # 檔案管理器
│   ├── clock/         # 數位時鐘
│   └── ...            # 其他 GUI/CLI 應用
├── lib/               # 使用者端函式庫 (libc)
│   ├── stdio.c        # 標準輸入輸出 (printf 等)
│   ├── stdlib.c       # 標準函式庫 (malloc, free)
│   └── syscall.c      # 系統呼叫封裝 (User-side wrappers)
├── asm/               # User-mode 引導組合語言 (crt0.S)
└── include/           # 使用者端標頭檔 (API 定義)
```
