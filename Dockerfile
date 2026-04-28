FROM debian:bullseye

RUN apt-get update && apt-get install -y \
    nasm \
    build-essential \
    grub-pc-bin \
    grub-common \
    xorriso \
    mtools \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
