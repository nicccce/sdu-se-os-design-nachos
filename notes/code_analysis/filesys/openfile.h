// openfile.h 
//	用于打开、关闭、读取和写入
//	单个文件的数据结构。支持的操作类似于
//	UNIX -- 在UNIX提示符下输入 'man open'。
//
//	有两种实现。一种是"STUB"，直接
//	将文件操作转换为底层UNIX操作。
//	(参见 filesys.h 中的注释)。
//
//	另一种是"真实"实现，将这些
//	操作转换为读取和写入磁盘扇区请求。
//	在此文件系统的基线实现中，我们不
//	担心不同线程对文件系统的并发访问
//	-- 这是作业的一部分。
//
// 版权所有 (c) 1992-1993 加州大学董事会。
// 保留所有权利。请参阅 copyright.h 以获得版权公告和责任限制 
// 以及保修声明条款。

#ifndef OPENFILE_H
#define OPENFILE_H

#include "copyright.h"
#include "utility.h"

#ifdef FILESYS_STUB			// 临时将对
					// Nachos 文件系统的调用实现为对 UNIX 的调用！
					// 参见 #else 下列出的定义
class OpenFile {
  public:
    OpenFile(int f) { file = f; currentOffset = 0; }	// 打开文件
    ~OpenFile() { Close(file); }			// 关闭文件

    int ReadAt(char *into, int numBytes, int position) { 
    		Lseek(file, position, 0); 
		return ReadPartial(file, into, numBytes); 
		}	
    int WriteAt(char *from, int numBytes, int position) { 
    		Lseek(file, position, 0); 
		WriteFile(file, from, numBytes); 
		return numBytes;
		}	
    int Read(char *into, int numBytes) {
		int numRead = ReadAt(into, numBytes, currentOffset); 
		currentOffset += numRead;
		return numRead;
    		}
    int Write(char *from, int numBytes) {
		int numWritten = WriteAt(from, numBytes, currentOffset); 
		currentOffset += numWritten;
		return numWritten;
		}

    int Length() { Lseek(file, 0, 2); return Tell(file); }
    
  private:
    int file;
    int currentOffset;
};

#else // FILESYS
class FileHeader;

class OpenFile {
  public:
    OpenFile(int sector);		// 打开一个文件，其头部位于
					// 磁盘上的 "sector"
    ~OpenFile();			// 关闭文件

    void Seek(int position); 		// 设置从哪里开始
					// 读取/写入 -- UNIX lseek

    int Read(char *into, int numBytes); // 从文件读取/写入字节，
					// 从隐式位置开始。
					// 返回实际读取/写入的数量，
					// 并在文件中增加位置。
    int Write(char *from, int numBytes);

    int ReadAt(char *into, int numBytes, int position);
    					// 从文件读取/写入字节，
					// 绕过隐式位置。
    int WriteAt(char *from, int numBytes, int position);

    int Length(); 			// 返回文件中的字节数
					// (此接口比
					// UNIX 习惯更简单 -- lseek 到
					// 文件末尾，tell，lseek 回来
    
  private:
    FileHeader *hdr;			// 此文件的头部
    int seekPosition;			// 文件中的当前位置
};

#endif // FILESYS

#endif // OPENFILE_H