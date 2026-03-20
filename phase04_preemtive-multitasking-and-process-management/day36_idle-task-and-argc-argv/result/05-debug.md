依照你的建議，修改傳後，出現在 crt0.S 的錯誤，看起來是 app.c 之間有衝突？

```bash
❯ make clean; make
rm -f crt0.o app.o my_app.elf  # 【修改 3】記得把 crt0.o 也清掉
==> User App 已清除。
rm -f hdd.img
rm -f asm/*.o lib/*.o *.bin *.iso
rm -rf isodir
docker build -t os-builder .
[+] Building 1.5s (7/7) FINISHED                                                                             docker:orbstack
 => [internal] load build definition from Dockerfile                                                                    0.0s
 => => transferring dockerfile: 288B                                                                                    0.0s
 => WARN: FromPlatformFlagConstDisallowed: FROM --platform flag should not use constant value "linux/amd64" (line 1)    0.0s
 => [internal] load metadata for docker.io/library/debian:bullseye                                                      1.4s
 => [internal] load .dockerignore                                                                                       0.0s
 => => transferring context: 2B                                                                                         0.0s
 => [1/3] FROM docker.io/library/debian:bullseye@sha256:943d97fa707482c24e1bc2bdd0b0adc45f75eb345c61dc4272c4157f9a2cc9  0.0s
 => CACHED [2/3] RUN apt-get update && apt-get install -y     nasm     build-essential     grub-pc-bin     grub-common  0.0s
 => CACHED [3/3] WORKDIR /workspace                                                                                     0.0s
 => exporting to image                                                                                                  0.0s
 => => exporting layers                                                                                                 0.0s
 => => writing image sha256:2e718a84f2e8302d405984c3c30d974040605b9ac63e0ac05e677ea5eefbc551                            0.0s
 => => naming to docker.io/library/os-builder                                                                           0.0s

 1 warning found (use docker --debug to expand):
 - FromPlatformFlagConstDisallowed: FROM --platform flag should not use constant value "linux/amd64" (line 1)
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder nasm -f elf32 asm/boot.S -o asm/boot.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder nasm -f elf32 asm/crt0.S -o asm/crt0.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder nasm -f elf32 asm/gdt_flush.S -o asm/gdt_flush.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder nasm -f elf32 asm/interrupts.S -o asm/interrupts.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder nasm -f elf32 asm/paging.S -o asm/paging.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder nasm -f elf32 asm/switch_task.S -o asm/switch_task.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder nasm -f elf32 asm/user_mode.S -o asm/user_mode.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/ata.c -o lib/ata.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/elf.c -o lib/elf.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/gdt.c -o lib/gdt.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/idt.c -o lib/idt.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/kernel.c -o lib/kernel.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/keyboard.c -o lib/keyboard.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/kheap.c -o lib/kheap.o
lib/kheap.c: In function 'init_kheap':
lib/kheap.c:16:30: warning: initialization of 'uint32_t' {aka 'unsigned int'} from 'void *' makes integer from pointer without a cast [-Wint-conversion]
   16 |         uint32_t phys_addr = pmm_alloc_page();
      |                              ^~~~~~~~~~~~~~
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/mbr.c -o lib/mbr.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/paging.c -o lib/paging.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/pmm.c -o lib/pmm.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/simplefs.c -o lib/simplefs.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/syscall.c -o lib/syscall.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/task.c -o lib/task.o
lib/task.c: In function 'schedule':
lib/task.c:103:33: warning: passing argument 2 of 'switch_task' discards 'volatile' qualifier from pointer target type [-Wdiscarded-qualifiers]
  103 |         switch_task(&prev->esp, &current_task->esp);
      |                                 ^~~~~~~~~~~~~~~~~~
lib/task.c:19:58: note: expected 'uint32_t *' {aka 'unsigned int *'} but argument is of type 'volatile uint32_t *' {aka 'volatile unsigned int *'}
   19 | extern void switch_task(uint32_t *current_esp, uint32_t *next_esp);
      |                                                ~~~~~~~~~~^~~~~~~~
lib/task.c: In function 'sys_fork':
lib/task.c:191:34: warning: initialization of 'uint32_t' {aka 'unsigned int'} from 'void *' makes integer from pointer without a cast [-Wint-conversion]
  191 |     uint32_t child_ustack_phys = pmm_alloc_page();
      |                                  ^~~~~~~~~~~~~~
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/timer.c -o lib/timer.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/tty.c -o lib/tty.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/utils.c -o lib/utils.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/vfs.c -o lib/vfs.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder ld -m elf_i386 -n -T linker.ld -o myos.bin asm/boot.o asm/crt0.o asm/gdt_flush.o asm/interrupts.o asm/paging.o asm/switch_task.o asm/user_mode.o lib/ata.o lib/elf.o lib/gdt.o lib/idt.o lib/kernel.o lib/keyboard.o lib/kheap.o lib/mbr.o lib/paging.o lib/pmm.o lib/simplefs.o lib/syscall.o lib/task.o lib/timer.o lib/tty.o lib/utils.o lib/vfs.o
ld: asm/crt0.o: in function `_start':
asm/crt0.S:(.text+0x0): multiple definition of `_start'; asm/boot.o:asm/boot.S:(.text+0x0): first defined here
ld: asm/crt0.o: in function `_start':
asm/crt0.S:(.text+0xd): undefined reference to `main'
ld: lib/syscall.o: in function `syscall_handler':
syscall.c:(.text+0x88): undefined reference to `sys_wait'
make: *** [myos.bin] Error 1
```

---
app.c
```c
// [Day34] 封裝 sys_fork
int sys_fork() {
    int pid;
    __asm__ volatile ("int $0x80" : "=a"(pid) : "a"(8) : "memory");
    return pid;
}

// // [Day35][新增] 封裝 sys_exec (Syscall 9)
// int sys_exec(char* filename) {
//     int ret;
//     __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(9), "b"(filename) : "memory");
//     return ret;
// }

int sys_exec(char* filename, char** argv) {
    int ret;
    // ebx 傳檔名，ecx 傳字串陣列指標
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(9), "b"(filename), "c"(argv) : "memory");
    return ret;
}

// [新增] 封裝 sys_wait (Syscall 10)
int sys_wait(int pid) {
    int ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(10), "b"(pid) : "memory");
    return ret;
}

// 系統呼叫封裝
void sys_print(char* msg) {
    __asm__ volatile ("int $0x80" : : "a"(2), "b"(msg) : "memory");
}

int sys_open(char* filename) {
    int fd;
    __asm__ volatile ("int $0x80" : "=a"(fd) : "a"(3), "b"(filename) : "memory");
    return fd;
}

int sys_read(int fd, char* buffer, int size) {
    int bytes;
    __asm__ volatile ("int $0x80" : "=a"(bytes) : "a"(4), "b"(fd), "c"(buffer), "d"(size) : "memory");
    return bytes;
}

char sys_getchar() {
    int c;
    __asm__ volatile ("int $0x80" : "=a"(c) : "a"(5) : "memory");
    return (char)c;
}

// User Space 專用的字串比對工具
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// 讀取一整行指令
void read_line(char* buffer, int max_len) {
    int i = 0;
    while (i < max_len - 1) {
        char c = sys_getchar();
        if (c == '\n') {
            break; // 使用者按下了 Enter
        } else if (c == '\b') {
            if (i > 0) i--; // 處理倒退鍵 (Backspace)
        } else {
            buffer[i++] = c;
        }
    }
    buffer[i] = '\0'; // 字串結尾
}

void sys_yield() {
    __asm__ volatile ("int $0x80" : : "a"(6) : "memory");
}

void sys_exit() {
    __asm__ volatile ("int $0x80" : : "a"(7) : "memory");
}

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
        sys_print("SimpleOS> ");

        // 讀取使用者輸入的指令
        read_line(cmd_buffer, 128);

        // 執行指令邏輯
        if (strcmp(cmd_buffer, "") == 0) {
            continue;
        }
        else if (strcmp(cmd_buffer, "help") == 0) {
            sys_print("Available commands:\n");
            sys_print("  help    - Show this message\n");
            sys_print("  cat     - Read 'hello.txt' from disk\n");
            sys_print("  about   - OS information\n");
            sys_print("  fork    - Test pure fork()\n");
            sys_print("  run     - Test fork() + exec() to spawn a new shell\n");
        }
        else if (strcmp(cmd_buffer, "about") == 0) {
            sys_print("Simple OS v1.0\nBuilt from scratch in 35 days!\n");
        }
        else if (strcmp(cmd_buffer, "cat") == 0) {
            int fd = sys_open("hello.txt");
            if (fd == -1) {
                sys_print("Error: hello.txt not found.\n");
            } else {
                char file_buf[128];
                for(int i=0; i<128; i++) file_buf[i] = 0;
                sys_read(fd, file_buf, 100);
                sys_print("--- File Content ---\n");
                sys_print(file_buf);
                sys_print("\n--------------------\n");
            }
        }
        else if (strcmp(cmd_buffer, "exit") == 0) {
            sys_print("Goodbye!\n");
            sys_exit();
        }
        else if (strcmp(cmd_buffer, "fork") == 0) {
            int pid = sys_fork();

            if (pid == 0) {
                sys_print("\n[CHILD] Hello! I am the newborn process!\n");
                sys_print("[CHILD] My work here is done, committing suicide...\n");
                sys_exit();
            } else {
                sys_print("\n[PARENT] Magic! I just created a child process!\n");
                sys_yield();
                sys_yield();
            }
        }
        // [Day35][新增] 測試 Fork-Exec 模型
        else if (strcmp(cmd_buffer, "run") == 0) {
            int pid = sys_fork();
            if (pid == 0) {
                // 準備傳遞給新程式的參數陣列 (最後必須是 0/NULL)
                char* my_args[] = {"my_app.elf", "Hello", "From", "Parent!", 0};

                sys_exec("my_app.elf", my_args);
                sys_exit();
            } else {
                sys_print("\n[PARENT] Spawned child. Waiting for it to finish...\n");
                sys_wait(pid);
                sys_print("[PARENT] Child has finished! I am back in control.\n");
            }
        }
        else {
            sys_print("Command not found: ");
            sys_print(cmd_buffer);
            sys_print("\n");
        }
    }

    return 0; // 配合 int main 的回傳值
}

```
