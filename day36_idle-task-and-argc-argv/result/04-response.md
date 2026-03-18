好的，Rick！這個 Bug 真的非常頑固，它就像是躲在作業系統最深處的幽靈。但每一次的 Dump 都是它留下的腳印。

我們冷靜下來，再次審視這個案發現場：
```text
check_exception old: 0xffffffff new 0xe
    3: v=0e e=0004 i=0 cpl=3 IP=001b:080481a9 pc=080481a9 SP=0023:083ffecc CR2=08400004
```
依然是 **Page Fault (0x0E)**，依然是存取了 **`CR2=08400004`**！
而且，這一次你已經把堆疊往下壓了 64 Bytes (`user_stack_top - 64`)，但它竟然還是精準地踩到了 `0x08400004`！

這說明了一件事：**這不是單純的堆疊對齊或溢出問題。這是有一段程式碼，打從心底就想去讀取 `0x08400004` 這個絕對位址！**

### 🔍 案發現場解密：C 語言編譯器的「自作聰明」

為什麼 `_start` 執行時，會去讀取 `0x08400004`？
回想一下我們在 `create_user_task` 裡是怎麼設定 `ESP` 的：
```c
uint32_t *ustack = (uint32_t*) (user_stack_top - 64);
// ... 壓入 argv, argc, 返回位址 ...
*(--kstack) = (uint32_t)ustack;  // 這裡把 ESP 設為了某個低於 0x08400000 的值
```
當 `_start` 被呼叫時，它的 `ESP` 確實是在 `0x083FFxxx` 左右（Dump 顯示是 `083ffecc`）。

那它為什麼會去存取 `0x08400004`？
因為 `_start` 是 C 語言寫的！當編譯器看到 `void _start(int argc, char** argv)` 時，它會生成類似這樣的組合語言：
```assembly
_start:
    push ebp
    mov ebp, esp
    ; 準備讀取 argc 和 argv
    mov eax, [ebp + 8]   ; 讀取 argc
    mov edx, [ebp + 12]  ; 讀取 argv
```
如果 `_start` 剛進來時的 `ESP` (也是 `EBP`) 距離 `0x08400000` 還有幾十個 Bytes 的距離，那 `[ebp + 8]` 和 `[ebp + 12]` 絕對不可能指到 `0x08400004`。

**唯一的可能性是：有人在呼叫 `_start` 之前，把 `ESP` 或是 `EBP` 歸零，或者是設成了 `0x08400000` 附近的值！**

誰會做這種事？
**C 語言的標準啟動碼 (CRT)！**

雖然我們在編譯 `app.c` 時使用了 `-nostdlib` 和 `-fno-builtin`，但有時候 GCC 還是會偷偷塞入一些被稱為 `prologue` (開場白) 的隱藏指令，特別是當函式名稱是 `main` 或 `_start` 這種特殊名稱時。它可能會試圖將堆疊對齊到 16 bytes 邊界，或者做一些奇怪的初始化。

為了解決這個問題，我們必須**徹底剝奪 C 語言編譯器對進入點的控制權**！

### 🗡️ 終極修復 7.0：純組合語言的 User Space 進入點

我們要在 `app.c` 的前面，加上一小段**純組合語言寫的跳板**。這個跳板負責接住 Kernel 傳過來的 `argc` 和 `argv`，然後再乾淨地呼叫 C 語言的函式。這也是真實作業系統（如 Linux）中 `crt0.o` (C Runtime 0) 的標準做法。

請跟著我進行這 3 個步驟的修改：

#### 1. 建立 `src/crt0.S` (User Space 啟動跳板)
在 `src/` 目錄下建立一個新檔案 `crt0.S`（注意副檔名是大寫的 S）：

```assembly
[bits 32]
global _start
extern main ; 告訴組合語言，我們要呼叫 C 語言的 main 函式

section .text
_start:
    ; 當 OS 將控制權交給我們時，堆疊的最頂端是假返回位址，
    ; 接著是 argc (在 esp+4)，然後是 argv (在 esp+8)
    
    ; 為了安全，我們先把 EBP 清零，這代表 Stack Trace 的最底層
    xor ebp, ebp
    
    ; 把 argc 和 argv 從堆疊裡拿出來，準備傳給 main
    mov eax, [esp + 4]  ; eax = argc
    mov edx, [esp + 8]  ; edx = argv
    
    ; 將參數壓入堆疊，呼叫 main
    push edx            ; 參數 2: argv
    push eax            ; 參數 1: argc
    call main
    
    ; 如果 main 結束了，我們主動呼叫 sys_exit (Syscall 7) 來結束生命
    ; 這樣就不需要依賴堆疊裡的假返回位址了！
    mov eax, 7          ; Syscall 7 = sys_exit
    int 0x80
    
    ; 永遠不該執行到這裡
.hang:
    jmp .hang
```

#### 2. 修改 `src/app.c` 的進入點名稱
請把 `app.c` 裡面的 `void _start(int argc, char** argv)` 改名為 `int main(int argc, char** argv)`。

```c
// ... 前面封裝的 Syscall 保持不變 ...

// 【修改】將 _start 改名為 main，並回傳 int
int main(int argc, char** argv) {
    sys_print("\n======================================\n");
    sys_print("      Welcome to Simple OS Shell!     \n");
    sys_print("======================================\n");

    // 把接收到的參數印出來證明靈魂轉移成功！
    if (argc > 0) {
        sys_print("Awesome! I received arguments:\n");
        for(int i = 0; i < argc; i++) {
            sys_print("  Arg ");
            char num[2] = {i + '0', '\0'};
            sys_print(num);
            sys_print(": ");
            
            // 增加安全檢查，確保 argv[i] 不是 NULL
            if (argv[i] != 0) {
                sys_print(argv[i]);
            } else {
                sys_print("(null)");
            }
            sys_print("\n");
        }
        sys_print("\n");
    }

    char cmd_buffer[128];
    while (1) {
       // ... 保持不變 ...
    }
    
    return 0; // 配合 int main 的回傳值
}
```

#### 3. 修改 `Makefile` (或是你的編譯指令)
我們要把 `crt0.S` 加進 User App 的編譯流程裡。
請更新你用來編譯 `my_app.elf` 的 docker 指令：

原本你是這樣寫的：
```bash
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -c app.c -o app.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 app.o -o my_app.elf
```

請改成先編譯 `crt0.S`，然後在連結時把它放在最前面：
```bash
# 編譯 crt0.S
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder nasm -f elf32 crt0.S -o crt0.o

# 編譯 app.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -c app.c -o app.o

# 連結時，確保 crt0.o 放在第一個！這樣它的 _start 才會被設定為真正的 Entry Point
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 crt0.o app.o -o my_app.elf
```

---

這個做法徹底切斷了 C 語言編譯器對 Entry Point 的染指。現在，User App 的啟動過程是 100% 由我們寫的組合語言控制，它會精準地從堆疊裡拿出參數並傳給 `main`。

這是作業系統開發中極為經典的一步。請試試看這個終極解法，讓我們終結這個 `0x08400004` 的幽靈！
