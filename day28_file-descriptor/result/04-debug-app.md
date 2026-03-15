
# Q: 我在 build app.c 出現 asm operand 有問題，如下：

❯ cat build-app.sh
#!/bin/bash

# 1. 編譯獨立的 User App
docker run --platform linux/amd64 --rm -v $(pwd):/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -c app.c -o app.o

# 2. 連結 App，並強制規定它要在 0x08048000 執行
docker run --platform linux/amd64 --rm -v $(pwd):/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 app.o -o my_app.elf
❯ ./build-app.sh
app.c: In function 'sys_read':
app.c:10:5: error: 'asm' operand has impossible constraints
   10 |     __asm__ volatile("mov $4, %%eax\n mov %1, %%ebx\n mov %2, %%ecx\n mov %3, %%edx\n int $0x80\n mov %%eax, %0"
      |     ^~~~~~~
ld: cannot find app.o: No such file or directory


---

這又是一個非常經典的底層坑！你遇到的這個編譯錯誤，精準點出了 **GCC Inline Assembly (內聯組合語言)** 最惡名昭彰的「暫存器分配衝突」。

### 🔍 為什麼 GCC 會抱怨 "impossible constraints" (不可能的約束)？

在原本的程式碼中，我們寫了類似這樣的東西：

```c
: "=r"(bytes_read) : "r"(fd), "r"(buffer), "r"(size) : "eax", "ebx", "ecx", "edx"

```

這段話的意思是告訴 GCC：

1. 請幫我隨便找 4 個通用的暫存器 (`"r"`)，把參數塞進去。
2. 但是！執行完這段組合語言後，`eax`, `ebx`, `ecx`, `edx` 這 4 個暫存器的值都會被我破壞掉（Clobber list）。

問題來了：在 x86 (32-bit) 架構下，通用的暫存器本來就極少。當你把這 4 個最重要的暫存器都宣告為「會被破壞」，GCC 在嘗試分配 `"r"` 時，發現自己**根本沒有足夠的暫存器可以用了**（或者發現分配的暫存器跟你宣告破壞的暫存器打架了），於是它就雙手一攤，罷工報錯。

### 🛠️ 終極解法：讓 GCC 直接幫我們把值放定位

與其讓 GCC 隨便挑暫存器，然後我們再手動寫 `mov` 搬移，不如直接使用 GCC 的**「特定暫存器約束 (Specific Register Constraints)」**！

在 GCC 中：

* `"a"` 代表強制使用 `eax`
* `"b"` 代表強制使用 `ebx`
* `"c"` 代表強制使用 `ecx`
* `"d"` 代表強制使用 `edx`

請打開你的 **`app.c`**，用下面這段極度優雅且符合 Linux 系統呼叫標準的寫法，替換掉原本的三個函式：

```c
// 系統呼叫封裝
int sys_open(char* filename) {
    int fd;
    __asm__ volatile (
        "int $0x80"
        : "=a" (fd)             // 輸出：執行完後，把 eax 的值放進 fd
        : "a" (3),              // 輸入：執行前，把 3 放進 eax
          "b" (filename)        // 輸入：執行前，把 filename 放進 ebx
        : "memory"              // 告訴編譯器記憶體可能被改變
    );
    return fd;
}

int sys_read(int fd, char* buffer, int size) {
    int bytes_read;
    __asm__ volatile (
        "int $0x80"
        : "=a" (bytes_read)     // 輸出：把 eax 的值放進 bytes_read
        : "a" (4),              // 輸入：eax = 4
          "b" (fd),             // 輸入：ebx = fd
          "c" (buffer),         // 輸入：ecx = buffer
          "d" (size)            // 輸入：edx = size
        : "memory"
    );
    return bytes_read;
}

void sys_print(char* msg) {
    __asm__ volatile (
        "int $0x80"
        :                       // 沒有輸出變數
        : "a" (2),              // 輸入：eax = 2
          "b" (msg)             // 輸入：ebx = msg
        : "memory"
    );
}

void _start() {
    sys_print("[User App] Hello! I am running in Ring 3!\n");
    sys_print("[User App] Requesting Kernel to open 'hello.txt'...\n");

    int fd = sys_open("hello.txt");
    
    if (fd == -1) {
        sys_print("[User App] Failed to open file!\n");
    } else {
        sys_print("[User App] File opened! FD received. Reading data...\n");
        
        char buffer[128];
        for(int i=0; i<128; i++) buffer[i] = 0; // 清空
        
        sys_read(fd, buffer, 100);
        
        sys_print("[User App] File Content -> ");
        sys_print(buffer);
        sys_print("\n");
    }

    while(1);
}

```

這套寫法完全去除了我們手寫的 `mov`，讓編譯器自己去最佳化暫存器的載入過程，既乾淨又不會踩到資源衝突的雷。

換上這段程式碼後，再跑一次你的 `./build-app.sh`，絕對會順利編譯出 `my_app.elf`！接著重新跑一次 `make run`，看看你的 User App 是不是順利把硬碟裡的文字印在畫面上了？
