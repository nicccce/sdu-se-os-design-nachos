// main.cc 
//	Bootstrap code to initialize the operating system kernel.
//      For Lab 2, focusing on priority scheduling.
//
// Usage: n2 -d <debugflags> -rs <random seed #>
//		-P (Priority Test) -C (Complex Priority Test) -S (Starvation Test)
//		-z (print copyright)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#define MAIN
#include "copyright.h"
#undef MAIN

#include "utility.h"
#include "system.h"

// External function declarations for scheduler tests
extern void ThreadTest(void);
extern void PriorityTest(void), ComplexPriorityTest(void), StarvationTest(void);

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(_int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread %d looped %d times, priority=%d\n", (int) which, num, currentThread->getPriority());
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest
// 	Set up a test with multiple threads of different priorities
//----------------------------------------------------------------------

void
ThreadTest()
{
    DEBUG('t', "Entering ThreadTest");

    Thread *t1 = new Thread("forked thread 1", 1);  // priority 1 (highest)
    Thread *t2 = new Thread("forked thread 2", 2);  // priority 2 
    Thread *t3 = new Thread("forked thread 3", 3);  // priority 3

    t1->Fork(SimpleThread, 1);
    t2->Fork(SimpleThread, 2);
    t3->Fork(SimpleThread, 3);
    SimpleThread(0);  // main thread has default priority (which should be 9)
}

//----------------------------------------------------------------------
// main
// 	Bootstrap the operating system kernel.  
//	
//	Check command line arguments
//	Initialize data structures
//	(optionally) Call test procedure
//
//	"argc" is the number of command line arguments (including the name
//		of the command) -- ex: "n2 -d +" -> argc = 3 
//	"argv" is an array of strings, one for each command line argument
//		ex: "n2 -d +" -> argv = {"n2", "-d", "+"}
//----------------------------------------------------------------------

int
main(int argc, char **argv)
{
    int argCount;               // the number of arguments 
                    // for a particular command

    DEBUG('t', "Entering main");
    (void) Initialize(argc, argv);
    
#ifdef THREADS
    // Default to ThreadTest unless another test is specified
    bool testCalled = false;
    
    // Check for specific test options
    for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount) {
	argCount = 1;
        if (!strcmp(*argv, "-z"))               // print copyright
            printf ("%s", copyright);
        
        if (!strcmp(*argv, "-P")) {             // run priority test
            PriorityTest();
            testCalled = true;
            argCount = 1;
        } else if (!strcmp(*argv, "-C")) {      // run complex priority test
            ComplexPriorityTest();
            testCalled = true;
            argCount = 1;
        } else if (!strcmp(*argv, "-S")) {      // run starvation test
            StarvationTest();
            testCalled = true;
            argCount = 1;
        }
    }
    
    // If no specific test was called, run default ThreadTest
    if (!testCalled) {
        ThreadTest();
    }
#endif

    currentThread->Finish();    // NOTE: if the procedure "main" 
                // returns, then the program "n2"
                // will exit (as any other normal program
                // would).  But there may be other
                // threads on the ready list.  We switch
                // to those threads by saying that the
                // "main" thread is finished, preventing
                // it from returning.
    return(0);          // Not reached...
}
