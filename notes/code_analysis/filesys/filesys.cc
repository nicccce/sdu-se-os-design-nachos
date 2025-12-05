// filesys.cc 
//	管理文件系统整体操作的例程。
//	实现从文本文件名到文件的映射例程。
//
//	文件系统中的每个文件都有：
//	   一个文件头，存储在磁盘上的一个扇区中
//		（文件头数据结构的大小被安排为精确等于 1 个磁盘扇区的大小）
//	   多个数据块
//	   文件系统目录中的一个条目
//
// 	文件系统由几个数据结构组成：
//	   空闲磁盘扇区的位图（参见 bitmap.h）
//	   文件名和文件头的目录
//
//      位图和目录都表示为普通文件。
//	它们的文件头位于特定的扇区（扇区 0 和扇区 1），
//	以便文件系统可以在启动时找到它们。
//
//	文件系统假设位图和目录文件在 Nachos 运行期间保持"打开"状态。
//
//	对于那些修改目录和/或位图的操作（如 Create、Remove），
//	如果操作成功，更改会立即写回磁盘（这两个文件在此期间保持打开状态）。
//	如果操作失败，并且我们已经修改了目录和/或位图的一部分，
//	我们只是丢弃更改的版本，而不将其写回磁盘。
//
// 	我们此时的实现有以下限制：
//
//	   没有并发访问的同步
//	   文件有固定大小，在文件创建时设置
//	   文件大小不能超过约 3KB
//	   没有分层目录结构，只能向系统添加有限数量的文件
//	   没有尝试使系统对故障具有健壮性
//	    （如果 Nachos 在修改文件系统的操作中途退出，
//	    可能会损坏磁盘）
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "disk.h"
#include "bitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"

// 文件系统磁盘布局常量定义
// 包含空闲扇区位图的文件头所在的扇区
#define FreeMapSector 		0
#define DirectorySector 	1	// 根目录文件头所在的扇区

// 位图和目录的初始文件大小；直到文件系统支持可扩展文件，
// 目录大小决定了可以加载到磁盘上的最大文件数量。
#define FreeMapFileSize 	(NumSectors / BitsInByte)	// 位图文件大小(字节)
#define NumDirEntries 		10	// 目录中最大文件数量
#define DirectoryFileSize 	(sizeof(DirectoryEntry) * NumDirEntries)	// 目录文件大小

//----------------------------------------------------------------------
// FileSystem::FileSystem
// 	初始化文件系统。如果 format = TRUE，磁盘上没有任何内容，
//	我们需要初始化磁盘以包含一个空目录和空闲扇区的位图
//	（几乎所有扇区都被标记为空闲，但不是全部）。
//
//	如果 format = FALSE，我们只需要打开表示位图和目录的文件。
//
//	"format" -- 我们是否应该初始化磁盘？
//
//	实现逻辑：
//	1. 如果是格式化操作：
//	   a. 创建位图和目录对象
//	   b. 为系统文件(位图文件头、目录文件头)预留扇区
//	   c. 为位图和目录数据分配空间
//	   d. 将文件头写入磁盘
//	   e. 打开位图和目录文件
//	   f. 初始化位图和目录内容并写回磁盘
//	2. 如果不是格式化操作：
//	   a. 直接打开已存在的位图和目录文件
//
//	关键点：
//	- 位图和目录文件头固定在扇区0和1
//	- 系统文件(位图、目录)本身也作为普通文件存储
//	- 这是一个"鸡生蛋还是蛋生鸡"的问题，通过先预留扇区解决
//	- 格式化后，整个文件系统结构就建立起来了
//----------------------------------------------------------------------

FileSystem::FileSystem(bool format)
{ 
    DEBUG('f', "Initializing the file system.\n");
    if (format) {
        // 创建文件系统核心数据结构
        BitMap *freeMap = new BitMap(NumSectors);			// 磁盘空间位图
        Directory *directory = new Directory(NumDirEntries);	// 根目录
	FileHeader *mapHdr = new FileHeader;				// 位图文件头
	FileHeader *dirHdr = new FileHeader;				// 目录文件头

        DEBUG('f', "Formatting the file system.\n");

    // 第一步：为系统文件头预留扇区
    // 确保位图和目录的文件头扇区不会被其他文件占用
	freeMap->Mark(FreeMapSector);	    // 标记扇区0为已用(位图文件头)
	freeMap->Mark(DirectorySector);	    // 标记扇区1为已用(目录文件头)

    // 第二步：为位图和目录的数据内容分配空间
    // 位图需要足够空间记录所有扇区状态
    // 目录需要足够空间存储目录条目
	ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));	// 分配位图数据空间
	ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));	// 分配目录数据空间

    // 第三步：将文件头写入磁盘
    // 这必须在打开文件之前完成，因为OpenFile需要读取文件头
        DEBUG('f', "Writing headers back to disk.\n");
	mapHdr->WriteBack(FreeMapSector);	    // 写入位图文件头到扇区0
	dirHdr->WriteBack(DirectorySector);	  // 写入目录文件头到扇区1

    // 第四步：打开位图和目录文件
    // 文件系统操作假设这两个文件在 Nachos 运行期间保持打开状态
    // 这是为了避免频繁打开/关闭系统文件的开销

        freeMapFile = new OpenFile(FreeMapSector);		// 打开位图文件
        directoryFile = new OpenFile(DirectorySector);	// 打开目录文件
     
    // 第五步：将初始化的位图和目录内容写回磁盘
    // 此时目录完全为空(没有文件)；
    // 位图已标记扇区0和1为已用，其他扇区标记为可用

        DEBUG('f', "Writing bitmap and directory back to disk.\n");
	freeMap->WriteBack(freeMapFile);	 // 将位图更改写回磁盘
	directory->WriteBack(directoryFile); // 将目录更改写回磁盘

	if (DebugIsEnabled('f')) {
	    freeMap->Print();      // 调试输出：打印位图内容
	    directory->Print();    // 调试输出：打印目录内容
        }
        // 释放临时对象，但保持文件句柄(freeMapFile, directoryFile)继续使用
        delete freeMap; 
	delete directory; 
	delete mapHdr; 
	delete dirHdr;

    } else {
    // 非格式化模式：从现有磁盘加载文件系统
    // 假设磁盘上已有完整的文件系统结构
    // 直接打开位图和目录文件用于后续操作
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
    }
}

//----------------------------------------------------------------------
// FileSystem::Create
// 	在 Nachos 文件系统中创建一个文件（类似于 UNIX create）。
//	由于我们无法动态增加文件的大小，我们必须给 Create 文件的初始大小。
//
//	创建文件的步骤是：
//	  确保文件不存在
//        为文件头分配一个扇区
//	  为文件的数据块在磁盘上分配空间
//	  将名称添加到目录
//	  在磁盘上存储新的文件头
//	  将位图和目录的更改刷新回磁盘
//
//	如果一切正常，返回 TRUE，否则返回 FALSE。
//
// 	在以下情况下创建失败：
//   		文件已在目录中
//	 	文件头没有空闲空间
//	 	目录中没有文件的空闲条目
//	 	文件的数据块没有空闲空间
//
// 	请注意，此实现假设没有对文件系统的并发访问！
//
//	"name" -- 要创建的文件的名称
//	"initialSize" -- 要创建的文件的大小
//
//	实现逻辑：
//	1. 加载当前目录内容到内存
//	2. 检查文件名是否已存在
//	3. 为新文件头分配一个磁盘扇区
//	4. 为文件数据分配所需空间
//	5. 在目录中添加新条目
//	6. 将所有更改写回磁盘
//
//	关键点：
//	- 文件大小在创建时固定，之后无法改变
//	- 原子操作：全部成功或全部失败
//	- 失败时不写回磁盘，保持原有状态
//	- 文件系统一致性通过这种设计保证
//----------------------------------------------------------------------

bool
FileSystem::Create(char *name, int initialSize)
{
    Directory *directory;	// 临时目录对象，用于操作目录
    BitMap *freeMap;		// 临时位图对象，用于管理磁盘空间
    FileHeader *hdr;		// 临时文件头对象，用于创建新文件头
    int sector;			// 分配给新文件头的扇区号
    bool success;		// 操作结果标志

    DEBUG('f', "Creating file %s, size %d\n", name, initialSize);

    // 第一步：从磁盘加载当前目录内容
    directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);

    // 第二步：检查文件是否已存在
    if (directory->Find(name) != -1)
      success = FALSE;			// 文件已存在，创建失败
    else {	
        // 第三步：加载磁盘空间位图
        freeMap = new BitMap(NumSectors);
        freeMap->FetchFrom(freeMapFile);
        
        // 第四步：为文件头分配一个扇区
        sector = freeMap->Find();	// 查找一个空闲扇区用于文件头
    	if (sector == -1) 		
            success = FALSE;		// 没有空闲扇区用于文件头，创建失败 
        else if (!directory->Add(name, sector))
            success = FALSE;	// 目录已满，无法添加新条目，创建失败
	else {
    	    // 第五步：为文件数据分配空间
    	    hdr = new FileHeader;
	    if (!hdr->Allocate(freeMap, initialSize))
            	success = FALSE;	// 没有足够空间用于文件数据，创建失败
	    else {	
	    	success = TRUE;		// 所有步骤成功
	
		// 第六步：将所有更改写回磁盘（原子操作的关键）
		// 确保所有操作都成功后，才持久化更改
    	    	hdr->WriteBack(sector); 			// 将新文件头写入磁盘
    	    	directory->WriteBack(directoryFile);	// 将更新的目录写回磁盘
    	    	freeMap->WriteBack(freeMapFile);		// 将更新的位图写回磁盘
	    }
            delete hdr;	// 释放临时文件头对象
	}
        delete freeMap;	// 释放临时位图对象
    }
    delete directory;	// 释放临时目录对象
    return success;	// 返回创建结果
}

//----------------------------------------------------------------------
// FileSystem::Open
// 	打开文件进行读写。
//	要打开文件：
//	  使用目录找到文件头的位置
//	  将头文件导入内存
//
//	"name" -- 要打开的文件的文本名称
//
//	实现逻辑：
//	1. 从磁盘加载目录内容
//	2. 在目录中查找指定文件名
//	3. 如果找到，获取文件头的扇区号
//	4. 创建OpenFile对象，将文件头读入内存
//	5. 返回OpenFile对象指针
//
//	关键点：
//	- 成功时返回OpenFile对象指针
//	- 失败时返回NULL(文件不存在)
//	- OpenFile对象负责文件的读写操作
//	- 文件打开后可以进行多次读写操作
//
//	返回值：
//	- 成功：返回指向OpenFile对象的指针
//	- 失败：返回NULL
//----------------------------------------------------------------------

OpenFile *
FileSystem::Open(char *name)
{ 
    Directory *directory = new Directory(NumDirEntries);	// 创建临时目录对象
    OpenFile *openFile = NULL;		// 打开文件对象指针，初始为NULL
    int sector;				// 文件头所在的扇区号

    DEBUG('f', "Opening file %s\n", name);
    
    // 从磁盘加载目录内容到内存
    directory->FetchFrom(directoryFile);
    
    // 在目录中查找文件名，获取文件头扇区号
    sector = directory->Find(name); 
    
    // 如果找到文件(扇区号>=0)，创建OpenFile对象
    if (sector >= 0) 		
	openFile = new OpenFile(sector);	// 创建OpenFile对象并加载文件头
    delete directory;	// 释放临时目录对象
    return openFile;	// 返回OpenFile对象指针，如果未找到则为NULL
}

//----------------------------------------------------------------------
// FileSystem::Remove
// 	从文件系统中删除文件。这需要：
//	    从目录中删除它
//	    删除其头的空间
//	    删除其数据块的空间
//	    将目录、位图的更改写回磁盘
//
//	如果文件被删除，返回 TRUE，如果文件不在文件系统中，返回 FALSE。
//
//	"name" -- 要删除的文件的文本名称
//
//	实现逻辑：
//	1. 从磁盘加载目录内容
//	2. 在目录中查找要删除的文件
//	3. 如果找到，从磁盘加载文件头
//	4. 加载磁盘空间位图
//	5. 释放文件的所有数据块
//	6. 释放文件头占用的扇区
//	7. 从目录中移除文件条目
//	8. 将更新后的位图和目录写回磁盘
//
//	关键点：
//	- 完整的删除操作：数据、文件头、目录条目全部清理
//	- 原子操作：全部成功或全部失败
//	- 释放的空间可以被新文件重用
//	- 文件删除后无法恢复(没有回收站机制)
//
//	返回值：
//	- TRUE：文件成功删除
//	- FALSE：文件不存在，删除失败
//----------------------------------------------------------------------

bool
FileSystem::Remove(char *name)
{ 
    Directory *directory;	// 临时目录对象
    BitMap *freeMap;		// 临时位图对象
    FileHeader *fileHdr;	// 临时文件头对象
    int sector;			// 文件头所在的扇区号
    
    // 加载目录内容
    directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);
    
    // 在目录中查找要删除的文件
    sector = directory->Find(name);
    
    // 检查文件是否存在
    if (sector == -1) {
       delete directory;	// 释放临时目录对象
       return FALSE;			 // 文件未找到，删除失败 
    }
    
    // 加载文件头
    fileHdr = new FileHeader;
    fileHdr->FetchFrom(sector);

    // 加载磁盘空间位图
    freeMap = new BitMap(NumSectors);
    freeMap->FetchFrom(freeMapFile);

    // 释放文件占用的所有数据块
    fileHdr->Deallocate(freeMap);  		// 释放数据块空间
    freeMap->Clear(sector);			// 释放文件头占用的扇区
    directory->Remove(name);			// 从目录中移除文件条目

    // 将所有更改写回磁盘(原子操作)
    freeMap->WriteBack(freeMapFile);		// 将更新的位图写回磁盘
    directory->WriteBack(directoryFile);        // 将更新的目录写回磁盘
    
    // 释放临时对象
    delete fileHdr;
    delete directory;
    delete freeMap;
    return TRUE;	// 删除成功
} 

//----------------------------------------------------------------------
// FileSystem::List
//  列出文件系统目录中的所有文件。
//
// 实现逻辑：
// 1. 创建临时目录对象
// 2. 从磁盘加载目录内容
// 3. 调用目录的List方法打印所有文件名
// 4. 释放临时对象
//
// 输出示例：
// file1.txt
// test.c
// nachos
//
// 使用场景：
// - 用户查看目录内容
// - 文件系统调试
// - 类似sUNIX的ls命令功能
//----------------------------------------------------------------------

void
FileSystem::List()
{
    Directory *directory = new Directory(NumDirEntries);    // 创建临时目录对象

    directory->FetchFrom(directoryFile);    // 从磁盘加载目录内容
    directory->List();          // 打印所有文件名
    delete directory;           // 释放临时对象
}

//----------------------------------------------------------------------
// FileSystem::Print
//  打印关于文件系统的所有内容：
//    位图的内容
//    目录的内容
//    对于目录中的每个文件，
//        文件头的内容
//        文件中的数据
//
// 实现逻辑：
// 1. 创建所有必要的临时对象
// 2. 打印位图文件头信息
// 3. 打印目录文件头信息
// 4. 打印位图内容(扇区使用情况)
// 5. 打印目录内容(所有文件及其扇区号)
// 6. 对每个文件，打印其文件头和内容
// 7. 释放所有临时对象
//
// 关键点：
// - 这是一个全面的调试函数
// - 会读取整个文件系统的内容
// - 产生大量输出，仅用于详细调试
// - 显示文件系统的完整状态
//
// 输出包括：
// - 位图文件头
// - 目录文件头
// - 位图内容(扇区使用状态)
// - 目录内容(文件名和扇区号)
// - 每个文件的详细信息和内容
//----------------------------------------------------------------------

void
FileSystem::Print()
{
    FileHeader *bitHdr = new FileHeader;        // 临时位图文件头对象
    FileHeader *dirHdr = new FileHeader;        // 临时目录文件头对象
    BitMap *freeMap = new BitMap(NumSectors);       // 临时位图对象
    Directory *directory = new Directory(NumDirEntries);    // 临时目录对象

    // 打印位图文件头信息
    printf("Bit map file header:\n");
    bitHdr->FetchFrom(FreeMapSector);       // 从扇区0加载位图文件头
    bitHdr->Print();                // 打印位图文件头内容

    // 打印目录文件头信息
    printf("Directory file header:\n");
    dirHdr->FetchFrom(DirectorySector);       // 从扇区1加载目录文件头
    dirHdr->Print();                // 打印目录文件头内容

    // 加载并打印位图内容
    freeMap->FetchFrom(freeMapFile);
    freeMap->Print();

    // 加载并打印目录内容
    directory->FetchFrom(directoryFile);
    directory->Print();

    // 释放所有临时对象
    delete bitHdr;
    delete dirHdr;
    delete freeMap;
    delete directory;
}