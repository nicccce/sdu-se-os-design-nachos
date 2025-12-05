# 注意：这是一个 GNU Makefile。您必须使用 "gmake" 而不是 "make"。
#
# 文件系统作业的 Makefile
#    假设多程序和虚拟内存先完成，然后定义设置。
#    如果不是，请使用下面的"裸机"定义。
#
# Copyright (c) 1992 The Regents of the University of California.
# All rights reserved.  See copyright.h for copyright notice and limitation 
# of liability and disclaimer of warranty provisions.

ifndef MAKEFILE_FILESYS
define MAKEFILE_FILESYS
yes
endef

# 您可以重新排序分配。如果 filesys 在 userprog 之前，
# 只需重新排序并适当地注释下面的包含。

include ../threads/Makefile.local
include ../filesys/Makefile.local
#include ../userprog/Makefile.local
#include ../vm/Makefile.local
#include ../filesys/Makefile.local

include ../Makefile.dep
include ../Makefile.common

endif # MAKEFILE_FILESYS