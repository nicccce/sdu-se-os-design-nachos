// main.cc 
//	引导代码以初始化操作系统内核。
//
//	允许直接调用内部操作系统函数，
//	以简化调试和测试。实际上，
//	引导代码只会初始化数据结构，
//	并启动一个用户程序以打印登录提示。
//
// 	此文件中的大部分内容直到后续作业才需要。
//
// 用法: nachos -d <debugflags> -rs <random seed #>
//		-s -x <nachos file> -c <consoleIn> <consoleOut>
//		-f -cp <unix file> <nachos file>
//		-p <nachos file> -r <nachos file> -l -D -t
//              -n <network reliability> -e <network orderability>
//              -m <machine id>
//              -o <other machine id>
//              -z
//
//    -d 导致打印某些调试消息（参见 utility.h）
//    -rs 导致在随机（但可重复）的位置发生 Yield
//    -z 打印版权信息
//
//  USER_PROGRAM
//    -s 使用户程序在单步模式下执行
//    -x 运行一个用户程序
//    -c 测试控制台
//
//  FILESYS
//    -f 导致物理磁盘被格式化
//    -cp 从 UNIX 复制文件到 Nachos
//    -p 打印一个 Nachos 文件到标准输出
//    -r 从文件系统中删除一个 Nachos 文件
//    -l 列出 Nachos 目录的内容
//    -D 打印整个文件系统的内容
//    -t 测试 Nachos 文件系统的性能
//
//  NETWORK
//    -n 设置网络可靠性
//    -e 设置网络顺序性
//    -m 设置此机器的主机ID（网络需要）
//    -o 运行一个简单的 Nachos 网络软件测试
//
//  注意 -- 标志在相关作业之前被忽略。
//  一些标志在这里解释；一些在 system.cc 中。
//
// 版权所有 (c) 1992-1993 加州大学董事会。
// 保留所有权利。请参阅 copyright.h 以获得版权公告和责任限制 
// 以及保修声明条款。

#define MAIN
#include "copyright.h"
#undef MAIN

#include "utility.h"
#include "system.h"


// 此文件使用的外部函数

extern void ThreadTest(void), Copy(char *unixFile, char *nachosFile);
extern void Print(char *file), PerformanceTest(void);
extern void StartProcess(char *file), ConsoleTest(char *in, char *out);
extern void MailTest(int networkID);
extern void SynchTest(void);

//----------------------------------------------------------------------
// main
// 	引导操作系统内核。
//	
//	检查命令行参数
//	初始化数据结构
//	（可选）调用测试过程
//
//	"argc" 是命令行参数的数量（包括命令名称）
//		例如： "nachos -d +" -> argc = 3 
//	"argv" 是字符串数组，每个命令行参数一个
//		例如： "nachos -d +" -> argv = {"nachos", "-d", "+"}
//----------------------------------------------------------------------

int
main(int argc, char **argv)
{
    int argCount;			// 特定命令的参数数量

    DEBUG('t', "Entering main");
    (void) Initialize(argc, argv);
    
#ifdef THREADS
    ThreadTest();
#if 0
    SynchTest();
#endif 
#endif

    for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount) {
	argCount = 1;
        if (!strcmp(*argv, "-z"))               // 打印版权信息
            printf ("%s", copyright);
#ifdef USER_PROGRAM
        if (!strcmp(*argv, "-x")) {        	// 运行一个用户程序
	    ASSERT(argc > 1);
            StartProcess(*(argv + 1));
            argCount = 2;
        } else if (!strcmp(*argv, "-c")) {      // 测试控制台
	    if (argc == 1)
	        ConsoleTest(NULL, NULL);
	    else {
		ASSERT(argc > 2);
	        ConsoleTest(*(argv + 1), *(argv + 2));
	        argCount = 3;
	    }
	    interrupt->Halt();		// 一旦我们启动控制台，那么
					// Nachos 将无限循环等待
					// 控制台输入
	}
#endif // USER_PROGRAM
#ifdef FILESYS
	if (!strcmp(*argv, "-cp")) { 		// 从 UNIX 复制到 Nachos
	    ASSERT(argc > 2);
	    Copy(*(argv + 1), *(argv + 2));
	    argCount = 3;
	} else if (!strcmp(*argv, "-p")) {	// 打印一个 Nachos 文件
	    ASSERT(argc > 1);
	    Print(*(argv + 1));
	    argCount = 2;
	} else if (!strcmp(*argv, "-r")) {	// 删除 Nachos 文件
	    ASSERT(argc > 1);
	    fileSystem->Remove(*(argv + 1));
	    argCount = 2;
	} else if (!strcmp(*argv, "-l")) {	// 列出 Nachos 目录
            fileSystem->List();
	} else if (!strcmp(*argv, "-D")) {	// 打印整个文件系统
            fileSystem->Print();
	} else if (!strcmp(*argv, "-t")) {	// 性能测试
            PerformanceTest();
	}
#endif // FILESYS
#ifdef NETWORK
        if (!strcmp(*argv, "-o")) {
	    ASSERT(argc > 1);
            Delay(2); 				// 延迟2秒
						// 给用户时间启动
						// 另一个 nachos
            MailTest(atoi(*(argv + 1)));
            argCount = 2;
        }
#endif // NETWORK
    }

    currentThread->Finish();	// 注意：如果程序 "main"
				// 返回，那么程序 "nachos"
				// 将退出（像任何其他普通程序
				// 一样）。但可能还有其他
				// 线程在就绪列表上。我们切换
				// 到那些线程通过说明
				// "main" 线程已完成，防止
				// 它返回。
    return(0);			// 不可达...
}