#!/bin/bash
echo "void main(){}" > app.c
docker run --platform linux/amd64 --rm -v $(pwd):/workspace -w /workspace os-builder gcc -m32 -ffreestanding -c app.c -o app.o
docker run --platform linux/amd64 --rm -v $(pwd):/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 app.o -o my_app.elf
