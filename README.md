
全文: [Blog 版本](https://rickhw.github.io/2026/03/27/ComputerScience/Simple-OS/)

## 很久以前就想做的事 - **純手工自幹一個作業系統**

有點瘋狂，有點無聊，但卻又超有趣！

以前做不來，是因為門檻高、網路上的資料雜亂、內容太硬核，加上沒人可以討論、工作忙碌沒時間等因素。

但現在是 AI 時代，許多過去看似困難、門檻極高的事，在 AI 的協助下都變得有趣且可行！


![](/about/Result-Window.png)

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

心裡有 `它們都是作業系統` 的念頭一直放在心上，而那段繁忙的工作歷程，我利用上下班坐公車往返兩小時的時間，回去反覆重讀作業系統，包含上清大開放課程 - 周志遠教授 的 [作業系統](https://www.youtube.com/playlist?list=PLS0SUwlYe8cxj8FCPRoPHAehIiN9Vo6VZ)、follow 成大 [黃敬群 Jserv - Linux 核心設計][r3] 課程。但是這些 follow 總是淺薄，加上工作上各種 Interrupt 與 Context Switch，我覺得都只是學到一些皮毛。後來我也買了 `作業系統概念 (俗稱恐龍書)` 回來啃，補足很多重要的概念，像是 [淺談同步機制][ct3] 就是整理第六章的內容。

![](/about/OS-Concepts.jpeg)

但恐龍書講的是概念，缺少實作。所以我又透過 `Linux Programming Interface` 了解更多實際上在 Linux 上怎麼實作的細節，像是 Linux 怎麼管理共享記憶體。

![](/about/Linux-Programming-Interface1.jpeg)
![](/about/Linux-Programming-Interface2.jpeg)


過去幾年 (Since 2016)，我花最多時間研究的主題是 [分散式系統](/categories/Distributed-Systems/) 以及延伸的主題，但實際上研究到最後發現，本質還是回到作業系統。


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

至於是否繼續念 CS 或者 CS 無用論，不是第一次遇到這個問題，2023 年 12 月，ChatGPT 剛上線的那個月，我去 成大資工所演講，主題是 [軟體職涯的分享][ct9]，當時就有同學問我如何面對 AI 世代；2025 年 6 月我在高雄示範大學演講時，現場也有同學和老師問到同樣的問題。

這個題目放到最後統一整理，本質思路是這樣：

> AI 很強，工具很好用，那你想做什麼？或者你能做什麼？

如果現在不缺錢與資源，那你想做什麼？或者給你最強的 AI 你想做什麼？

如果都沒有，那你能做什麼？


---

## 實作內容

整個實作大概分成底下六大部分：

1. **環境與中斷實作**：透過組合語言從 Bootloader 開始，實作 GDT / IDT，控制 IRQ 處理鍵盤中斷。
2. **存儲系統**：實作 MBR 解析與簡易檔案系統 (FileSystem)。
3. **多工核心**：實作 Timer、Context Switch 與 Scheduler，讓單核 CPU 實現「多工」的幻覺。
4. **記憶體與行程**：實作 MMU Paging，實現記憶體隔離、`fork`/`exec` 機制、`Shell` 的誕生，以及 IPC (進程間通訊) 與 Mutex 機制。
5. **使用者空間 (User Space)** ：開發 C Library，包含 `malloc`/`free`/`printf` 等基礎函式。透過 Syscall 打造基礎指令（`ls`/`mkdir`/`cat`/`touch`/`rm`）及內建指令（`cd`/`pwd`/`exit`）。
6. **圖形介面 (GUI)** ：實作簡單的視窗介面，包含滑鼠 I/O、Double Buffering（雙重緩衝）機制、字型渲染，並實作基礎的 Window Manager。

這可以想像成六個 Sprint，每個部分都有約 10 個核心功能需要達成。


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
EAX=00000000 EBX=00000000 ECX=02000000 EDX=02000628
ESI=0000000b EDI=02000000 EBP=06feb010 ESP=00006d5c
EIP=000ebede EFL=00000046 [---Z-P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000044 CCD=ffffffff CCO=EFLAGS
EFER=0000000000000000

EAX=000000b5 EBX=00007d47 ECX=00005678 EDX=00000003
ESI=06f31fb0 EDI=06ffee71 EBP=0000695c ESP=0000695c
EIP=00007d47 EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =da80 000da800 ffffffff 00809300
CS =f000 000f0000 ffffffff 00809b00
SS =0000 00000000 ffffffff 00809300
DS =0000 00000000 ffffffff 00809300
FS =0000 00000000 ffffffff 00809300
GS =ca00 000ca000 ffffffff 00809300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000004 CCD=00000001 CCO=EFLAGS
EFER=0000000000000000
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08

Servicing hardware INT=0x09
Servicing hardware INT=0x08
Servicing hardware INT=0x20
     0: v=20 e=0000 i=0 cpl=0 IP=0008:001002a1 pc=001002a1 SP=0010:00109d38 env->regs[R_EAX]=ffffff50
EAX=ffffff50 EBX=00105084 ECX=000001f7 EDX=000001f7
ESI=00109f78 EDI=00109f78 EBP=00002800 ESP=00109d38
EIP=001002a1 EFL=00000286 [--S--P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
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
CCS=00000084 CCD=ffffffc0 CCO=EFLAGS
EFER=0000000000000000
Servicing hardware INT=0x21
     
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


### 結果與原始碼

整個作業系統 binary (`myos.bin`) 大小僅約 48 KB。

```bash
❯ ls -lh
total 96
drwxr-xr-x@ 7 rickhwang  staff   224B Mar 26 21:15 kernel
-rwxr-xr-x  1 rickhwang  staff    48K Mar 26 21:15 myos.bin
drwxr-xr-x@ 6 rickhwang  staff   192B Mar 26 21:10 user
```

底下錄影分別 Demo 了 Terminal 和 Window：

<iframe width="560" height="315" src="https://www.youtube.com/embed/WVQSnAE8AJM?si=9qayW1VGcYEGr65J" title="YouTube video player" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>

<iframe width="560" height="315" src="https://www.youtube.com/embed/07ytT9O286E?si=LVDDYjLzQMv8oWhf" title="YouTube video player" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>


原始碼放在 [GitHub](https://github.com/rickhw/simple-operating-systems) 上，用 Branch 分開每個階段的內容：

![](/about/Source-Code-Branch.png)

目錄 course 則有完整的 [課程大綱](https://github.com/rickhw/simple-operating-systems/tree/main/course)、[課程藍圖](https://github.com/rickhw/simple-operating-systems/blob/main/course/Course-Overview.mermaid)，以及每個階段的原始碼，以及 [專有名詞對照表](https://github.com/rickhw/simple-operating-systems/blob/main/course/Terms.md) 。

![](/about/Course-Overview1.png)
![](/about/Course-Overview2.png)

![](/about/Course-Overview.png)


### 課前知識與準備

1. C 語言, 組合語言
1. 對計算機組織有基本認識: 記憶體
1. 在 macOS / Ubuntu 24.04 都可以正常編譯
1. Docker (推薦 OrbStack) 與 QEMU

```bash
❯ qemu-system-i386 -version
QEMU emulator version 10.2.2
Copyright (c) 2003-2025 Fabrice Bellard and the QEMU Project developers
```


### 跟恐龍書比較起來

這個課程內容，嚴格說比 `30 天打造 OS - 作業系統自作入門` 簡單很多，算是很容易快速上手、或者說體驗開發作業系統的種種問題。我實作完之後，回去翻 `恐龍書`、`30 天打造 OS` 感覺是偏簡單的。可能是我本身就已經在實務上有一定程度的經驗。


---
# 延伸閱讀

## 站內文章

- [聊聊 API First 開發策略][ct6]
- [從應用程式 Inside-Out 出發的 Web 可靠性設計][ct7] - 台灣軟體工程協會, 高雄師範大學
- [管理者如何持續學習技術？][ct8]
- [在生命轉彎的地方 - 從軟體開發職涯，探索人生][ct9] - 成功大學 資工所 20231201_5

## 參考資料

- [COSCUP 2012 第一次自幹作業系統核心就上手 - Jserv][r1]
- [11010周志遠教授｜作業系統 - 字幕版][r2]
- [Linux 核心設計][r3]



[r1]: https://www.youtube.com/watch?v=m6R1mFpifa4
[r2]: https://www.youtube.com/playlist?list=PLS0SUwlYe8cxj8FCPRoPHAehIiN9Vo6VZ
[r3]: https://www.youtube.com/@sysprog

[ct1]: /2018/05/12/SQA/Designing-Test-Architecture-and-Framework/
[ct2]: /2025/12/19/About/Java-2D-RPG-Game/
[ct3]: /2020/09/12/ComputerScience/Synchronization/
[ct4]: /2013/11/17/OSX/Install-OSX-Mavericks-on-PC/
[ct5]: /2023/09/11/DistributedSystems/SaaS-KeyDesign_MultiTenancy-with-Isolation-Factor/
[ct6]: /2025/03/08/Design/Gossip-API-First-Joureny/
[ct7]: https://speakerdeck.com/rickhwang/cong-ying-yong-cheng-shi-inside-out-chu-fa-de-web-ke-kao-xing-she-ji-tai-wan-ruan-ti-gong-cheng-xie-hui-gao-xiong-shi-fan-da-xue-20250628
[ct8]: /2019/03/30/Management/How-do-Manager-Keep-Learning/
[ct9]: https://speakerdeck.com/rickhwang/zai-sheng-ming-zhuan-wan-de-di-fang-cong-ruan-ti-kai-fa-zhi-ya-tan-suo-ren-sheng-20231201-5
[ct10]: /2019/09/11/SoftwareEngineering/Great-Software/


[l1]: https://github.com/Netflix/conductor
[l2]: https://www.bmc.com/it-solutions/automation-orchestration.html
