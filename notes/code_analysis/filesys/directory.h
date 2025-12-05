// directory.h 
//	管理类似 UNIX 的文件名目录的数据结构。
// 
//      目录是成对的表：<文件名，扇区号>，
//	给出目录中每个文件的名称，以及
//	在哪里可以找到它的文件头（描述
//	在哪里可以找到文件数据块的数据结构）在磁盘上。
//
//      我们假设互斥由调用者提供。
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "openfile.h"

// 文件名最大长度限制
// 为简单起见，我们假设文件名长度 <= 9 个字符
// +1 是为了字符串结尾的'\0'字符
#define FileNameMaxLen 		9

// 下面的类定义了一个"目录条目"，代表目录中的一个文件。
// 每个条目给出文件的名称，以及文件的头部在磁盘上可以找到的位置。
//
// 内部数据结构保持公开，以便目录操作可以直接访问它们。
//
// 目录条目结构说明：
// 这是文件系统中目录的基本组成单元，每个条目对应一个文件
// 类似于UNIX文件系统中的目录项，但结构更简单

class DirectoryEntry {
  public:
    bool inUse;				// 使用状态标志：TRUE表示条目被占用，FALSE表示空闲
    int sector;				// 文件头所在的磁盘扇区号，用于定位文件
    char name[FileNameMaxLen + 1];	// 文件名字符串，最多9个字符+1个结束符'\0'
    
    // 注意：Nachos目录条目不包含以下UNIX目录项常见信息：
    // - 文件类型(普通文件/目录/设备文件等)
    // - 文件权限
    // - 文件大小
    // - 创建/修改时间
    // - 所有者信息
    // - i-node号(这里直接使用扇区号)
};

// 下面的类定义了一个类似 UNIX 的"目录"。目录中的每个条目
// 描述一个文件，以及在哪里可以在磁盘上找到它。
//
// 目录数据结构可以存储在内存中，或者在磁盘上。
// 当它在磁盘上时，它作为常规的 Nachos 文件存储。
//
// 构造函数在内存中初始化一个目录结构；
// FetchFrom/WriteBack 操作将目录信息从/到磁盘进行传输。
//
// 目录类设计说明：
// 1. 固定大小：目录在创建时指定大小，不支持动态扩展
// 2. 单级目录：不支持子目录，所有文件都在根目录下
// 3. 线性搜索：使用简单的线性搜索算法查找文件
// 4. 内存映射：目录可以完全加载到内存中进行操作

class Directory {
  public:
    // 构造函数：创建一个空的目录结构
    // 参数size：目录的最大容量(可容纳的文件数量)
    Directory(int size); 	

    // 析构函数：释放目录占用的内存资源
    ~Directory();			

    // 从磁盘加载目录内容到内存
    // 参数file：包含目录数据的打开文件对象
    void FetchFrom(OpenFile *file);  	
    
    // 将内存中的目录内容写回磁盘，持久化更改
    // 参数file：用于存储目录数据的打开文件对象
    void WriteBack(OpenFile *file);	

    // 查找文件并返回其文件头所在的扇区号
    // 参数name：要查找的文件名
    // 返回值：成功返回扇区号，失败返回-1
    int Find(char *name);		

    // 向目录中添加新文件
    // 参数name：文件名；newSector：文件头所在的扇区号
    // 返回值：成功返回TRUE，失败(文件已存在或目录满)返回FALSE
    bool Add(char *name, int newSector);  

    // 从目录中删除文件(逻辑删除，只标记条目为未使用)
    // 参数name：要删除的文件名
    // 返回值：成功返回TRUE，失败(文件不存在)返回FALSE
    bool Remove(char *name);		

    // 列出目录中所有文件的名称(简单列表)
    void List();			
    
    // 详细打印目录内容，包括文件信息和文件内容(调试用)
    void Print();			

  private:
    int tableSize;			// 目录表的最大容量(条目数)
    DirectoryEntry *table;		// 目录条目数组，每个条目存储<文件名，扇区号>

    // 私有辅助函数：查找文件名在目录表中的索引位置
    // 参数name：要查找的文件名
    // 返回值：成功返回索引(0到tableSize-1)，失败返回-1
    int FindIndex(char *name);		
};

#endif // DIRECTORY_H