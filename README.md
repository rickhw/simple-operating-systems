
全文: [Blog 版本](https://rickhw.github.io/2026/03/27/ComputerScience/Simple-OS/)

## 很久以前就想做的事 - **純手工自幹一個作業系統**


很久以前就想做的事 - **純手工自幹一個作業系統**

有點瘋狂，有點無聊，但卻又超有趣！

以前做不來，是因為門檻高、網路上的資料雜亂、內容太硬核，加上沒人可以討論、工作忙碌沒時間等因素。

但現在是 AI 時代，許多過去看似困難、門檻極高的事，在 AI 的協助下都變得有趣且可行！


![](/about/Result-Window_20260402.png)


<!-- more -->


## 動機

### `它們都是作業系統` - 資源分配與調度

工作上處理過不少 BI（商業智慧）與 Workflow（工作流）系統，像是自幹的 [自動化回歸測試框架與平台][ct1]、電商系統維護平台 (PowerShell)、電商的非同步批次系統、[Netflix Conductor][l1] 、[BMC - Orchestration][l2]、n8n、Gitlab、SaaS Platform、Artifact & Build System 等。觀察這些系統的本質，其實都是在處理**資源分配與調度**。

我也管理過不少 K8s 平台與雲端架構（AWS / GCP）。以 K8s 為例，維護多個 Cluster 時，必須根據應用程式的屬性（CPU Bound / IO Bound / Memory Bound）來規劃調度策略並引導使用者。AWS 與 GCP 本質上的治理邏輯也是如此。

也曾經實作過類似 AWS DynamoDB 的 Capacity Unit 機制，應用在 [API 架構 / 平台][ct6] 上，實現 [Multi-Tenancy (多租戶架構)][ct5] 的限流與資源分配，可以有效的管控用量，同時讓系統實務上 [可靠性][ct7]。這讓我深刻理解到，分散式系統不論形式為何，最終都要回到作業系統的核心設計機制。不論任務是 BI、K8s 還是雲端，最後都會遇到資源競爭與協調的問題。

**工作上遇到的各種需求與設計，在我眼裡都是一種「作業系統的變體」。** 即使像 K8s 這種複雜的分散式架構，其核心本質與作業系統並無二致。


除了技術經歷，擔任管理職務時，對外協調、對內分配，本質上也是在調度「人與事」。專案管理 (Project Management) 的任務管理就像作業系統的 Scheduler（調度器），根據優先序排列任務。而在管理實務上，常會遇到「重要但不緊急」的技術債發生「飢餓現象 (Starvation)」，這與作業系統處理進程排程的問題如出一轍。


我去年做的 [純手工遊戲開發][ct2] 整個遊戲的各種機制，很多也都是在做資源分配與調度的機制。像是地圖系統、背包系統、多個 NPC 動態路徑 ... 等，都是資源分配與調度的實作。

在計算機科學領域，將資源分配與調度做得最極致、經歷最多考驗的，就是作業系統。其他如演算法、資料結構、計算機網路、計算機結構、離散數學等，都是為了幫助我們更有效率地完成調度任務。**作業系統的核心就是處理 CPU 搶佔、I/O 競爭與記憶體分配。** 徹底了解作業系統的運作，對系統架構設計會有極大的幫助。



### Just for fun

2010 年，我在書店偶然看到《30 天打造 OS：作業系統自作入門》（作者：川合秀實）。書中手把手教你從 Bootloader 開始，用組合語言到 C 語言，一步步寫出一個作業系統。當時覺得如果能親手做出來一定超酷，於是買回家研究。

![](/about/Book-BuildOS-in-30Days.jpeg)

雖然學生時代也學過組合語言 (8051)、C，工作後也實際做過 Embedded 開發工作，但面對這種要懂 CPU 指令集、各種暫存器 ... 等知識，就像一開頭我說的，沒人可以問，過程遇到問題只能 Google 到天荒地老，加上生活上種種因素，大概做到第五天就放棄了 XD

自幹作業系統這件事情就變成心裡的另一個懸念了 ... (不過有轉去做其他事啦，像是 [搞黑金塔][ct4] 之類的，容易很多 ... XD)

在電商領域工作期間，遇到很多流量問題、交易問題、非同步問題、[SaaS / 多租戶架構][ct5] / [API 設計][ct6]，在學 [AWS](/categories/AWS/) / [SRE](/categories/DevOps/SRE/) 過程，也展開 [分散式系統](/categories/Distributed-Systems/) 的探索，這些實務經驗，都持續不斷的讓我加深 `它們都是作業系統` 的想法，覺得還是應該回去自幹一下作業系統才過癮啊 ～～～

心裡有 `它們都是作業系統` 的念頭一直放在心上，而那段繁忙的工作歷程，我利用上下班坐公車往返兩小時的時間，回去反覆重讀作業系統，包含上清大開放課程 - 周志遠教授 的 [作業系統][r2]、follow 成大 [黃敬群 Jserv - Linux 核心設計][r3] 課程。但是這些 follow 總是淺薄，加上工作上各種 Interrupt 與 Context Switch，我覺得都只是學到一些皮毛。後來我也買了 `作業系統概念 (俗稱恐龍書)` 回來啃，補足很多重要的概念，像是 [淺談同步機制][ct3] 就是整理第六章的內容。

![](/about/OS-Concepts.jpeg)

但恐龍書講的是概念，缺少實作。所以我又透過 `Linux Programming Interface` 了解更多實際上在 Linux 上怎麼實作的細節，像是 Linux 怎麼管理共享記憶體。

![](/about/Linux-Programming-Interface1.jpeg)
![](/about/Linux-Programming-Interface2.jpeg)

過去幾年 (Since 2016)，我花最多時間研究的主題是 [分散式系統](/categories/Distributed-Systems/) 以及延伸的主題，但實際上研究到最後發現，本質還是回到作業系統。

不過，紙上單兵，或者在工作上混亂的狀態，都不如自己動手做做看的有趣 ... 加上現在有 AI ..


### 驗證：透過 AI 可以自學很多東西

除了前面提到的個人經歷，以及有趣之外，另外我也想驗證一件事情：

> 透過 AI 自學東西的可行性

現在寫文章，如果不搭上 AI 好像沒有點閱率 XDD

觸發我實際想動手自幹作業系統的源頭是今年 (2026) 很多人在玩 Thread，有很多不錯的話題，有兩個話題是我一直在思考的：

1. [想試著從零開始學習C++ 有什麼資源建議？](https://www.threads.com/@sjej77138/post/DVvrnqHElku)
1. [現在就學中的人，或者剛就業兩年多，要不要繼續學計算機科學？像是作業系統 or 演算法？](https://www.threads.com/@dreamitguitar/post/DVIJ2vXicgn)

這兩個問題，我都有實際上去回答我當時的想法，截圖如下：

![](/about/Thread-Learning-by-AI.png)

同樣問題也丟在 [Facebook](https://www.facebook.com/rick.kyhwang/posts/pfbid02RrS7sEbJjWry84bNM1t2totc9fWYoHCQ61heQ3XT2vzN3aCknVqvmBp4BvEfQBf6l) 試探性看看，不過沒啥反應 XD

![](/about/Thread-Learning-CS1.png)
![](/about/Thread-Learning-CS2.png)


學習部分，我敲完那段話之後，當天 (2026/03/12) 我就開始用同樣的 Prompt 在 Google Gemini 上開工了！重要的是，我不是用 AI Agent 幫我寫，而是在對話過程，請 Gemini 教我，然後一步一步前進。

至於是否繼續念 CS 或者 CS 無用論，不是第一次遇到這個問題，2023 年 12 月，ChatGPT 剛上線的那個月，我去 成大資工所演講，主題是 [軟體職涯的分享][ct9]，當時就有同學問我如何面對 AI 世代；2025 年 6 月我在高雄師範大學演講時，現場也有同學和老師問到同樣的問題。

這個題目放到最後統一整理，本質思路是這樣：

> AI 很強，工具很好用，那你想做什麼？或者你能做什麼？

如果現在不缺錢與資源，那你想做什麼？或者給你最強的 AI 你想做什麼？

如果都沒有，那你能做什麼？


---

## 實作內容

整個實作大概分成底下幾個部分：

1. **環境與中斷實作**：透過組合語言從 Bootloader 開始，實作 GDT / IDT，控制 IRQ 處理鍵盤中斷。
2. **存儲系統**：實作 MBR 解析與簡易檔案系統 (FileSystem)。
3. **多工核心**：實作 Timer、Context Switch 與 Scheduler，讓單核 CPU 實現「多工」的幻覺。
4. **記憶體與行程**：實作 MMU Paging，實現記憶體隔離、`fork`/`exec` 機制、`Shell` 的誕生，以及 IPC (進程間通訊) 與 Mutex 機制。
5. **使用者空間 (User Space)** ：開發 C Library，包含 `malloc`/`free`/`printf` 等基礎函式。透過 Syscall 打造基礎指令（`ls`/`mkdir`/`cat`/`touch`/`rm`）及內建指令（`cd`/`pwd`/`exit`）。
6. **圖形介面 (GUI)** ：實作簡單的視窗介面，包含滑鼠 I/O、Double Buffering（雙重緩衝）機制、字型渲染，並實作基礎的 Window Manager。
7. **行程管理 (Process Manager)** ：實作 top、ps、kill 等行程管理
8. **視窗多工 GUI 生態系** ：實作 File Manager、Task Manager、Event Routing、讀圖 ... 等。
9. **網路 (Network)**: 實作網路卡驅動程式 (rtl8139)、底層協議 ethernet layer, IPv4、通訊協議實作 (ARP, ARP Cache, ICMP, Ping, UDP, TCP), DNS, HTTP, wget 等.

這可以想像成幾個 Sprint，每個部分都有約 10 個核心功能需要達成。


### 學習過程

全程都在 [Google Gemini](https://gemini.google.com/app/18b8fcafe3a7592b) 上同一個 Session 裡面，以問答的方式完成。遇到 Assembly 或者 C 的問題，則開另一個 Session 當「助教」，然後在那邊問細節。

過程常常遇到 Core Dump 和 Debug 的問題，寫作業系統不像一般的 Application，是連 `printf` 都沒得用的，所以常常需要透過 Core Dump 找問題。這時候 AI 就非常有幫助，反正整坨都丟給他就好，這比我第一次寫 bootloader 在除錯時，在 Google 找個半死方便多了 XDD

Core Dump 大概如下，這不知道怎麼看的話，真的會發轟 XD

```bash
❯ make debug
qemu-system-i386 -cdrom myos.iso -d int -no-reboot -hda hdd.img -boot d
WARNING: Image format was not specified for 'hdd.img' and probing guessed raw.
         Automatically detecting the format is dangerous for raw images, write operations on block 0 will be restricted.
         Specify the 'raw' format explicitly to remove the restrictions.
SMM: enter
Servicing hardware INT=0x09
Servicing hardware INT=0x08
Servicing hardware INT=0x20

     2: v=20 e=0000 i=0 cpl=0 IP=0008:00101870 pc=00101870 SP=0010:00109fc8 env->regs[R_EAX]=c0000030
EAX=c0000030 EBX=c000000c ECX=00000000 EDX=c0000030
ESI=c000000c EDI=00105084 EBP=00000000 ESP=00109fc8
EIP=00101870 EFL=00000297 [--S-APC] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 0010a000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     0010a080 0000002f
IDT=     0010a0e0 000007ff
CR0=80000011 CR2=00000000 CR3=0010f000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000095 CCD=ffffffff CCO=EFLAGS
EFER=0000000000000000
^Cqemu-system-i386: terminating on signal 2 from pid 10254 (<unknown process>)
```


### 結果 - Demo & Screenshot

底下錄影 Demo 整個過程，包含編譯與執行：

[![Watch the video](https://img.youtube.com/vi/zXh80VTh-5E/0.jpg)](https://www.youtube.com/watch?v=zXh80VTh-5E)


底下是實作的 Terminal 指令：

- system: shell, echo
- file：touch, cat, ls, rm, mkdir
- Proces: ps, top, kill
- memory: free, segfault
- network: ping


### 結果: Desktop and Window Apps

底下則是 Window App 的實作:

- clock, notepad (筆記本), paint, viewer, 
- explorer, taskmgr, status
- tictactoe (井字遊戲)

過實作一些簡單的 window app，驗證 desktop 底層的機制沒問題，包含 double buffering, window event, event routing, gui event (IPC), window dragging, ui toolkit ... 等 graphic 底層必要的機制. 其實這塊，跟我去年在做 手工開發遊戲 很像, 不過機制要自己實作, 挑戰更高. 有這些機制與 toolkit, 就可實作常用的 window app, 這次實作的有 clock, notepad (筆記本), paint (動態繪圖), viewer (讀 bitmap), explorer (簡易檔案總管), taskmgr, status, tictactoe (井字遊戲) ...

![](/about/Result-Window_20260402.png)


### 結果: 網路

網路實作包含以下：

- 驅動 RTL8139 (Kernel Space)
- 實作 (Kernel Space) Ethernet Layer, IPv4, ARP, ICMP, UDP, TCP
- 封裝 (User Space) Ping, UDP/TCP test, Socket API between Kernel and User Space, DNS, wget, HTTP

下圖是大概的內容：

![](/about/NetworkOverview_20260408.png)

網路功能, 用 ping (UDP) 來驗收. 用結果論來看沒啥, 但實作的內容卻超過前面 windows app 全部做的, 就是增加網路的部分. 包含了網卡驅動程式 (rtl8139), 底層協議 ethernet layer, 通訊協議實作 (ARP, ARP Cache, IPv4, ICMP, Ping, UDP, Socket API) 等實作. 這部分的難度遠超過 前面一堆看起來花俏的 window app, 而且 debug 不太容易, 要搭配 wireshark 才能搞定.


![](/about/result/net_rtl8139-arp-icmp-ping_20260403.png)
![](/about/result/net_icmp-wireshark_20260403.png)


### Binary & 原始碼

整個作業系統 binary (`myos.bin`) 大小僅約 62 KB。

```bash
❯ ls -lh
total 128
drwxr-xr-x@ 11 rickhwang  staff   352B Apr  2 22:46 kernel
-rwxr-xr-x   1 rickhwang  staff    62K Apr  3 13:54 myos.bin
drwxr-xr-x@  6 rickhwang  staff   192B Mar 26 21:10 user
```

原始碼放在 [GitHub](https://github.com/rickhw/simple-operating-systems) 上，結構如下：

```bash
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

核心原始碼 (src/kernel/)：核心程式碼採用功能模組化分類，實現高度解耦：

```bash
src/kernel/
├── arch/x86/          # 處理器架構相關實作
├── drivers/           # 硬體驅動程式
├── fs/                # 檔案系統層
├── init/              # 系統引導與入口
│   └── main.c         # 核心主程式 (初始化所有次系統)
├── kernel/            # 核心邏輯層
├── mm/                # 記憶體管理
├── net/               # 網路協定棧
├── lib/               # 核心基礎函式庫 (utils, string, memory)
└── include/           # 核心標頭檔 (.h)
```

User Space (src/user/)：包含應用程式、函式庫與系統呼叫介面：

```bash
src/user/
├── bin/               # 應用程式原始碼
│   ├── shell/         # 命令列介面
│   ├── explorer/      # 檔案管理器
│   ├── clock/         # 數位時鐘
│   └── ...            # 其他 GUI/CLI 應用
├── lib/               # 使用者端函式庫 (libc)
├── asm/               # User-mode 引導組合語言 (crt0.S)
└── include/       
```

用 Branch 分開每個階段的內容：

![](/about/Source-Code-Branch.png)

目錄則有以下整理：

- [課程大綱](https://github.com/rickhw/simple-operating-systems/tree/main/course)
- [課程藍圖](https://github.com/rickhw/simple-operating-systems/blob/main/course/Course-Overview.mermaid)
- [每個階段的原始碼](https://github.com/rickhw/simple-operating-systems/tree/main/course)
- [專有名詞對照表](https://github.com/rickhw/simple-operating-systems/blob/main/course/Terms.md)
- [x86 Instruction](https://github.com/rickhw/simple-operating-systems/blob/main/course/x86_Instruction.md)


### 課前知識與準備

1. C 語言, 組合語言
1. 對計算機組織有基本認識
1. 一台 macOS or Ubuntu 24.04
1. Docker (macOS 推薦 OrbStack) 與 QEMU

```bash
❯ qemu-system-i386 -version
QEMU emulator version 10.2.2
Copyright (c) 2003-2025 Fabrice Bellard and the QEMU Project developers
```


### 跟恐龍書比較起來

這個課程內容，嚴格說比 `30 天打造 OS - 作業系統自作入門` 簡單很多，算是很容易快速上手、或者說體驗開發作業系統的種種問題。我實作完之後，回去翻 `恐龍書`、`30 天打造 OS` 感覺是偏簡單的。可能是我本身就已經在實務上有一定程度的經驗。

---
## 心得與想法

AI 時代，有很多疑問會反覆地被討論。我在寫這篇的時候，在 Thread 上、各種媒體、朋友之間，看到很多類似的疑問，問題整理大概有以下：

- 大家都在用 OpenClaw 做自動化、或者用 Claude Code 開發產品，為什麼要花時間「自幹」這個？
- 學這個有什麼用？AI 時代還需要學作業系統？還需要學計算機科學 (CS)？
- 交給小龍蝦 (OpenClaw) 就好了，而且可同時開十隻，一下子就可以搞定了！
- 或者用現代各種 AI Agent，像是 AntiGravity、Cursor、Kiro，也是一下子就搞定了

這些問題本質上指向了兩個方向：

- **學習者模式**：我是學生，AI 是導師。透過 AI 學習已知知識（如 CS 基礎、歷史、思辨）。
- **創作者模式**：我是產品經理 (PdM)，AI 是我的工程團隊。我專注於「做什麼」，AI 負責「怎麼做」。

當然，更多時候我們是兩者兼具，只是比重不同。但我認為，在 AI 能夠秒殺所有底層代碼的今天，我們回頭去「純手工」實作，其實是一種對 **知識主權** 的宣示：「**因為我知道它如何運作，所以我能駕馭 AI，而不是被 AI 餵養。**」


用任何技術之前，我會用以下的思路，討論本質問題：

1. 提問者的 **知識與經歷** 的基礎點是什麼？
2. 提問者的 **動機是什麼** ？
3. 在時間上做篇位移

販賣焦慮是容易發財的方法，AI 的出現，讓焦慮指數瞬間提高了很多。問題往往建構基礎、動機的落差，搭配時間模糊，所以販賣焦慮，本質上就是在賣資訊落差造成的焦慮感。


### 動機

沒有 AI 的時代，企業發展到一定階段，就會開始重視生產力、效率化的問題，我的書討論 [平台工程 (Platform Engineering)][ct11]，動機也是在提高生產效率。所以動機不外乎就是降本增效，結果則是增加餘裕，可以做自己的事。

如果從個人、組織到理想世界的角度來看：

- **個人** ：增加生產力、創造被動收入，最終讓自己有「餘裕」做想做的事。
- **組織** ：提高生存機率，提升產能，並降低成本 - 降本增效
- **理想世界** ：如反烏托邦小說《美麗新世界》般的自動化社會，人類不再為生存勞動，完全都在做自己想做的事

對企業來說，用 AI 是為了**「生存」；但對個人來說，回頭學 OS 是為了「進化」**。當 AI 讓程式碼變得廉價，人類的價值就從單純執行的「執行緒 (Thread)」，轉變成了負責決策與資源調度的「核心 (Kernel)」。

繼續往下問，自己的事是什麼事？我們需要進一步探討與思考的是：**當 AI 幫我們省下了時間，我們增加的「餘裕」要拿來做什麼？**。我這段錄影談的是 [投資的目的](https://www.youtube.com/watch?v=f5uRt1IzK2E)，就在深度討論有了餘裕之後的人生。


### 還需要學計算機科學 (Computer Science, CS)？

這是許多 CS 學生的擔憂。業界大佬說寫程式不再值錢，這讓「CS 無用論」甚囂塵上。嘗試用我的思考框架來討論。

主詞換掉就是：

- `黃仁勳 | 馬斯克` 學這個有什麼用？
- `黃仁勳 | 馬斯克` AI 時代還需要學作業系統？
- `黃仁勳 | 馬斯克` 還需要學計算機科學 (CS)？

黃仁勳在 2024 年說 [不需要學寫程式][l3]，是因為他早已具備數十年的底層經驗。作為時代的先行者，他們是創造知識的人。而我們作為跟隨者，必須思考自身的基礎點：**你現在是學習者還是創作者？**

對我而言，動手幹一個 OS 完全是以學習者角度切入，補足我那些「淺薄」的皮毛。更重要的是，這件事極其有趣。就像鋼鐵人裡的 Jarvis，它能幫 Tony Stark 完成無數繁瑣的設計與計算，但我們不能忽略一個事實：**如果 Stark 本身不懂飛行力學與材料科學，他永遠造不出 Mark 3，他只會是一個「下錯指令」的 PM**。

學生學習目的，是探索知識，增加自己的基礎能力。如果以就業思考，那建議多往實際的應用思考，也就是我在 [本質思考][ct12] 反覆提及的，在生活中學習。

寫這篇文章的時候，剛好有人在 Thread 上在分享 [實作 x86 CPU](https://www.threads.com/@tsla.mark/post/DWXY9UEgfWg)，我猜動機應該跟我差不多吧 XD


### 交給龍蝦就好了？

OpenClaw (俗稱龍蝦) 這段時間 (2025/12 ~ 2026/03) 很紅，有機會像電影鋼鐵人裡的 [Tony Stark][l5] 的個人助理 Jarvis，幫 Stark 完成很多設計工作。OpenClaw 則可能成為人類第一個實質 Jarvis 的祖先。

不過回到現實，就是我開頭反覆提及的事情：**資源分配與調度**

反過來問好了:

> 給你十個人來自不同領域的專家，他們每個人都有 Ph.D 的學位。
> 你要帶領這十個人，去完成一個任務。
> 
> 你會怎麼分派工作？

這個問題，就是我在開頭反覆提到的概念：**它們都是作業系統**，本質都在做 **資源分配與調度**

很多人會說，現在任務就是交給 AI Agent 去做，不需要關注他怎麼做，而是專注產出就好。所以很多 PM 都一樣可以做產品，不需要懂技術。

這段話其實就是一個主管在做的工作，但實務上，很多人又會有以下靠北：

> 我的主管啥都不懂，技術能力又差，他憑什麼當主管？

回到個人身上，先不管有沒有 AI 可以用，先思考自己的腦袋裡有什麼？知識？經驗？經歷？怎麼判斷產出？什麼是產出？在生活中，你為什麼要買 Macbook Pro？為什麼要用 Windows？你怎麼判斷？

我以前寫過這篇文章：[什麼是好軟體？][ct10]、 [本質思考][ct12]，就是在探討怎麼做判斷，即使這台 Macbook Pro 不是我做的，他依舊可以達到我預期的水準。

當管理者或 PdM 本身不了解產品的「邊界」與「品質指標」時，產出就會失控。而塑造這些指標的方法，就是實際去體驗、去實作、去理解「好產品」的底層邏輯。

這回到了我反覆提及的概念：「**它們都是作業系統**」。不管是帶領團隊還是下 Prompt 給 Agent，本質都在做 資源分配與調度。


### AI 時代的 CS 學習路徑

透過這次實作，我驗證了 AI 是極佳的導師。我建議在 AI 時代，學習 CS 可以分為三個 Level：

- **Level 1 (PdM 模式)** ：讓 AI 寫 Code，你只負責看結果。這適合快速驗證。
- **Level 2 (學習者模式)** ：讓 AI 解釋 Code，你負責理解背後的 OS 或演算法原理。
- **Level 3 (架構師模式)** ：你定義核心架構與資源調度邏輯，讓 AI 填補細節。

這次「自幹作業系統」，我是在 Level 2 與 Level 3 之間反覆修煉。雖然工具會變，但「資源調度」的本質邏輯是永恆的。**唯有理解底層，你才能在 AI 時代，從一個被調度的 Thread，進化成掌握全局的 Kernel**。


### Star Trak - First Contact

「這東西花了多少錢？」

![](/about/StarTrak_FirstContact/01.png)

『未來的經濟是不太一樣的』
『金錢 ... 在 24 世紀是不存在的』

![](/about/StarTrak_FirstContact/02.png)
![](/about/StarTrak_FirstContact/03.png)
![](/about/StarTrak_FirstContact/04.png)

「沒有錢？」
「你是說你們沒有薪水？」

![](/about/StarTrak_FirstContact/05.png)
![](/about/StarTrak_FirstContact/06.png)

『獲取財富不再是生命的動力』
『我們工作是為了超越自己 造福人類』

![](/about/StarTrak_FirstContact/07.png)
![](/about/StarTrak_FirstContact/08.png)
![](/about/StarTrak_FirstContact/09.png)

---
# 延伸閱讀

## 站內文章

- [Designing Test Architecture and Framework][ct1]
- [純手工遊戲開發心得紀錄 - Java 2D RPG Game][ct2]
- [淺談同步機制][ct3]
- [Install Mac OS Mavericks on PC (蘋果安裝筆記)][ct4]
- [SaaS 關鍵設計 - Multi-Tenancy - 探討真實世界的租賃關係][ct5]
- [聊聊 API First 開發策略][ct6]
- [從應用程式 Inside-Out 出發的 Web 可靠性設計][ct7] - 台灣軟體工程協會, 高雄師範大學
- [管理者如何持續學習技術？][ct8]
- [在生命轉彎的地方 - 從軟體開發職涯，探索人生][ct9] - 成功大學 資工所 20231201_5
- [什麼是好軟體？][ct10]
- [置頂 - 思考本質、實踐、抽象、想像力、教育][ct12]

## 參考資料

- [COSCUP 2012 第一次自幹作業系統核心就上手 - Jserv][r1]
- [11010周志遠教授｜作業系統 - 字幕版][r2]
- [Linux 核心設計][r3]


[r1]: https://www.youtube.com/watch?v=m6R1mFpifa4
[r2]: https://www.youtube.com/playlist?list=PLS0SUwlYe8cxj8FCPRoPHAehIiN9Vo6VZ
[r3]: https://www.youtube.com/@sysprog

[ct1]: https://rickhw.github.io//2018/05/12/SQA/Designing-Test-Architecture-and-Framework/
[ct2]: https://rickhw.github.io//2025/12/19/About/Java-2D-RPG-Game/
[ct3]: https://rickhw.github.io//2020/09/12/ComputerScience/Synchronization/
[ct4]: https://rickhw.github.io//2013/11/17/OSX/Install-OSX-Mavericks-on-PC/
[ct5]: https://rickhw.github.io//2023/09/11/DistributedSystems/SaaS-KeyDesign_MultiTenancy-with-Isolation-Factor/
[ct6]: https://rickhw.github.io//2025/03/08/Design/Gossip-API-First-Joureny/
[ct7]: https://speakerdeck.com/rickhwang/cong-ying-yong-cheng-shi-inside-out-chu-fa-de-web-ke-kao-xing-she-ji-tai-wan-ruan-ti-gong-cheng-xie-hui-gao-xiong-shi-fan-da-xue-20250628
[ct8]: https://rickhw.github.io//2019/03/30/Management/How-do-Manager-Keep-Learning/
[ct9]: https://speakerdeck.com/rickhwang/zai-sheng-ming-zhuan-wan-de-di-fang-cong-ruan-ti-kai-fa-zhi-ya-tan-suo-ren-sheng-20231201-5
[ct10]: https://rickhw.github.io//2019/09/11/SoftwareEngineering/Great-Software/
[ct11]: https://rickhw.github.io//2023/07/17/About/2023-SRE-Practice-and-IDP/
[ct12]: https://rickhw.github.io//2017/11/26/Thinking-in-Essence/

[l1]: https://github.com/Netflix/conductor
[l2]: https://www.bmc.com/it-solutions/automation-orchestration.html
[l3]: https://www.bnext.com.tw/article/78420/jensen-huang-computer-past
[l5]: https://zh.wikipedia.org/zh-tw/%E6%9D%B1%E5%B0%BC%C2%B7%E5%8F%B2%E5%A1%94%E5%85%8B_(%E6%BC%AB%E5%A8%81%E9%9B%BB%E5%BD%B1%E5%AE%87%E5%AE%99)
