# 設定原始碼目錄變數，方便後續維護
SRC_DIR = src
SCRIPT_DIR = scripts

# 修改 Docker 掛載與工作目錄：掛載根目錄，但工作目錄設在 src 內
DOCKER_CMD = docker run --platform linux/amd64 --rm -v $(PWD):/workspace -w /workspace/$(SRC_DIR) os-builder

# --- 編譯參數修正：標頭檔路徑需相對於編譯時的工作目錄 (src) ---
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include
APP_CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include

# --- 路徑抓取修正：加上 $(SRC_DIR)/ 前綴 ---
C_SOURCES = $(wildcard $(SRC_DIR)/lib/*.c)
C_OBJS = $(C_SOURCES:.c=.o)
ASM_SOURCES = $(wildcard $(SRC_DIR)/asm/*.S)
ASM_OBJS = $(ASM_SOURCES:.S=.o)

LIBC_SOURCES = $(wildcard $(SRC_DIR)/libc/*.c)
LIBC_OBJS = $(LIBC_SOURCES:.c=.o)

# 核心物件檔案清單 (boot.o 必須在最前面)
OBJS = $(SRC_DIR)/asm/boot.o $(filter-out $(SRC_DIR)/asm/boot.o, $(ASM_OBJS)) $(C_OBJS)

# 定義所有的 User App 目標
APPS = $(SRC_DIR)/shell.elf $(SRC_DIR)/echo.elf $(SRC_DIR)/cat.elf \
       $(SRC_DIR)/ping.elf $(SRC_DIR)/pong.elf $(SRC_DIR)/touch.elf \
       $(SRC_DIR)/ls.elf $(SRC_DIR)/rm.elf $(SRC_DIR)/mkdir.elf

all: build-env $(APPS) myos.iso

build-env:
	docker build -t os-builder .

.PRECIOUS: %.o $(SRC_DIR)/%.o $(SRC_DIR)/libc/%.o


# -----------------------------------------------------------------------------
# Compile Rules
# -----------------------------------------------------------------------------

## Kernel
$(SRC_DIR)/asm/%.o: $(SRC_DIR)/asm/%.S
	@echo "==> 編譯 Kernel ASM: $<"
	$(DOCKER_CMD) nasm -f elf32 $(subst $(SRC_DIR)/,,$<) -o $(subst $(SRC_DIR)/,,$@)

$(SRC_DIR)/lib/%.o: $(SRC_DIR)/lib/%.c
	@echo "==> 編譯 Kernel Lib: $<"
	$(DOCKER_CMD) gcc $(CFLAGS) -c $(subst $(SRC_DIR)/,,$<) -o $(subst $(SRC_DIR)/,,$@)

## User App (libc & apps)
$(SRC_DIR)/libc/%.o: $(SRC_DIR)/libc/%.c
	@echo "==> 編譯 libc 組件: $<"
	$(DOCKER_CMD) gcc $(APP_CFLAGS) -c $(subst $(SRC_DIR)/,,$<) -o $(subst $(SRC_DIR)/,,$@)

$(SRC_DIR)/crt0.o: $(SRC_DIR)/crt0.S
	@echo "==> 編譯 crt0.S..."
	$(DOCKER_CMD) nasm -f elf32 crt0.S -o crt0.o

# 通用 User App 編譯規則 (.c -> .o)
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "==> 編譯 User App: $<"
	$(DOCKER_CMD) gcc $(APP_CFLAGS) -c $(subst $(SRC_DIR)/,,$<) -o $(subst $(SRC_DIR)/,,$@)

# 通用 User App 連結規則 (.o -> .elf)
$(SRC_DIR)/%.elf: $(SRC_DIR)/crt0.o $(SRC_DIR)/%.o $(LIBC_OBJS)
	@echo "==> 連結 User App: $@"
	$(DOCKER_CMD) ld -m elf_i386 -Ttext 0x08048000 crt0.o $(subst $(SRC_DIR)/,,$*.o) $(subst $(SRC_DIR)/,,$(LIBC_OBJS)) -o $(subst $(SRC_DIR)/,,$@)

# -----------------------------------------------------------------------------
# --- OS ISO 建置規則 ---
# -----------------------------------------------------------------------------

myos.iso: $(SRC_DIR)/myos.bin $(SCRIPT_DIR)/grub.cfg $(APPS)
	# 1. 在實體機的 src 底下建立臨時目錄 (確保 Docker 看得到)
	mkdir -p $(SRC_DIR)/isodir/boot/grub

	# 2. 複製檔案到該目錄
	cp $(SRC_DIR)/myos.bin $(SRC_DIR)/isodir/boot/myos.bin
	cp $(SCRIPT_DIR)/grub.cfg $(SRC_DIR)/isodir/boot/grub/grub.cfg
	cp $(SRC_DIR)/*.elf $(SRC_DIR)/isodir/boot/

	# 3. 執行 grub-mkrescue
	# 注意：我們的工作目錄是 /workspace/src，所以路徑直接寫 isodir 即可
	# 產出的 iso 會放在 /workspace/ (即根目錄)
	$(DOCKER_CMD) grub-mkrescue -o ../$@ isodir

	# 4. 清理臨時目錄
	rm -rf $(SRC_DIR)/isodir

$(SRC_DIR)/myos.bin: $(OBJS)
	$(DOCKER_CMD) ld -m elf_i386 -n -T ../$(SCRIPT_DIR)/linker.ld -o myos.bin $(subst $(SRC_DIR)/,,$(OBJS))

# --- 其他工具 ---
hdd.img:
	dd if=/dev/zero of=hdd.img bs=1M count=10
	docker run --rm -i --platform linux/amd64 -v $(PWD):/workspace -w /workspace alpine sh -c "apk add --no-cache util-linux && echo -e 'o\nn\np\n1\n\n\nw' | fdisk hdd.img"

clean:
	rm -f hdd.img myos.iso
	find $(SRC_DIR) -name "*.o" -delete
	find $(SRC_DIR) -name "*.elf" -delete
	find $(SRC_DIR) -name "*.bin" -delete
	rm -rf $(SRC_DIR)/isodir

run: all hdd.img
	qemu-system-i386 -cdrom myos.iso -monitor stdio -hda hdd.img -boot d

debug: all hdd.img
	qemu-system-i386 -cdrom myos.iso -d int -no-reboot -hda hdd.img -boot d
