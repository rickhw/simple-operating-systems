# 緣由 / 動機


Just for fun


實作過 AWS DynamoDB 的 Capacutiy Unit 機制，應用在 API 架構上。


[Facebook](https://www.facebook.com/rick.kyhwang/posts/pfbid02RrS7sEbJjWry84bNM1t2totc9fWYoHCQ61heQ3XT2vzN3aCknVqvmBp4BvEfQBf6l)

[Thread](https://www.threads.com/@sjej77138/post/DVvrnqHElku)

[周志遠教授｜作業系統](https://www.youtube.com/playlist?list=PLS0SUwlYe8cxj8FCPRoPHAehIiN9Vo6VZ)

[黃敬群 Jserv - Linux 核心設計]

[管理者如何持續學習技術？](https://rickhw.github.io/2019/03/30/Management/How-do-Manager-Keep-Learning/)

知其然，理論與實務

現代分散式系統

有興趣的 Topic, 像是 IPC


## 有趣的實作

- Day39/40: 實作 IPC (Inter Process Communication), 實作單核心的鎖機制: Mutex
- Built-in: help, about, exit, cd, pwd, path solution
- ls, touch, cat, mkdir



## BI and Workflow 工作流程

- 自幹的
- [Netflix Conductor](https://github.com/Netflix/conductor)
- [BMC - Orchestration](https://www.bmc.com/it-solutions/automation-orchestration.html)
- n8n
- OpenClaw


## 方法

一個 Session 作為主要課程內容，取得課程內容，討論主要課程問題。
另一個 Session 當作助教，諮詢像是 c / asm / qemu 等知識


## 心得：課前知識

1. c 語言
2. 組合語言
3. 對計算機組織有基本認識: 記憶體


---
# 心得與思考

## 有 AI 之後，還需要學作業系統？還需要學計算機科學 (CS)？

### 學這個有什麼用？

### 交給小龍蝦就好？或者用現代各種 AI Agent，像是 AntiGravity, Cursor, Kiro, 

寫應用，也就是功能需求 (Functional Requirement, FR)，大部分都不太難，

寫非功能需求 (Non-Functional Requirement, NFR)，對於 AI 來講，也不太難。

但是把 FR & NFR 這兩個放在一起，是大部分的人比較不會意識到的，所以不會特別去跟 AI Agent 説。

## Multiple AI Agent

我看到很多人已經掉入 multitask 的認知問題了，也就是開了一堆 AI Agent，同時做很多事，做了一堆 Context Switch。

其實開一堆 AI Agent，人自己轉換成調度者，這個本質上就是主管在做的事，包含決定產品發展方向、任務分配、資源調度、品質標準、驗收標準、上線計劃 ... 等。



## 太多、太快、太跳

做軟體架構師的時候，為了要滿足業務需求，在解決問題思路需要銜接業務需求與軟體工程之間。

這兩者介於抽象、具象之間，而往往在表達不夠完整，以及經驗上的落差之下，就會造成「太多」、「太快」、「太跳」的現象。

這些現象，在 AI 興起之後，又更明顯了。


## 專業的傲慢

我在學習過程，看到 AI 很自然地吐出一堆我看不懂的東西，過程中我回想到學生時代在學習 8051 以及資料結構的時候，那時候可能是我沒有專心聽清楚課堂開始的介紹，或者回家沒有好好複習，經常會發生課程到中間的時候，很多名詞看不懂，然後老師在講的東西，我就跟不上了，然後我也不好意思問。

類似的狀況，在工作上我可能也曾經是這樣的，只是反過來，我是架構師或者主管的角色，專案的第一天，或者前期會提一些基礎概念，或者重要的名詞，但是專案進行到一半，會發現團隊有些人會完全狀況外。實際了解後，才發現，有些基礎的概念，或者產品的專有名詞他根本不懂，或者說跟不上了。但他也不好意思問。

用 AI 不一樣，我就會很不客氣地一直問。


## 這個世界需要這麼快？

人類一直在搞一些很詭異的事，任何技術應該都是要改善人類的生活，提高生活品質。

AI 出現後，出現更多的是裁員、焦慮、越來越快。


## 懶人包？

唯有透過深度的思考，人們才能真正的學習與獲得知識。






---

# Prompt

你是一個資訊科學專家，教授 作業系統實作 這門課程，課程內容是教導學生透過實作一個真的可以運作的小型作業系統，透過課程內容學習到作業系統原理。

1. 請你安排一個為期 60 天，每天兩小時的課程內容，同時每次內容都是迭代、且完整的實作，可以輸出成可運行的內容。
2. 這個課程最終的結果將實作出一個小型的作業系統，包含了開機、核心 (Kernel)、具備簡單 Terminal 以及檔案系統、簡單的視窗、以及基本管理工具，像是 ls、top 等。
3. 學生可以透過學習作業系統，了解到檔案系統、開機程序、程序管理、圖形運作、底層驅動、記憶體管理、網路 ... 等概念。

針對上面的需求，你可以再給予建議或者反饋，然後安排課程內容，我每天會安排固定時間，執行上課。


https://gemini.google.com/app/18b8fcafe3a7592b


---

## 課程設計核心原則

* **從零開始 (From Scratch)：** 不依賴標準函式庫 (No standard library)，我們會自己寫 `printf`、`malloc` 等。
* **迭代開發：** 每一階段都要能編譯並在 QEMU 上運行，看到實質的回饋。
* **工具鏈：** 使用 `gcc-cross-compiler`、`nasm`、`ld` (Linker) 與 `make`。

---

# Simple OS 60 天實作課程大綱

本專案將 60 天的開發旅程劃分為六個衝刺階段（Sprints）。

## Phase 1: System Boot & Kernel Infrastructure 

**第一階段**：啟動與核心骨架。
**目標：** 從 BIOS 接管控制權，建立中斷與基礎輸出能力。

* **Day 1-4:** GRUB Multiboot 啟動機制、VGA 文字模式與 `kprint` 基礎輸出實作。
* **Day 5-8:** GDT (全域描述符表)、IDT (中斷描述符表)、ISR 中斷跳板與 PIC 控制器。
* **Day 9-10:** 鍵盤驅動與 Timer (PIT) 基礎中斷處理，完成硬體事件攔截。

## Phase 2: Memory Management & Privilege Isolation

**第二階段**：記憶體與特權階級大挪移。
**目標：** 建立現代記憶體管理機制，並成功在 Ring 3 執行外部應用程式。

* **Day 11-12:** 任務控制區塊 (TCB) 與基礎 Context Switch (上下文切換)。
* **Day 13-15:** 實體記憶體管理 (PMM)、虛擬記憶體分頁 (Paging)、核心堆積分配器 (`kmalloc`)。
* **Day 16-17:** 系統呼叫 (Syscall, `int 0x80`) 與 TSS 設定，防護降級至 User Mode (Ring 3)。
* **Day 18-20:** ELF 執行檔解析器、GRUB Multiboot 模組接收與虛實記憶體映射。

## Phase 3: Storage, File System & Interactive Shell 

**第三階段**：儲存裝置、檔案生態與互動 Shell。
**目標：** 脫離 GRUB 保母，讓系統具備讀取實體硬碟、動態載入應用程式與雙向互動的能力。

* **Day 21-23:** ATA/IDE 硬碟驅動實作 (PIO 模式) 與 MBR 分區表解析。
* **Day 24-27:** VFS (虛擬檔案系統) 路由層設計與 SimpleFS 檔案系統實作 (支援跨磁區讀寫)。
* **Day 28-30:** 檔案描述符 (FD)、User Stack 分配、動態 ELF 載入器，以及結合鍵盤緩衝區的互動式 Simple Shell。

## Phase 4: Preemptive Multitasking & Process Management 

**第四階段**：搶佔式多工與行程管理。
**目標：** 讓系統同時執行多個應用程式，完善行程生命週期管理與記憶體保護。

* **Day 31-33:** 搶佔式排程器 (Preemptive Scheduler) 實作，利用 Timer 中斷強行切換多個 Ring 3 行程，解決無窮迴圈死鎖問題。
* **Day 34-37:** 實作 UNIX 經典系統呼叫：`fork` (複製行程)、`exec` (替換執行檔)、`exit` 與 `wait`，並讓 Shell 具備解析與動態載入外部 ELF 工具的能力。
* **Day 38-40:** 實作 MMU 記憶體隔離 (Memory Isolation) 與跨宇宙的 CR3 切換；建立核心同步機制 (Mutex) 與行程間通訊 (IPC Message Queue)，徹底解決資料競爭與記憶體奪舍問題。

## Phase 5: User Space Ecosystem & Advanced File System 

**第五階段**：User Space 生態擴張與寫入能力。
**目標：** 打造平民專用的標準 C 函式庫，並讓檔案系統具備建立與寫入能力。

* **Day 41-43:** 建立 User Space 專屬的標準 C 函式庫 (迷你 libc)，封裝底層 Syscall。
* **Day 44-46:** Ring 3 的動態記憶體分配器 (`malloc`/`free`) 與 `sbrk` 系統呼叫。
* **Day 47-50:** SimpleFS 升級（支援目錄結構與 `sys_write` 寫入硬碟），實作進階指令如 `ls`, `mkdir`, `echo > file`。

## Phase 6: Graphical User Interface & Window System 

**第六階段**：圖形介面與視窗系統。
**目標：** 脫離純文字模式，進入高解析度的畫布與視窗世界。

* **Day 51-54:** VBE (VESA BIOS Extensions) 啟動與線性幀緩衝 (Framebuffer) 渲染。
* **Day 55-57:** 滑鼠驅動程式 (PS/2) 實作與基礎圖形引擎開發（畫點、線、矩形）。
* **Day 58-60:** 字體解析渲染與簡單的視窗合成器 (Compositor)，完成 GUI 作業系統雛形。


---

## 課程藍圖

![](Course-Overview.png)

---

## 學習環境

1. **調試是最大的挑戰：** 在 OS 開發中，你沒有 `gdb` 可以直接連。建議先學會如何使用 QEMU 的偵錯開關（如 `-d int` 查看中斷）以及使用 GDB 遠端連接 QEMU 核心。
2. **不要過度設計：** 在這 60 天內，目標是「理解概念」而非做出「下一個 Linux」。例如：檔案系統選 FAT16 就好，不要挑戰實作實時日誌系統。
3. **硬體環境：** **MacBook Pro M1**，你需要建立一個 x86_64 的 Docker 容器來進行編譯，或者使用虛擬機運行 Linux，因為 macOS 的 Linker 與 Linux 格式 (ELF) 不同。

---
