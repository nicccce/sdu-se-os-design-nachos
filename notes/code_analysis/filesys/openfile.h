// openfile.h 
//	用于打开、关闭、读取和写入单个文件的数据结构。
//	支持的操作类似于 UNIX 的操作 -- 在 UNIX 提示符下键入 'man open'。
//
//	有两种实现。一种是"STUB"，直接将文件操作转换为底层的 UNIX 操作。
//	（参见 filesys.h 中的注释）。
//
//	另一种是"真实"实现，将这些操作转换为读写磁盘扇区请求。
//	在文件系统的这个基线实现中，我们不担心不同线程对文件系统的
//	并发访问 -- 这是作业的一部分。
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef OPENFILE_H
#define OPENFILE_H

#include "copyright.h"
#include "utility.h"

// STUB版本：使用宿主机的UNIX文件系统
// 用于在完成文件系统作业之前进行其他作业
#ifdef FILESYS_STUB			
class OpenFile {
  public:
    // 构造函数：使用已打开的文件描述符
    OpenFile(int f) { file = f; currentOffset = 0; }
    
    // 析构函数：关闭文件
    ~OpenFile() { Close(file); }

    // 在指定位置读取数据
    int ReadAt(char *into, int numBytes, int position) { 
    		Lseek(file, position, 0); 
		return ReadPartial(file, into, numBytes); 
		}	
    
    // 在指定位置写入数据
    int WriteAt(char *from, int numBytes, int position) { 
    		Lseek(file, position, 0); 
		WriteFile(file, from, numBytes); 
		return numBytes;
		}	
    
    // 从当前位置读取数据（顺序读取）
    int Read(char *into, int numBytes) {
		int numRead = ReadAt(into, numBytes, currentOffset); 
		currentOffset += numRead;
		return numRead;
    		}
    
    // 从当前位置写入数据（顺序写入）
    int Write(char *from, int numBytes) {
		int numWritten = WriteAt(from, numBytes, currentOffset); 
		currentOffset += numWritten;
		return numWritten;
		}

    // 获取文件长度
    int Length() { Lseek(file, 0, 2); return Tell(file); }
    
  private:
    int file;			// UNIX文件描述符
    int currentOffset;		// 当前文件位置指针
};

#else // FILESYS

// 声明FileHeader类，用于访问文件元数据
class FileHeader;

// 真实文件系统中的OpenFile类实现
// 提供对Nachos文件系统的文件访问接口
class OpenFile {
  public:
    // 构造函数：打开位于磁盘扇区sector的文件
    // 参数：sector - 文件头所在的磁盘扇区号
    // 操作：从磁盘加载文件头到内存，初始化位置指针为0
    OpenFile(int sector);		

    // 析构函数：关闭文件
    // 操作：释放内存中的文件头对象
    ~OpenFile();			

    // 设置文件读写位置
    // 参数：position - 新的读写位置
    // 功能：类似UNIX的lseek系统调用
    void Seek(int position); 	

    // 从当前位置顺序读取数据
    // 参数：into - 目标缓冲区，numBytes - 要读取的字节数
    // 返回：实际读取的字节数
    // 功能：从当前seekPosition开始读取，并更新位置指针
    int Read(char *into, int numBytes); 
    
    // 从当前位置顺序写入数据
    // 参数：from - 源缓冲区，numBytes - 要写入的字节数
    // 返回：实际写入的字节数
    // 功能：在当前seekPosition开始写入，并更新位置指针
    int Write(char *from, int numBytes);

    // 在指定位置读取数据
    // 参数：into - 目标缓冲区，numBytes - 要读取的字节数，position - 读取位置
    // 返回：实际读取的字节数
    // 功能：在指定位置读取，不影响当前位置指针
    int ReadAt(char *into, int numBytes, int position);
    				

    // 在指定位置写入数据
    // 参数：from - 源缓冲区，numBytes - 要写入的字节数，position - 写入位置
    // 返回：实际写入的字节数
    // 功能：在指定位置写入，不影响当前位置指针
    int WriteAt(char *from, int numBytes, int position);

    // 获取文件长度
    // 返回：文件中的字节数
    // 功能：返回文件的实际大小(逻辑大小)
    int Length(); 			
    
  private:
    FileHeader *hdr;		// 文件头对象指针，包含文件元数据(大小、扇区位置等)
    int seekPosition;		// 当前文件读写位置指针，用于顺序读写操作
    
    // OpenFile类设计要点：
    // 1. 封装了底层扇区管理的复杂性
    // 2. 提供类似UNIX的文件操作接口
    // 3. 透明处理扇区边界对齐问题
    // 4. 维护文件指针位置，支持顺序和随机访问
    // 5. 通过synchDisk确保线程安全的磁盘访问
};

#endif // FILESYS

#endif // OPENFILE_H