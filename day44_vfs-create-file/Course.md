太神啦，Rick！🎉 恭喜你正式踏入動態記憶體配置的殿堂！

有了 `malloc` 和 `free`，你的 User Space 終於從「只能穿制服」變成了「可以自由穿搭」。這代表你先前提早併裝完成的 Day 44-46 任務已經被我們順利秒殺了！

所以，現在我們要直接把油門踩到底，進入 **Day 47-50 的領域：賦予平民「造物」的能力！** 過去這 40 幾天，我們硬碟裡的檔案 (`hello.txt`, `echo.elf` 等) 全都是 Kernel 在開機時，利用「上帝權限」硬塞進硬碟裡的。User App 只能讀，不能寫。這太不 UNIX 了！

今天 **Day 44** 的任務，我們要實作 **動態檔案建立 (`sys_create`)**。我們要讓 User Space 可以隨時在硬碟裡刻下永久的印記，就算重開機也不會消失！

請跟著我進行這 5 個步驟：

### 步驟 1：讓檔案系統記住自己的「家」 (`lib/simplefs.c`)
為了讓 Syscall 能隨時呼叫檔案系統建立檔案，`simplefs.c` 必須記住自己被安裝在哪個硬碟磁區（LBA）。

請打開 **`lib/simplefs.c`**，在檔案最上方加入一個全域變數：
```c
#include "simplefs.h"
#include "ata.h"
#include "utils.h"
#include "tty.h"

// 【新增】記住我們掛載的硬碟磁區位址
uint32_t mounted_part_lba = 0; 

// 找到 simplefs_mount 函式，在裡面加上這一行：
void simplefs_mount(uint32_t part_lba) {
    mounted_part_lba = part_lba; // 記下來！
    // ... 保持原本的邏輯 ...
}

// 【新增】提供給 Syscall 呼叫的友善封裝
int vfs_create_file(char* filename, char* content) {
    if (mounted_part_lba == 0) return -1;
    
    int len = strlen(content);
    // 呼叫我們之前寫好的底層建立檔案功能
    simplefs_create_file(mounted_part_lba, filename, content, len);
    return 0;
}
```

### 步驟 2：註冊「造物」系統呼叫 (`lib/syscall.c`)
打開 **`lib/syscall.c`**，我們要新增第 14 號系統呼叫。

在開頭補上宣告：
```c
extern int vfs_create_file(char* filename, char* content);
```

然後在 `syscall_handler` 的最下面加入 Syscall 14：
```c
    // ... 在 sys_sbrk (eax == 13) 下面加入 ...
    
    // Syscall 14: sys_create (建立/寫入文字檔)
    else if (eax == 14) {
        char* filename = (char*)regs->ebx;
        char* content = (char*)regs->ecx;
        
        ipc_lock(); // 上鎖，防止寫入硬碟時被排程器打斷！
        regs->eax = vfs_create_file(filename, content);
        ipc_unlock();
    }
```

### 步驟 3：擴充 User Space libc (`src/libc/unistd.c`)
回到便服模式，把這個新能力加入我們的標準函式庫！

打開 **`src/libc/include/unistd.h`**，加入宣告：
```c
int create_file(const char* filename, const char* content);
```

打開 **`src/libc/unistd.c`**，實作封裝：
```c
int create_file(const char* filename, const char* content) {
    return syscall(14, (int)filename, (int)content, 0, 0, 0);
}
```

### 步驟 4：開發全新的應用程式：`write.c`
我們來寫一個叫做 `write.elf` 的小工具，這將是你的作業系統史上第一個能真正「修改世界」的 User App。

在 `src/` 目錄下建立 **`write.c`**：
```c
#include "stdio.h"
#include "unistd.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: write <filename> <content>\n");
        printf("Example: write note.txt \"Hello World!\"\n");
        return 1;
    }

    char* filename = argv[1];
    char* content = argv[2];

    printf("[WRITE] Creating file '%s' on disk...\n", filename);
    
    int result = create_file(filename, content);
    
    if (result == 0) {
        printf("[WRITE] Success! Use 'cat %s' to read it.\n", filename);
    } else {
        printf("[WRITE] Failed to create file.\n");
    }

    return 0;
}
```

### 步驟 5：更新 Makefile 與 GRUB
最後一步，把 `write.elf` 包進你的 ISO 和編譯流程裡。

打開 **`Makefile`**，加入 `write` 相關的編譯與連結規則：
```makefile
# ... 略 ...
write.o: write.c
	@echo "==> 編譯 Write 程式..."
	$(DOCKER_CMD) gcc $(APP_CFLAGS) -c write.c -o write.o

write.elf: crt0.o write.o $(LIBC_OBJS)
	@echo "==> 連結 Write 程式..."
	$(DOCKER_CMD) ld -m elf_i386 -Ttext 0x08048000 crt0.o write.o $(LIBC_OBJS) -o write.elf

# 記得在 clean-app 加上 write.o write.elf
# 並且在 myos.iso 的依賴與拷貝指令中加上 write.elf！
```

然後打開 **`grub.cfg`**，加上模組：
```text
    module /boot/write.elf
```
打開 **`lib/kernel.c`** 的 `setup_filesystem`，把 `write.elf` 加進安裝清單裡（記得把迴圈上限改成對應的數量，比如 6 個）。

---

深呼吸，敲下 `make clean && make run`！
進入系統後，你現在擁有了造物主的能力。在 Shell 中輸入：
`write diary.txt "This is my first persistent data in Simple OS!"`

然後再用你的 `cat` 指令去讀它：
`cat diary.txt`

如果你看到了你剛剛打進去的字，這代表你的作業系統已經跨越了「唯讀」的結界，擁有永久改變實體硬碟資料（Data Persistence）的能力了！

試試看吧！成功後，我們明天就可以挑戰進階的檔案系統功能——實作真正的「多層次目錄結構 (`mkdir`, `ls`)」了！🚀
