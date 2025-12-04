// post.h 
//	提供不可靠、无序、固定大小消息传递到其他
//	(直接连接)机器上的邮箱的数据结构。消息可能被网络
//	丢弃或延迟，但它们永远不会被损坏。
//
// 	美国邮政局将邮件投递到指定邮箱。
// 	类比地，我们的邮局将数据包投递到特定缓冲区
// 	(邮箱)，基于数据包头部中存储的邮箱号。
// 	邮件在邮箱中等待，直到线程请求它；如果邮箱
//      为空，线程可以等待邮件到达。
//
// 	因此，我们邮局提供的服务是去复用
// 	传入的数据包，将它们投递到适当的线程。
//
//      对于每条消息，你都会得到一个返回地址，它由"来自"
// 	地址组成，这是发送消息的机器的ID，以及
// 	一个"来自邮箱"，这是发送机器上的邮箱号
//	你可以发送确认，如果你的协议要求
//	这样做。
//
// 版权所有 (c) 1992-1993 加州大学董事会。
// 保留所有权利。请参阅 copyright.h 以获得版权公告和责任限制 
// 以及保修声明条款。

#include "copyright.h"

#ifndef POST_H
#define POST_H

#include "network.h"
#include "synchlist.h"

// 邮箱地址 -- 唯一标识给定机器上的一个邮箱。
// 邮箱只是消息的临时存储位置。
typedef int MailBoxAddress;

// 以下类定义了消息头部的一部分。
// 这是由邮局附加到消息的，然后消息
// 被发送到网络。

class MailHeader {
  public:
    MailBoxAddress to;		// 目标邮箱
    MailBoxAddress from;	// 要回复的邮箱
    unsigned length;		// 消息数据的字节数（不包括
				// 邮件头部）
};

// 单个消息中可以包含的最大"有效载荷" -- 真实数据
// 不包括MailHeader和PacketHeader

#define MaxMailSize 	(MaxPacketSize - sizeof(MailHeader))


// 以下类定义了传入/传出的"邮件"消息格式。
// 消息格式是分层的：
//	网络头部 (PacketHeader)
//	邮局头部 (MailHeader)
//	数据

class Mail {
  public:
     Mail(PacketHeader pktH, MailHeader mailH, char *msgData);
				// 通过将头部连接到数据来
				// 初始化邮件消息

     PacketHeader pktHdr;	// 网络附加的头部
     MailHeader mailHdr;	// 邮局附加的头部
     char data[MaxMailSize];	// 有效载荷 -- 消息数据
};

// 以下类定义单个邮箱，或消息的临时存储
// 传入的消息由邮局放入
// 适当的邮箱，然后这些消息可以被
// 这台机器上的线程检索。

class MailBox {
  public: 
    MailBox();			// 分配并初始化邮箱
    ~MailBox();			// 释放邮箱

    void Put(PacketHeader pktHdr, MailHeader mailHdr, char *data);
   				// 原子地将消息放入邮箱
    void Get(PacketHeader *pktHdr, MailHeader *mailHdr, char *data); 
   				// 原子地从邮箱获取消息
				// （如果没有消息获取则等待！）
  private:
    SynchList *messages;	// 邮箱只是一个到达消息的列表
};

// 以下类定义"邮局"，或
// 邮箱集合。邮局是一个同步对象，提供
// 两个主要操作：Send -- 向远程机器上的邮箱发送消息，
// 以及Receive -- 等待消息进入邮箱，
// 然后移除并返回它。
//
// 传入的消息由邮局放入
// 适当的邮箱，唤醒任何在Receive上等待的线程。

class PostOffice {
  public:
    PostOffice(NetworkAddress addr, double reliability,
	       double orderability, int nBoxes);
				// 分配并初始化邮局
				//   "reliability" 是底层网络
				//   丢弃的数据包数量
    ~PostOffice();		// 释放邮局数据
    
    void Send(PacketHeader pktHdr, MailHeader mailHdr, char *data);
    				// 向远程机器上的邮箱发送消息。
				// MailHeader中的fromBox是
				// 确认的返回邮箱。
    
    void Receive(int box, PacketHeader *pktHdr, 
		MailHeader *mailHdr, char *data);
    				// 从"box"检索消息。等待如果
				// 盒子中没有消息。

    void PostalDelivery();	// 等待传入消息，
				// 然后将它们放入正确的邮箱

    void PacketSent();		// 中断处理程序，在传出数据包
				// 已被放置到网络上调用；现在可以发送
				// 下一个数据包
    void IncomingPacket();	// 中断处理程序，在传入数据包
   				// 已到达并且可以被拉取时调用
				// 网络（即，可以调用
				// PostalDelivery的时间）

  private:
    Network *network;		// 物理网络连接
    NetworkAddress netAddr;	// 此机器的网络地址
    MailBox *boxes;		// 邮箱表，用于保存传入邮件
    int numBoxes;		// 邮箱数量
    Semaphore *messageAvailable;// 当消息从网络到达时V操作
    Semaphore *messageSent;	// 当下一个消息可以发送到网络时V操作
    Lock *sendLock;		// 一次只有一个传出消息
};

#endif