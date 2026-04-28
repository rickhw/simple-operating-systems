# -----------------------------------------------------------------------------
# 共用目錄定義
# -----------------------------------------------------------------------------
SRC_DIR    = src
SCRIPT_DIR = scripts

# =============================================================================
# HOST MODE（預設，在本機執行）
# 只負責啟動單一 container，編譯全部在裡面進行
# =============================================================================
ifeq ($(IN_CONTAINER),)

.PHONY: all build-env clean run debug

all: build-env
	docker run --rm \
		-v $(CURDIR):/workspace \
		-w /workspace \
		os-builder \
		make IN_CONTAINER=1

# 若 Dockerfile 有異動，先執行 `docker rmi os-builder` 再重新 build
build-env:
	docker image inspect os-builder >/dev/null 2>&1 || \
		docker build --platform linux/amd64 --load -t os-builder .

clean:
	rm -f hdd.img simple-os.iso
	find $(SRC_DIR) -name "*.o" -delete
	find $(SRC_DIR) -name "*.elf" -delete
	find $(SRC_DIR) -name "*.bin" -delete
	rm -f *.pcap
	rm -rf $(SRC_DIR)/isodir

hdd.img:
	dd if=/dev/zero of=hdd.img bs=1M count=10
	docker run --rm -i -v $(CURDIR):/workspace -w /workspace alpine \
		sh -c "apk add --no-cache util-linux && echo -e 'o\nn\np\n1\n\n\nw' | fdisk hdd.img"

run: all hdd.img
	@echo "==> 寫入網路封包到 dump.pcap"
	qemu-system-i386 -cdrom simple-os.iso -monitor stdio \
		-netdev user,id=n0,hostfwd=udp::8080-:8080 \
		-device rtl8139,netdev=n0 \
		-object filter-dump,id=f1,netdev=n0,file=dump.pcap \
		-hda hdd.img \
		-boot d

debug: all hdd.img
	qemu-system-i386 -cdrom simple-os.iso \
		-d int -no-reboot \
		-netdev user,id=n0,hostfwd=udp::8080-:8080 \
		-device rtl8139,netdev=n0 \
		-object filter-dump,id=f1,netdev=n0,file=dump.pcap \
		-hda hdd.img \
		-boot d

else
# =============================================================================
# CONTAINER MODE（IN_CONTAINER=1，在 container 內執行，CWD = /workspace）
# 直接呼叫 gcc / nasm / ld / grub-mkrescue，不再有 docker run
# =============================================================================

KERNEL_DIR   = $(SRC_DIR)/kernel
USER_DIR     = $(SRC_DIR)/user
USER_ASM_DIR = $(USER_DIR)/asm
USER_LIB_DIR = $(USER_DIR)/lib
USER_BIN_DIR = $(USER_DIR)/bin

KERNEL_INC = -I$(KERNEL_DIR)/include -I$(KERNEL_DIR)/lib
USER_INC   = -I$(USER_DIR)/include -I$(USER_DIR)/lib

CFLAGS     = -m32 -ffreestanding -O2 -Wall -Wextra $(KERNEL_INC)
APP_CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector $(USER_INC)

# -----------------------------------------------------------------------------
# 檔案選取
# -----------------------------------------------------------------------------

# [Kernel]
ALL_KERNEL_C_SOURCES = $(shell find $(KERNEL_DIR) -name "*.c")
ALL_KERNEL_S_SOURCES = $(shell find $(KERNEL_DIR) -name "*.S")

C_OBJS   = $(ALL_KERNEL_C_SOURCES:.c=.o)
ASM_OBJS = $(ALL_KERNEL_S_SOURCES:.S=.o)

BOOT_OBJ = $(KERNEL_DIR)/arch/x86/boot.o
OBJS     = $(BOOT_OBJ) $(filter-out $(BOOT_OBJ), $(ASM_OBJS) $(C_OBJS))

# [User]
CRT0_OBJ      = $(USER_ASM_DIR)/crt0.o
USER_LIB_SRCS = $(wildcard $(USER_LIB_DIR)/*.c)
USER_LIB_OBJS = $(USER_LIB_SRCS:.c=.o)
USER_APPS_C   = $(wildcard $(USER_BIN_DIR)/*.c)
APPS          = $(USER_APPS_C:.c=.elf)

# -----------------------------------------------------------------------------
# 主要目標
# -----------------------------------------------------------------------------
.PHONY: all
.PRECIOUS: $(KERNEL_DIR)/%.o \
           $(USER_ASM_DIR)/%.o $(USER_LIB_DIR)/%.o $(USER_BIN_DIR)/%.o

all: $(APPS) simple-os.iso

# -----------------------------------------------------------------------------
# Compile Rules
# -----------------------------------------------------------------------------

# [Kernel] ASM
$(KERNEL_DIR)/%.o: $(KERNEL_DIR)/%.S
	@echo "==> 編譯 Kernel ASM: $<"
	nasm -f elf32 $< -o $@

# [Kernel] C
$(KERNEL_DIR)/%.o: $(KERNEL_DIR)/%.c
	@echo "==> 編譯 Kernel C: $<"
	gcc $(CFLAGS) -c $< -o $@

# [User] CRT0
$(USER_ASM_DIR)/%.o: $(USER_ASM_DIR)/%.S
	@echo "==> 編譯 User CRT0: $<"
	nasm -f elf32 $< -o $@

# [User] Lib
$(USER_LIB_DIR)/%.o: $(USER_LIB_DIR)/%.c
	@echo "==> 編譯 User Lib: $<"
	gcc $(APP_CFLAGS) -c $< -o $@

# [User] App
$(USER_BIN_DIR)/%.o: $(USER_BIN_DIR)/%.c
	@echo "==> 編譯 User App: $<"
	gcc $(APP_CFLAGS) -c $< -o $@

# -----------------------------------------------------------------------------
# Link Rules
# -----------------------------------------------------------------------------

# [User] 連結
$(USER_BIN_DIR)/%.elf: $(CRT0_OBJ) $(USER_BIN_DIR)/%.o $(USER_LIB_OBJS)
	@echo "==> 連結 User App: $@"
	ld -m elf_i386 -Ttext 0x08048000 \
		$(CRT0_OBJ) \
		$(USER_BIN_DIR)/$*.o \
		$(USER_LIB_OBJS) \
		-o $@

# [Kernel] Binary
$(SRC_DIR)/myos.bin: $(OBJS)
	ld -m elf_i386 -n -T $(SCRIPT_DIR)/linker.ld -o $@ $(OBJS)

# -----------------------------------------------------------------------------
# ISO（輸出到 /workspace/simple-os.iso，即 host 的 project root）
# -----------------------------------------------------------------------------
simple-os.iso: $(SRC_DIR)/myos.bin $(APPS)
	mkdir -p $(SRC_DIR)/isodir/boot/grub
	cp $(SRC_DIR)/myos.bin          $(SRC_DIR)/isodir/boot/myos.bin
	cp $(SCRIPT_DIR)/grub.cfg       $(SRC_DIR)/isodir/boot/grub/grub.cfg
	cp $(USER_BIN_DIR)/*.elf        $(SRC_DIR)/isodir/boot/
	cp $(USER_BIN_DIR)/*.bmp        $(SRC_DIR)/isodir/boot/
	grub-mkrescue -o simple-os.iso $(SRC_DIR)/isodir
	rm -rf $(SRC_DIR)/isodir

endif
