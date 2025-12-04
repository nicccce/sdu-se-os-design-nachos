// utility.cc 
//	调试例程。允许用户控制是否
//	打印DEBUG语句，基于命令行参数。
//
// 版权所有 (c) 1992-1993 加州大学董事会。
// 保留所有权利。请参阅 copyright.h 以获得版权公告和责任限制 
// 以及保修声明条款。

#include "copyright.h"
#include "utility.h"

// 这似乎依赖于编译器的配置方式。
// 如果你在va_start上有问题，尝试这两种替代方法
#include <stdarg.h>

static char *enableFlags = NULL; // 控制哪些DEBUG消息被打印

//----------------------------------------------------------------------
// DebugInit
//      初始化，使得只有带有flagList中标志的DEBUG消息
//	将被打印。
//
//	如果标志是"+"，我们启用所有DEBUG消息。
//
// 	"flagList"是一个字符串，包含要启用其DEBUG消息的字符。
//----------------------------------------------------------------------

void
DebugInit(char *flagList)
{
    enableFlags = flagList;
}

//----------------------------------------------------------------------
// DebugIsEnabled
//      如果带有"flag"的DEBUG消息要被打印，则返回TRUE。
//----------------------------------------------------------------------

bool
DebugIsEnabled(char flag)
{
    if (enableFlags != NULL)
       return (bool)((strchr(enableFlags, flag) != 0) 
		|| (strchr(enableFlags, '+') != 0));
    else
      return FALSE;
}

//----------------------------------------------------------------------
// DEBUG
//      打印调试消息，如果标志已启用。类似于printf，
//	只是在前面多了一个参数。
//----------------------------------------------------------------------

void 
DEBUG(char flag, const char *format, ...)
{
    if (DebugIsEnabled(flag)) {
	va_list ap;
	// 你会在这里收到一个未使用变量的消息 -- 忽略它。
	va_start(ap, format);
	vfprintf(stdout, format, ap);
	va_end(ap);
	fflush(stdout);
    }
}