// filesys.h 
//	表示 Nachos 文件系统的数据结构。
//
//	文件系统是存储在磁盘上的一组文件，组织成目录。
//	文件系统的操作与"命名"有关 -- 创建、打开和删除文件，
//	给定文本文件名。对单个"打开"文件的操作（读、写、关闭）
//	可以在 OpenFile 类（openfile.h）中找到。
//
//	我们定义了文件系统的两个单独实现。
//	"STUB" 版本只是将 Nachos 文件系统操作重新定义为
//	运行 Nachos 模拟的机器上的本机 UNIX 文件系统上的操作。
//	提供这是为了以防多程序和虚拟内存作业（使用文件系统）
//	在文件系统作业之前完成。
//
//	另一个版本是"真实"文件系统，建立在磁盘模拟器之上。
//	磁盘使用本机 UNIX 文件系统模拟（在名为 "DISK" 的文件中）。
//
//	在"真实"实现中，文件系统中使用两个关键数据结构。
//	有一个单独的"根"目录，列出文件系统中的所有文件；
//	与 UNIX 不同，基线系统不提供分层目录结构。
//	此外，有一个用于分配磁盘扇区的位图。
//	根目录和位图本身都作为文件存储在 Nachos 文件系统中 --
//	这导致在初始化模拟磁盘时出现一个有趣的引导问题。
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef FS_H
#define FS_H

#include "copyright.h"
#include "openfile.h"

// STUB版本：临时实现，直接使用宿主机的UNIX文件系统
// 用于在完成文件系统作业之前进行多程序和虚拟内存作业
#ifdef FILESYS_STUB 		
class FileSystem {
  public:
    // 构造函数：初始化文件系统
    FileSystem(bool format) {}

    // 创建文件：使用宿主机系统调用
    bool Create(char *name, int initialSize) { 
	int fileDescriptor = OpenForWrite(name);

	if (fileDescriptor == -1) return FALSE;
	Close(fileDescriptor); 
	return TRUE; 
	}

    // 打开文件：使用宿主机系统调用
    OpenFile* Open(char *name) {
	  int fileDescriptor = OpenForReadWrite(name, FALSE);

	  if (fileDescriptor == -1) return NULL;
	  return new OpenFile(fileDescriptor);
      }

    // 删除文件：使用宿主机系统调用
    bool Remove(char *name) { return (bool)(Unlink(name) == 0); }

};

#else // FILESYS

// 真实文件系统实现：基于磁盘模拟器的完整文件系统
// 这是Nachos文件系统的完整实现版本
class FileSystem {
  public:
    // 构造函数：初始化文件系统
    // 参数：format - 是否格式化磁盘(TRUE=清空磁盘并创建新的文件系统结构，FALSE=加载现有文件系统)
    // 注意：必须在synchDisk初始化之后调用
    // 如果format为TRUE，会初始化磁盘以包含空目录和空闲块位图
    FileSystem(bool format);		

    // 创建新文件
    // 参数：name - 文件名，initialSize - 文件初始大小(字节)
    // 注意：文件大小在创建后固定不变
    // 返回值：成功返回TRUE，失败返回FALSE
    bool Create(char *name, int initialSize);  	
    				

    // 打开已存在的文件
    // 参数：name - 要打开的文件名
    // 返回值：成功返回OpenFile对象指针，失败返回NULL
    OpenFile* Open(char *name); 	

    // 删除文件
    // 参数：name - 要删除的文件名
    // 操作：从目录中移除条目，释放文件头和数据块空间
    // 返回值：成功返回TRUE，失败返回FALSE
    bool Remove(char *name);  		

    // 列出目录中的所有文件名
    // 输出：每个文件名占一行
    void List();			

    // 打印文件系统的完整信息(调试用)
    // 输出：位图、目录、每个文件的文件头和内容
    void Print();			

  private:
   OpenFile* freeMapFile;		// 空闲磁盘块位图文件的句柄
					// 存储扇区使用状态信息
   OpenFile* directoryFile;		// 根目录文件的句柄
					// 存储文件名到文件头扇区号的映射
   				
   // 文件系统磁盘布局：
   // 扇区0：位图文件头
   // 扇区1：目录文件头
   // 其他扇区：文件数据和文件头
   // 
   // 文件系统操作流程：
   // 1. 创建：检查文件名存在 -> 分配文件头扇区 -> 分配数据空间 -> 添加目录条目 -> 写回磁盘
   // 2. 打开：查找目录 -> 获取文件头扇区 -> 创建OpenFile对象
   // 3. 删除：查找目录 -> 释放数据块 -> 释放文件头扇区 -> 移除目录条目 -> 写回磁盘
};

#endif // FILESYS

#endif // FS_H