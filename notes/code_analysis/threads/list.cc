// list.cc 
//
//     	管理"事物"的单向链接列表的例程。
//
// 	为要放在列表上的每个项目分配一个"ListElement"；
//	在移除项目时将其释放。这意味着
//      我们不需要在每个想要放在列表上的对象中
//      保持一个"next"指针。
// 
//     	注意：互斥必须由调用者提供。
//  	如果你想要一个同步列表，你必须使用
//	在 synchlist.cc 中的例程。
//
// 版权所有 (c) 1992-1993 加州大学董事会。
// 保留所有权利。请参阅 copyright.h 以获得版权公告和责任限制 
// 以及保修声明条款。

#include "copyright.h"
#include "list.h"

//----------------------------------------------------------------------
// ListElement::ListElement
// 	初始化一个列表元素，以便它可以被添加到列表的某个位置。
//
//	"itemPtr" 是要放在列表上的项目。它可以是指向
//		任何东西的指针。
//	"sortKey" 是项目的优先级（如果有的话）。
//----------------------------------------------------------------------

ListElement::ListElement(void *itemPtr, int sortKey)
{
     item = itemPtr;
     key = sortKey;
     next = NULL;	// 假设我们将它放在列表的末尾
}

//----------------------------------------------------------------------
// List::List
//	初始化一个列表，开始时为空。
//	现在可以将元素添加到列表中。
//----------------------------------------------------------------------

List::List()
{ 
    first = last = NULL; 
}

//----------------------------------------------------------------------
// List::~List
//	准备释放一个列表。如果列表仍然包含任何
//	ListElements，释放它们。但是，请注意我们*不*
//	释放列表上的"items" -- 此模块分配
//	和释放ListElements以跟踪每个项目，
//	但给定项目可能在多个列表上，所以我们不能
//	在这里释放它们。
//----------------------------------------------------------------------

List::~List()
{ 
    while (Remove() != NULL)
	;	 // 删除所有列表元素
}

//----------------------------------------------------------------------
// List::Append
//      将一个"item"附加到列表的末尾。
//      
//	分配一个ListElement来跟踪项目。
//      如果列表为空，则这将是唯一元素。
//	否则，将其放在末尾。
//
//	"item" 是要放在列表上的东西，它可以是指向
//		任何东西的指针。
//----------------------------------------------------------------------

void
List::Append(void *item)
{
    ListElement *element = new ListElement(item, 0);

    if (IsEmpty()) {		// 列表为空
	first = element;
	last = element;
    } else {			// 否则将其放在最后
	last->next = element;
	last = element;
    }
}

//----------------------------------------------------------------------
// List::Prepend
//      将一个"item"放在列表的前面。
//      
//	分配一个ListElement来跟踪项目。
//      如果列表为空，则这将是唯一元素。
//	否则，将其放在开头。
//
//	"item" 是要放在列表上的东西，它可以是指向
//		任何东西的指针。
//----------------------------------------------------------------------

void
List::Prepend(void *item)
{
    ListElement *element = new ListElement(item, 0);

    if (IsEmpty()) {		// 列表为空
	first = element;
	last = element;
    } else {			// 否则将其放在第一个之前
	element->next = first;
	first = element;
    }
}

//----------------------------------------------------------------------
// List::Remove
//      从列表的前面移除第一个"item"。
// 
// 返回:
//	指向移除项目的指针，如果列表上没有项目则为NULL。
//----------------------------------------------------------------------

void *
List::Remove()
{
    return SortedRemove(NULL);  // 与SortedRemove相同，但忽略键
}

//----------------------------------------------------------------------
// List::Mapcar
//	将一个函数应用于列表上的每个项目，通过遍历
//	列表，一次一个元素。
//
//	与LISP不同，这个mapcar不返回任何东西！
//
//	"func" 是要应用于列表每个元素的过程。
//----------------------------------------------------------------------

void
List::Mapcar(VoidFunctionPtr func)
{
    for (ListElement *ptr = first; ptr != NULL; ptr = ptr->next) {
       DEBUG('l', "In mapcar, about to invoke %x(%x)\n", func, ptr->item);
       (*func)((_int)ptr->item);
    }
}

//----------------------------------------------------------------------
// List::IsEmpty
//      如果列表为空（没有项目）则返回TRUE。
//----------------------------------------------------------------------

bool
List::IsEmpty() 
{ 
    if (first == NULL)
        return TRUE;
    else
	return FALSE; 
}

//----------------------------------------------------------------------
// List::SortedInsert
//      将一个"item"插入到列表中，使列表元素按"sortKey"的
//	升序排列。
//      
//	分配一个ListElement来跟踪项目。
//      如果列表为空，则这将是唯一元素。
//	否则，逐个遍历列表元素，
//	以找到新项目应该放置的位置。
//
//	"item" 是要放在列表上的东西，它可以是指向
//		任何东西的指针。
//	"sortKey" 是项目的优先级。
//----------------------------------------------------------------------

void
List::SortedInsert(void *item, int sortKey)
{
    ListElement *element = new ListElement(item, sortKey);
    ListElement *ptr;		// 跟踪

    if (IsEmpty()) {	// 如果列表为空，放入
        first = element;
        last = element;
    } else if (sortKey < first->key) {	
		// 项目放在列表前面
	element->next = first;
	first = element;
    } else {		// 寻找列表中第一个比项目大的元素
        for (ptr = first; ptr->next != NULL; ptr = ptr->next) {
            if (sortKey < ptr->next->key) {
		element->next = ptr->next;
	        ptr->next = element;
		return;
	    }
	}
	last->next = element;		// 项目放在列表末尾
	last = element;
    }
}

//----------------------------------------------------------------------
// List::SortedRemove
//      从排序列表的前面移除第一个"item"。
// 
// 返回:
//	指向移除项目的指针，如果列表上没有项目则为NULL。
//	将*keyPtr设置为移除项目的优先级值
//	（这在interrupt.cc中需要，例如）。
//
//	"keyPtr" 是一个指向存储移除项目
//		优先级的位置的指针。
//----------------------------------------------------------------------

void *
List::SortedRemove(int *keyPtr)
{
    ListElement *element = first;
    void *thing;

    if (IsEmpty()) 
	return NULL;

    thing = first->item;
    if (first == last) {	// 列表有一个项目，现在没有了
        first = NULL;
	last = NULL;
    } else {
        first = element->next;
    }
    if (keyPtr != NULL)
        *keyPtr = element->key;
    delete element;
    return thing;
}