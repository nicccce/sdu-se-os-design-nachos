// filehdr.h 
//	管理磁盘文件头的数据结构。
//
//	文件头描述在磁盘上哪里可以找到文件中的数据，
//	以及关于文件的其他信息（例如，它的长度、所有者等）
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef FILEHDR_H
#define FILEHDR_H

#include "disk.h"
#include "bitmap.h"

// 直接指针数量计算公式
// 从扇区大小减去两个int字段(numBytes和numSectors)后，
// 剩余空间用于存储数据块指针
#define NumDirect 	(int)((SectorSize - 2 * sizeof(int)) / sizeof(int))

// 最大文件大小计算
// 等于直接指针数量乘以扇区大小
// 例如：如果SectorSize=128，则NumDirect=30，MaxFileSize=3840字节
#define MaxFileSize 	(NumDirect * SectorSize)

// 下面的类定义了 Nachos "文件头"（用 UNIX 术语来说是 "i-node"），
// 描述在磁盘上哪里可以找到文件中的所有数据。
// 文件头被组织为指向数据块的简单指针表。
//
// 文件头数据结构可以存储在内存中或磁盘上。
// 当它在磁盘上时，它存储在单个扇区中 -- 这意味着
// 我们假设这个数据结构的大小与一个磁盘扇区相同。
// 没有间接寻址，这限制了最大文件长度略低于 4K 字节。
//
// 没有构造函数；相反，文件头可以通过为文件分配块来初始化
//（如果它是新文件），或者通过从磁盘读取它。
//
// 文件头设计特点：
// 1. 固定大小：恰好占用一个磁盘扇区，简化I/O操作
// 2. 直接指针：只包含直接指针，无间接指针，限制文件大小
// 3. 简化信息：不包含权限、时间戳、所有者等UNIX i-node常见信息
// 4. 内存映射：可以在内存和磁盘之间直接传输

class FileHeader {
  public:
    // 为新文件分配磁盘空间并初始化文件头
    // 参数：bitMap-磁盘空间位图，fileSize-文件大小(字节)
    // 返回值：成功返回TRUE，空间不足返回FALSE
    bool Allocate(BitMap *bitMap, int fileSize);
    
    // 释放文件占用的所有数据块空间
    // 参数：bitMap-磁盘空间位图
    // 注意：只释放数据块，不释放文件头本身
    void Deallocate(BitMap *bitMap);  		

    // 从磁盘读取文件头内容到内存
    // 参数：sectorNumber-文件头所在的扇区号
    void FetchFrom(int sectorNumber); 	
    
    // 将内存中的文件头内容写回磁盘
    // 参数：sectorNumber-文件头要写入的扇区号
    void WriteBack(int sectorNumber); 	

    // 将文件中的字节偏移量转换为对应的磁盘扇区号
    // 参数：offset-文件内的字节偏移量
    // 返回值：对应的磁盘扇区号
    // 实现原理：sectorIndex = offset / SectorSize
    int ByteToSector(int offset);	

    // 获取文件的实际大小(字节数)
    // 返回值：文件包含的字节数
    int FileLength();			

    // 打印文件头信息和文件内容(调试用)
    // 注意：会读取整个文件内容，对大文件可能很慢
    void Print();			

  private:
    int numBytes;			// 文件的实际字节数(逻辑大小)
    int numSectors;			// 文件占用的数据扇区数
    int dataSectors[NumDirect];		// 直接指针数组，存储数据块的扇区号
    					// 每个条目指向一个数据块的起始扇区
};

#endif // FILEHDR_H