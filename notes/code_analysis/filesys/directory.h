// directory.h 
//	管理类UNIX目录的数据结构，用于文件名。
// 
//      目录是一个包含对的表：<文件名, 扇区号>，
//	给出目录中每个文件的名称，以及
//	在磁盘上找到其文件头的位置（描述
//	如何找到文件数据块的数据结构）。
//
//      我们假设互斥由调用者提供。
//
// 版权所有 (c) 1992-1993 加州大学董事会。
// 保留所有权利。请参阅 copyright.h 以获得版权公告和责任限制 
// 以及保修声明条款。

#include "copyright.h"

#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "openfile.h"

#define FileNameMaxLen 		9	// 为简化起见，我们假设
					// 文件名长度 <= 9 个字符

// 以下类定义了一个"目录条目"，表示目录中的一个文件。
// 每个条目给出文件的名称，以及
// 文件头在磁盘上的位置。
//
// 内部数据结构保持公开，以便 Directory 操作可以
// 直接访问它们。

class DirectoryEntry {
  public:
    bool inUse;				// 此目录条目是否正在使用？
    int sector;				// 在磁盘上找到此文件
					//   FileHeader 的位置
    char name[FileNameMaxLen + 1];	// 文件的文本名称，+1 用于
					// 尾部的 '\0'
};

// 以下类定义了一个类UNIX的"目录"。目录中的每个条目
// 描述一个文件，以及在磁盘上找到它的位置。
//
// 目录数据结构可以存储在内存中，或者在磁盘上。
// 当它在磁盘上时，它被存储为一个普通的 Nachos 文件。
//
// 构造函数在内存中初始化一个目录结构；而
// FetchFrom/WriteBack 操作将目录信息
// 从/到磁盘中搬移。

class Directory {
  public:
    Directory(int size); 		// 初始化一个空目录
					// 为 "size" 个文件预留空间
    ~Directory();			// 释放目录

    void FetchFrom(OpenFile *file);  	// 从磁盘初始化目录内容
    void WriteBack(OpenFile *file);	// 将修改写回
					// 目录内容到磁盘

    int Find(char *name);		// 查找文件的扇区号：
					// FileHeader 为文件："name"

    bool Add(char *name, int newSector);  // 将文件名添加到目录中

    bool Remove(char *name);		// 从目录中移除一个文件

    void List();			// 打印目录中所有文件的名称
					// 在目录中
    void Print();			// 详细打印目录内容
					// 目录的内容 -- 所有文件
					// 名称及其内容。

  private:
    int tableSize;			// 目录条目数量
    DirectoryEntry *table;		// 成对的表：
					// <文件名, 文件头位置>

    int FindIndex(char *name);		// 在目录中查找索引
					// 表中对应于 "name" 的索引
};

#endif // DIRECTORY_H