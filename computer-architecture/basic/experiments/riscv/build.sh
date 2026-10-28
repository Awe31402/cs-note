#!/bin/bash
# build.sh prog.s [attrs] : llvm-mc 組譯（RV32IMF），ld.lld 連結到位址 0，輸出 prog.bin（原始 byte，從位址 0 開始）
set -e
export PATH=/usr/lib/llvm-14/bin:$PATH
b=${1%.s}; attr=${2:-+m,+f}
llvm-mc -triple=riscv32 -mattr=$attr -filetype=obj -o $b.o $1
ld.lld -m elf32lriscv -Ttext=0 --no-relax -e 0 -o $b.elf $b.o
llvm-objcopy -O binary $b.elf $b.bin
