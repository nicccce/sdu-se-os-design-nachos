// list.h 
//	管理类似LISP列表的数据结构。
//
//      与LISP一样，列表可以包含任何类型的数据结构
//	作为列表上的项目：线程控制块，
//	待处理中断等。这就是为什么每个项目都是"void *"，
//	或者换句话说，是"指向任何东西的指针"。

// 版权所有 (c) 1992-1993 加州大学董事会。
// 保留所有权利。请参阅 copyright.h 以获得版权公告和责任限制 
// 以及保修声明条款。

#ifndef LIST_H
#define LIST_H

#include "copyright.h"
#include "utility.h"

// 以下类定义了一个"列表元素" -- 它
// 用于跟踪列表上的一个项目。它等价于
// 一个LISP单元，有一个"car"("next")指向列表上的下一个元素，
// 和一个"cdr"("item")指向列表上的项目。
//
// 内部数据结构保持公开，以便List操作可以
// 直接访问它们。

class ListElement {
   public:
     ListElement(void *itemPtr, int sortKey);	// 初始化一个列表元素

     ListElement *next;		// 列表上的下一个元素，
				// 如果这是最后一个则为NULL
     int key;		    	// 优先级，用于排序列表
     void *item; 	    	// 指向列表上项目的指针
};

// 以下类定义了一个"列表" -- 一个单链表
// 列表元素，每个元素指向列表上的单个项目。
//
// 通过使用"Sorted"函数，列表可以按ListElement中的"key"保持
// 按升序排序

class List {
  public:
    List();			// 初始化列表
    ~List();			// 释放列表

    void Prepend(void *item); 	// 将项目放在列表的开头
    void Append(void *item); 	// 将项目放在列表的末尾
    void *Remove(); 	 	// 从列表的前端取下项目

    void Mapcar(VoidFunctionPtr func);	// 对列表上的每个元素
					// 应用"func"
    bool IsEmpty();		// 列表是否为空？
    

    // 按顺序放入/取出列表项目的例程（按key排序）
    void SortedInsert(void *item, int sortKey);	// 将项目放入列表
    void *SortedRemove(int *keyPtr); 	  	// 从列表中移除第一个项目

  private:
    ListElement *first;  	// 列表的头部，如果列表为空则为NULL
    ListElement *last;		// 列表的最后一个元素
};

#endif // LIST_H