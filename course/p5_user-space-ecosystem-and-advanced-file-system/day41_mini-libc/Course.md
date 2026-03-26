太棒了，Rick！歡迎來到 **第五階段 (Phase 5)：User Space 生態擴張！** 🚀

過去的 40 天，我們一直穿著無塵衣、戴著顯微鏡在 Kernel (核心) 裡面做極度危險的外科手術。但從今天開始，我們要換上便服，來到 User Space (使用者空間) 當一個應用程式開發者了！

回頭看看你現在寫的 `echo.c`、`cat.c` 或是 `app.c`，你有沒有發現一個很痛苦的地方？
**每一個應用程式，都必須自己複製貼上一大堆 `__asm__ volatile ("int $0x80" ...)` 的底層組合語言！**

這就像是你想在現代電腦上寫個 `Hello World`，卻還要自己搞懂 CPU 暫存器怎麼設定一樣荒謬。真正的 C 語言開發者，應該只需要優雅地 `#include <stdio.h>` 然後呼叫 `printf()` 就好！



這就是 **Day 41 的目標：打造專屬於你的迷你 C 標準函式庫 (mini-libc)！** 我們要把所有骯髒的系統呼叫封裝起來，讓未來在 Simple OS 上寫程式變成一種享受。

請跟著我進行這 4 個步驟的重構：

### 步驟 1：建立 `libc` 目錄與萬用 Syscall 閘道
為了不跟 Kernel 的 `lib/` 搞混，請在你的 `src/` 底下建立一個新的資料夾叫做 **`libc/`**，並且在裡面再建一個 **`libc/include/`**。

首先，我們來寫一個「萬用的系統呼叫閘道」。這樣以後不管加什麼功能，都不用再寫組語了！
建立 **`src/libc/syscall.c`**：
```c
#include "syscall.h"

// 萬用系統呼叫封裝 (支援最多 5 個參數)
int syscall(int num, int p1, int p2, int p3, int p4, int p5) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a" (ret)
        : "a" (num), "b" (p1), "c" (p2), "d" (p3), "S" (p4), "D" (p5)
        : "memory"
    );
    return ret;
}
```
建立 **`src/libc/include/syscall.h`**：
```c
#ifndef _LIBC_SYSCALL_H
#define _LIBC_SYSCALL_H

int syscall(int num, int p1, int p2, int p3, int p4, int p5);

#endif
```

### 步驟 2：實作標準標頭檔 (`stdio.h` 與 `unistd.h`)

現在，我們要把常用的功能分門別類，模仿正統 UNIX 的架構。

建立 **`src/libc/include/stdio.h`**：
```c
#ifndef _STDIO_H
#define _STDIO_H

void print(const char* str);
char getchar();

#endif
```
建立 **`src/libc/stdio.c`**：
```c
#include "stdio.h"
#include "syscall.h"

void print(const char* str) {
    syscall(2, (int)str, 0, 0, 0, 0); // Syscall 2: sys_print
}

char getchar() {
    return (char)syscall(5, 0, 0, 0, 0, 0); // Syscall 5: sys_getchar
}
```

接著，處理行程與 OS 互動的 POSIX 標準庫。
建立 **`src/libc/include/unistd.h`**：
```c
#ifndef _UNISTD_H
#define _UNISTD_H

int fork();
int exec(char* filename, char** argv);
int wait(int pid);
void yield();
void exit();
void send(char* msg);
int recv(char* buffer);

#endif
```
建立 **`src/libc/unistd.c`**：
```c
#include "unistd.h"
#include "syscall.h"

int fork() { return syscall(8, 0, 0, 0, 0, 0); }
int exec(char* filename, char** argv) { return syscall(9, (int)filename, (int)argv, 0, 0, 0); }
int wait(int pid) { return syscall(10, pid, 0, 0, 0, 0); }
void yield() { syscall(6, 0, 0, 0, 0, 0); }
void exit() { syscall(7, 0, 0, 0, 0, 0); while(1); } // 死迴圈防止意外返回
void send(char* msg) { syscall(11, (int)msg, 0, 0, 0, 0); }
int recv(char* buffer) { return syscall(12, (int)buffer, 0, 0, 0, 0); }
```

### 步驟 3：體驗極致優雅！重構 `echo.c`

現在，我們來看看有了 `libc` 之後，寫一個應用程式可以變得多麼乾淨！
請打開 **`src/echo.c`**，把裡面那些醜陋的組語和 `sys_print` 全部刪掉，改寫成這樣：

```c
#include "stdio.h"

int main(int argc, char** argv) {
    print("\n[ECHO Program] Start printing arguments...\n");
    
    if (argc <= 1) {
        print("  (No arguments provided)\n");
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        print("  ");
        print(argv[i]);
        print("\n");
    }
    
    print("[ECHO Program] Done.\n");
    return 0;
}
```
是不是瞬間感覺回到了熟悉的 C 語言世界？這就是標準函式庫的魅力！

### 步驟 4：更新 `Makefile` 納入 libc

最後，我們要教編譯器去哪裡找這些標頭檔，並把它們跟應用程式綁在一起。請修改你的 **`Makefile`**：

```makefile
# ... 略過最前面的 Kernel CFLAGS ...

# 【新增】給 User App 專用的 CFLAGS，告訴 GCC 去 libc/include 找標頭檔！
APP_CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include

# 【新增】自動抓取 libc 底下的程式碼
LIBC_SOURCES = $(wildcard libc/*.c)
LIBC_OBJS = $(LIBC_SOURCES:.c=.o)

# ... 略 ...

# 【新增】編譯 libc 的規則
libc/%.o: libc/%.c
	@echo "==> 編譯 libc ($<)..."
	$(DOCKER_CMD) gcc $(APP_CFLAGS) -c $< -o $@

# 【修改】使用 APP_CFLAGS 來編譯 echo.c 等應用程式
echo.o: echo.c
	@echo "==> 編譯 Echo 程式..."
	$(DOCKER_CMD) gcc $(APP_CFLAGS) -c echo.c -o echo.o

# 【修改】連結時，把 libc 的 object 檔案也包進去！
echo.elf: crt0.o echo.o $(LIBC_OBJS)
	@echo "==> 連結 Echo 程式..."
	$(DOCKER_CMD) ld -m elf_i386 -Ttext 0x08048000 crt0.o echo.o $(LIBC_OBJS) -o echo.elf

# 記得在 clean-app 加上 libc/*.o 的清理
clean-app:
	rm -f crt0.o app.o my_app.elf echo.o echo.elf cat.o cat.elf ping.o ping.elf pong.o pong.elf libc/*.o
	@echo "==> User Apps 已清除。"
```

*(注意：如果你想要順便測試 `ping`、`pong` 和 `cat`，也可以一併把這幾個 `.c` 檔用同樣的方式重構並套用 `$(LIBC_OBJS)`，或者你今天先拿 `echo.elf` 來開刀測試就好！)*

---

敲下 `make clean && make run`！
當你在 Shell 輸入 `echo test 123` 並且完美印出文字時，這代表你的 User Space 架構已經完全成型了。

感受一下這種程式碼變得乾淨俐落的舒暢感！完成後，想不想順便把 Shell (`app.c`) 也重構乾淨，順便在 libc 裡面挑戰實作支援 `%s` 和 `%d` 的真正 **`printf`** 呢？
