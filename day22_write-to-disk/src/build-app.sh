#!/bin/bash

# 1. 編譯獨立的 User App
docker run --platform linux/amd64 --rm -v $(pwd):/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -c app.c -o app.o

# 2. 連結 App，並強制規定它要在 0x08048000 執行
docker run --platform linux/amd64 --rm -v $(pwd):/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 app.o -o my_app.elf
