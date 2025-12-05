// openfile.cc 
//	管理打开的 Nachos 文件的例程。与 UNIX 一样，在我们可以读取或写入文件之前
//	必须先打开文件。一旦我们全部完成，我们可以关闭它（在 Nachos 中，通过删除
//	OpenFile 数据结构）。
//
//	也与 UNIX 一样，为了方便，我们在文件打开时将文件头保留在内存中。
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "filehdr.h"
#include "openfile.h"
#include "system.h"

// OpenFile类设计说明：
// OpenFile类是Nachos文件系统的用户接口，提供了类似UNIX文件操作的API
// 主要功能包括：
// 1. 文件定位(seek) - 设置读写位置
// 2. 顺序读写 - 从当前seek位置开始读写
// 3. 随机读写 - 在指定位置读写
// 4. 文件大小查询 - 返回文件字节数
//
// 重要概念：
// - seekPosition: 当前文件指针位置，影响Read/Write操作的位置
// - 读写单位：字节，而非扇区
// - 透明扇区管理：隐藏了底层扇区边界对用户的影响
// - 线程安全：通过synchDisk确保磁盘访问的同步

//----------------------------------------------------------------------
// OpenFile::OpenFile
// 	打开一个 Nachos 文件进行读写。在文件打开时将文件头导入内存。
//
//	"sector" -- 此文件的文件头在磁盘上的位置
//
//	实现逻辑：
//	1. 创建FileHeader对象
//	2. 从磁盘读取文件头到内存
//	3. 初始化文件位置指针为0(文件开头)
//
//	关键点：
//	- 文件打开时立即加载文件头到内存，提高后续访问效率
//	- seekPosition初始化为0，新打开的文件从开头开始读写
//	- 如果文件头读取失败，对象状态可能不一致，需要进一步错误处理
//	- 文件头包含了文件大小、数据块位置等元信息
//----------------------------------------------------------------------

OpenFile::OpenFile(int sector)
{ 
    hdr = new FileHeader;			// 创建文件头对象
    hdr->FetchFrom(sector);			// 从磁盘扇区加载文件头
    seekPosition = 0;				// 初始化读写位置为文件开头
}

//----------------------------------------------------------------------
// OpenFile::~OpenFile
// 	关闭 Nachos 文件，释放任何内存中的数据结构。
//
//	实现逻辑：
//	1. 释放内存中的文件头对象
//	2. OpenFile对象被销毁
//
//	关键点：
//	- 遵循RAII原则，确保资源正确释放
//	- 不涉及磁盘上的数据修改，只是清理内存对象
//	- 文件内容的持久化由FileSystem在适当时候处理
//----------------------------------------------------------------------

OpenFile::~OpenFile()
{
    delete hdr;		// 释放内存中的文件头对象
}

//----------------------------------------------------------------------
// OpenFile::Seek
// 	更改打开文件内的当前位置 -- 下一次 Read 或 Write 将开始的位置。
//
//	"position" -- 文件内下一次 Read/Write 的位置
//
//	实现逻辑：
//	1. 直接设置seekPosition为指定位置
//	2. 位置超出文件边界时的行为由Read/Write函数处理
//
//	关键点：
//	- 立即改变文件指针位置，无磁盘I/O操作
//	- 位置可以是任意有效值(包括超过文件大小的位置)
//	- 如果位置超出文件边界，后续Read操作返回0，Write操作可能扩展文件(取决于实现)
//	- 类似UNIX的lseek系统调用，但更简单
//----------------------------------------------------------------------

void
OpenFile::Seek(int position)
{
    seekPosition = position;	// 直接设置文件读写位置
}	

//----------------------------------------------------------------------
// OpenFile::Read/Write
// 	从 seekPosition 开始读取/写入文件的一部分。
//	返回实际写入或读取的字节数，作为副作用，
//	增加文件内的当前位置。
//
//	使用更原始的 ReadAt/WriteAt 实现。
//
//	"into" -- 包含要从磁盘读取的数据的缓冲区
//	"from" -- 包含要写入磁盘的数据的缓冲区
//	"numBytes" -- 要传输的字节数
//
//	实现逻辑：
//	Read: 调用ReadAt从当前seekPosition读取，然后更新seekPosition
//	Write: 调用WriteAt在当前seekPosition写入，然后更新seekPosition
//
//	关键点：
//	- 这些是顺序读写函数，基于seekPosition进行操作
//	- 自动更新seekPosition，实现类似文件流的行为
//	- 返回值是实际读/写字节数，可能小于请求的字节数
//	- 使用ReadAt/WriteAt作为底层实现，处理扇区对齐等细节
//----------------------------------------------------------------------

int
OpenFile::Read(char *into, int numBytes)
{
   // 调用ReadAt从当前位置读取数据
   int result = ReadAt(into, numBytes, seekPosition);
   // 更新文件位置指针
   seekPosition += result;
   return result;  // 返回实际读取的字节数
}

int
OpenFile::Write(char *into, int numBytes)
{
   // 调用WriteAt在当前位置写入数据
   int result = WriteAt(into, numBytes, seekPosition);
   // 更新文件位置指针
   seekPosition += result;
   return result;  // 返回实际写入的字节数
}

//----------------------------------------------------------------------
// OpenFile::ReadAt/WriteAt
// 	从 "position" 开始读取/写入文件的一部分。
//	返回实际写入或读取的字节数，但没有副作用
//	（当然，除了 Write 修改文件之外）。
//
//	不保证请求开始或结束在偶数磁盘扇区边界上；
//	然而，磁盘只知道如何一次读取/写入整个磁盘扇区。
//	因此：
//
//	对于 ReadAt：
// 	   我们读取请求中的所有完整或部分扇区，但我们只复制我们感兴趣的部分。
//	对于 WriteAt：
// 	   我们必须首先读取任何将被部分写入的扇区，以便我们不会覆盖未修改的部分。
//	   然后我们复制将被修改的数据，并写回所有作为请求部分的完整或部分扇区。
//
//	"into" -- 包含要从磁盘读取的数据的缓冲区
//	"from" -- 包含要写入磁盘的数据的缓冲区
//	"numBytes" -- 要传输的字节数
//	"position" -- 要读取/写入的第一个字节在文件中的偏移量
//
//	ReadAt实现逻辑：
//	1. 验证读取请求的有效性
//	2. 计算需要读取的扇区范围
//	3. 从磁盘读取所有相关扇区到缓冲区
//	4. 从缓冲区复制所需字节到目标缓冲区
//
//	WriteAt实现逻辑：
//	1. 验证写入请求的有效性
//	2. 检查是否需要预读边界扇区(部分写入的情况)
//	3. 将要写入的数据复制到缓冲区
//	4. 将修改后的扇区写回磁盘
//
//	关键点：
//	- 处理跨扇区的读写操作
//	- 保持扇区边界对齐
//	- 避免覆盖未修改的数据(WriteAt中对边界扇区的特殊处理)
//	- 线程安全的磁盘访问
//----------------------------------------------------------------------

int
OpenFile::ReadAt(char *into, int numBytes, int position)
{
    // 获取文件长度信息
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    char *buf;        // 临时缓冲区，用于读取整个扇区

    // 第一步：验证读取请求参数
    if ((numBytes <= 0) || (position >= fileLength))
        return 0;                 // 无效请求，返回0
    if ((position + numBytes) > fileLength)        
        numBytes = fileLength - position;    // 调整读取大小，不超过文件边界
    DEBUG('f', "Reading %d bytes at %d, from file of length %d.\n",     
                        numBytes, position, fileLength);

    // 第二步：计算需要读取的扇区范围
    firstSector = divRoundDown(position, SectorSize);        // 第一个扇区
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);    // 最后一个扇区
    numSectors = 1 + lastSector - firstSector;            // 总扇区数

    // 第三步：分配临时缓冲区并读取所有相关扇区
    buf = new char[numSectors * SectorSize];    // 为所有扇区分配空间
    for (i = firstSector; i <= lastSector; i++) {        
        // 通过文件头获取第i个扇区的物理位置，从磁盘读取
        synchDisk->ReadSector(hdr->ByteToSector(i * SectorSize), 
                                        &buf[(i - firstSector) * SectorSize]);
    }

    // 第四步：从扇区缓冲区复制所需数据到目标缓冲区
    bcopy(&buf[position - (firstSector * SectorSize)], into, numBytes);
    delete [] buf;    // 释放临时缓冲区
    return numBytes;    // 返回实际读取字节数
}

int
OpenFile::WriteAt(char *from, int numBytes, int position)
{
    // 获取文件长度信息
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    bool firstAligned, lastAligned;    // 标记首尾扇区是否完全被写入
    char *buf;                // 临时缓冲区

    // 第一步：验证写入请求参数
    if ((numBytes <= 0) || (position >= fileLength))  // For original Nachos file system
//    if ((numBytes <= 0) || (position > fileLength))  // For lab4 ...
        return 0;                // 无效请求，返回0
    if ((position + numBytes) > fileLength)
        numBytes = fileLength - position;    // 调整写入大小，不超过文件边界
    DEBUG('f', "Writing %d bytes at %d, from file of length %d.\n",     
                        numBytes, position, fileLength);

    // 第二步：计算涉及的扇区范围
    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;

    // 第三步：分配临时缓冲区
    buf = new char[numSectors * SectorSize];

    // 第四步：检查首尾扇区是否需要预读
    // 如果起始位置不在扇区边界上，则首扇区是部分写入
    firstAligned = (bool)(position == (firstSector * SectorSize));
    // 如果结束位置不在扇区边界上，则尾扇区是部分写入
    lastAligned = (bool)((position + numBytes) == ((lastSector + 1) * SectorSize));

    // 如果首扇区是部分写入，需要先读取完整扇区内容
    if (!firstAligned)
        ReadAt(buf, SectorSize, firstSector * SectorSize);    
    // 如果尾扇区是部分写入，需要先读取完整扇区内容
    if (!lastAligned && ((firstSector != lastSector) || firstAligned))
        ReadAt(&buf[(lastSector - firstSector) * SectorSize], 
                                SectorSize, lastSector * SectorSize);    

    // 第五步：将要写入的数据复制到缓冲区的正确位置
    bcopy(from, &buf[position - (firstSector * SectorSize)], numBytes);

    // 第六步：将修改后的扇区写回磁盘
    for (i = firstSector; i <= lastSector; i++) {        
        // 将缓冲区中的扇区数据写回磁盘对应位置
        synchDisk->WriteSector(hdr->ByteToSector(i * SectorSize), 
                                        &buf[(i - firstSector) * SectorSize]);
    }
    delete [] buf;    // 释放临时缓冲区
    return numBytes;    // 返回实际写入字节数
}


//----------------------------------------------------------------------
// OpenFile::Length
// 	返回文件中的字节数。
//
//	实现逻辑：
//	调用内存中的文件头对象的FileLength方法
//
//	关键点：
//	- 返回的是文件的实际大小(逻辑大小)，而非分配的磁盘空间
//	- 非常高效的操作，无磁盘I/O
//	- 使用内存中缓存的文件头信息
//----------------------------------------------------------------------

int
OpenFile::Length() 
{ 
    return hdr->FileLength();	// 返回文件头中记录的文件长度
}