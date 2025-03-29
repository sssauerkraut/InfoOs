# InfoOs
This repository contains basic OS developed as part of a second-year Operating Systems course at Peter the Great St. Petersburg Polytechnic University (SPbPU). OS insludes only loader and kernel.

# Features: *coming soon*

# Purpose:
  The lab focuses on understanding low-level OS concepts, including bootstrapping, memory     management, and processor modes. 

# Requirements:
  - YASM
  - QEMU
  - Microsoft C Compiler (if extending with C/C++ code)

YASM and QEMU can be installed by [chocolatey](https://chocolatey.org/install)
# You can install packages by running:
    choco install chocolatey-compatibility.extension
    choco install yasm
    choco install qemu
Don't forget to add qemu to PATH

# Compiling
    yasm -f bin -o boot.bin boot.asm
    cl.exe /GS- /c kernel.cpp
    link.exe /OUT:kernel.bin /BASE:0x10000 /FIXED /FILEALIGN:512 /MERGE:.rdata=.data /IGNORE:4254 /NODEFAULTLIB /ENTRY:kmain /SUBSYSTEM:NATIVE kernel.obj
    qemu-system-x86_64 –fda boot.bin –fdb kernel.bin
