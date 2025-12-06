#!/bin/bash

echo "Testing Exec system call with current implementation..."

# Go to lab5 directory (which has filesystem support)
cd /home/202300300075/操作系統課程設計/sdu-se-os-design-nachos/code/lab5

# Format filesystem and copy test files
echo "Setting up filesystem..."
./nachos -f
./nachos -cp ../test/arch/unknown-i386-linux/bin/exec.noff exec.noff
./nachos -cp ../test/arch/unknown-i386-linux/bin/halt2.noff halt2.noff

# Test with lab5 (should work but without Exec system call)
echo "Testing with lab5 (no Exec support):"
./nachos -x exec.noff

echo "Test completed."