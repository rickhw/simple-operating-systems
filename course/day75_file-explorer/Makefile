# -----------------------------------------------------------------------------
# 目錄定義
# -----------------------------------------------------------------------------
SRC_DIR     = src
SCRIPT_DIR  = scripts

# Kernel 相關目錄
KERNEL_DIR     = $(SRC_DIR)/kernel
KERNEL_ASM_DIR = $(KERNEL_DIR)/asm
KERNEL_LIB_DIR = $(KERNEL_DIR)/lib
KERNEL_INC     = -Ikernel/include -Ikernel/lib

# User 相關目錄
USER_DIR     = $(SRC_DIR)/user
USER_ASM_DIR = $(USER_DIR)/asm
USER_LIB_DIR = $(USER_DIR)/lib
USER_BIN_DIR = $(USER_DIR)/bin
USER_INC     = -Iuser/include -Iuser/lib

# Docker 指令
DOCKER_CMD = docker run --platform linux/amd64 --rm -v $(PWD):/workspace -w /workspace/$(SRC_DIR) os-builder


# -----------------------------------------------------------------------------
# 編譯參數
# -----------------------------------------------------------------------------
CFLAGS     = -m32 -ffreestanding -O2 -Wall -Wextra $(KERNEL_INC)
APP_CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector $(USER_INC)

# -----------------------------------------------------------------------------
# 檔案選取
# -----------------------------------------------------------------------------

# [Kernel]
C_LIB_SOURCES  = $(wildcard $(KERNEL_LIB_DIR)/*.c)
C_CORE_SOURCES = $(wildcard $(KERNEL_DIR)/*.c)
C_OBJS         = $(C_LIB_SOURCES:.c=.o) $(C_CORE_SOURCES:.c=.o)
ASM_SOURCES    = $(wildcard $(KERNEL_ASM_DIR)/*.S)
ASM_OBJS       = $(ASM_SOURCES:.S=.o)

OBJS = $(KERNEL_ASM_DIR)/boot.o $(filter-out $(KERNEL_ASM_DIR)/boot.o, $(ASM_OBJS)) $(C_OBJS)

# [User]
CRT0_OBJ      = $(USER_ASM_DIR)/crt0.o
USER_LIB_SRCS = $(wildcard $(USER_LIB_DIR)/*.c)
USER_LIB_OBJS = $(USER_LIB_SRCS:.c=.o)

# 【修正】指向新的 bin 子目錄
USER_APPS_C   = $(wildcard $(USER_BIN_DIR)/*.c)
APPS          = $(USER_APPS_C:.c=.elf)

# -----------------------------------------------------------------------------
# 主要目標
# -----------------------------------------------------------------------------
all: build-env $(APPS) myos.iso

build-env:
	docker build -t os-builder .

# 防止 Make 自動刪除 .o 檔，加入新的 bin 目錄路徑
.PRECIOUS: %.o $(SRC_DIR)/%.o $(KERNEL_DIR)/%.o $(USER_DIR)/%.o $(USER_BIN_DIR)/%.o

# -----------------------------------------------------------------------------
# Compile Rules
# -----------------------------------------------------------------------------

## [Kernel 編譯]
$(KERNEL_ASM_DIR)/%.o: $(KERNEL_ASM_DIR)/%.S
	@echo "==> 編譯 Kernel ASM: $<"
	$(DOCKER_CMD) nasm -f elf32 $(subst $(SRC_DIR)/,,$<) -o $(subst $(SRC_DIR)/,,$@)

$(KERNEL_LIB_DIR)/%.o: $(KERNEL_LIB_DIR)/%.c
	@echo "==> 編譯 Kernel Lib: $<"
	$(DOCKER_CMD) gcc $(CFLAGS) -c $(subst $(SRC_DIR)/,,$<) -o $(subst $(SRC_DIR)/,,$@)

$(KERNEL_DIR)/%.o: $(KERNEL_DIR)/%.c
	@echo "==> 編譯 Kernel Core: $<"
	$(DOCKER_CMD) gcc $(CFLAGS) -c $(subst $(SRC_DIR)/,,$<) -o $(subst $(SRC_DIR)/,,$@)

## [User 編譯]
# 1. crt0 (user/asm)
$(USER_ASM_DIR)/%.o: $(USER_ASM_DIR)/%.S
	@echo "==> 編譯 User CRT0: $<"
	$(DOCKER_CMD) nasm -f elf32 $(subst $(SRC_DIR)/,,$<) -o $(subst $(SRC_DIR)/,,$@)

# 2. User Lib (user/lib)
$(USER_LIB_DIR)/%.o: $(USER_LIB_DIR)/%.c
	@echo "==> 編譯 User Lib: $<"
	$(DOCKER_CMD) gcc $(APP_CFLAGS) -c $(subst $(SRC_DIR)/,,$<) -o $(subst $(SRC_DIR)/,,$@)

# 3. User App 在 user/bin 目錄下的編譯規則
$(USER_BIN_DIR)/%.o: $(USER_BIN_DIR)/%.c
	@echo "==> 編譯 User App (bin): $<"
	$(DOCKER_CMD) gcc $(APP_CFLAGS) -c $(subst $(SRC_DIR)/,,$<) -o $(subst $(SRC_DIR)/,,$@)

# -----------------------------------------------------------------------------
# Link Rules
# -----------------------------------------------------------------------------

# 通用 User App 連結規則
$(USER_BIN_DIR)/%.elf: $(CRT0_OBJ) $(USER_BIN_DIR)/%.o $(USER_LIB_OBJS)
	@echo "==> 連結 User App: $@"
	$(DOCKER_CMD) ld -m elf_i386 -Ttext 0x08048000 \
		$(subst $(SRC_DIR)/,,$(CRT0_OBJ)) \
		$(subst $(SRC_DIR)/,,$(USER_BIN_DIR)/$*.o) \
		$(subst $(SRC_DIR)/,,$(USER_LIB_OBJS)) \
		-o $(subst $(SRC_DIR)/,,$@)

# -----------------------------------------------------------------------------
# ISO & Bin
# -----------------------------------------------------------------------------

myos.iso: $(SRC_DIR)/myos.bin $(SCRIPT_DIR)/grub.cfg $(APPS)
	mkdir -p $(SRC_DIR)/isodir/boot/grub
	cp $(SRC_DIR)/myos.bin $(SRC_DIR)/isodir/boot/myos.bin
	cp $(SCRIPT_DIR)/grub.cfg $(SRC_DIR)/isodir/boot/grub/grub.cfg
	cp $(USER_BIN_DIR)/*.elf $(SRC_DIR)/isodir/boot/
	cp $(USER_BIN_DIR)/*.bmp $(SRC_DIR)/isodir/boot/
	$(DOCKER_CMD) grub-mkrescue -o ../$@ isodir
	rm -rf $(SRC_DIR)/isodir

$(SRC_DIR)/myos.bin: $(OBJS)
	$(DOCKER_CMD) ld -m elf_i386 -n -T ../$(SCRIPT_DIR)/linker.ld -o myos.bin $(subst $(SRC_DIR)/,,$(OBJS))

# -----------------------------------------------------------------------------
# 工具
# -----------------------------------------------------------------------------
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
