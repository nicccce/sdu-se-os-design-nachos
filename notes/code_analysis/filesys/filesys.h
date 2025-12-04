// filesys.h 
//	表示Nachos文件系统的数据结构。
//
//	文件系统是一组存储在磁盘上的文件，组织
//	成目录。文件系统的操作必须
//	处理"命名" -- 创建、打开和删除文件，
//	给定一个文本文件名。对单个
//	"打开"文件的操作（读取、写入、关闭）可以在OpenFile
//	类（openfile.h）中找到。
//
//	我们定义了文件系统的两个独立实现。
//	"STUB"版本只是重新定义了Nachos文件系统
//	操作为运行Nachos模拟的机器上的
//	本机UNIX文件系统操作。这是在
//	多程序和虚拟内存作业（使用
//	文件系统）在文件系统作业之前完成的情况下提供的。
//
//	另一个版本是"真实"的文件系统，建立在
//	磁盘模拟器之上。磁盘使用本机UNIX
//	文件系统模拟（在一个名为"DISK"的文件中）。
//
//	在"真实"实现中，文件系统中使用了两个关键数据结构。
//	有一个单独的"根"目录，列出
//	文件系统中的所有文件；与UNIX不同，
//	基线系统不提供分层目录结构。
//	此外，还有一个用于分配
//	磁盘扇区的位图。根目录和位图本身
//	都作为文件存储在Nachos文件系统中 -- 这在初始化模拟磁盘时
//	引起了一个有趣的引导问题。
//
// 版权所有 (c) 1992-1993 加州大学董事会。
// 保留所有权利。请参阅 copyright.h 以获得版权公告和责任限制 
// 以及保修声明条款。

#ifndef FS_H
#define FS_H

#include "copyright.h"
#include "openfile.h"

#ifdef FILESYS_STUB 		// 临时将文件系统调用实现为
				// 对UNIX的调用，直到真实文件系统
				// 实现可用
class FileSystem {
  public:
    FileSystem(bool format) {}

    bool Create(char *name, int initialSize) { 
	int fileDescriptor = OpenForWrite(name);

	if (fileDescriptor == -1) return FALSE;
	Close(fileDescriptor); 
	return TRUE; 
	}

    OpenFile* Open(char *name) {
	  int fileDescriptor = OpenForReadWrite(name, FALSE);

	  if (fileDescriptor == -1) return NULL;
	  return new OpenFile(fileDescriptor);
      }

    bool Remove(char *name) { return (bool)(Unlink(name) == 0); }

};

#else // FILESYS
class FileSystem {
  public:
    FileSystem(bool format);		// 初始化文件系统。
					// 必须在"synchDisk"
					// 初始化*之后*调用。
    					// 如果"format"，磁盘上没有
					// 任何内容，所以初始化目录
    					// 和空闲块位图。

    bool Create(char *name, int initialSize);  	
					// 创建一个文件（UNIX creat）

    OpenFile* Open(char *name); 	// 打开一个文件（UNIX open）

    bool Remove(char *name);  		// 删除一个文件（UNIX unlink）

    void List();			// 列出文件系统中的所有文件

    void Print();			// 列出所有文件及其内容

  private:
   OpenFile* freeMapFile;		// 空闲磁盘块的位图，
					// 表示为一个文件
   OpenFile* directoryFile;		// "根"目录 -- 文件名列表，
					// 表示为一个文件
};

#endif // FILESYS

#endif // FS_H