// schedtest.cc
//	Test routines for the scheduler class.
//
//	Two different tests are implemented:
//	1. PriorityTest - Tests the priority scheduling functionality
//	2. ComplexPriorityTest - Tests more complex priority scenarios

#include "copyright.h"
#include "scheduler.h"
#include "thread.h"
#include "system.h"

//----------------------------------------------------------------------
// PriorityTestThread
//	Loop a fixed number of times, yielding each time, printing a
//	message.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
PriorityTestThread(_int which)
{
    int num;
    
    for (num = 0; num < 3; num++) {
        printf("Thread %d, iteration %d, priority %d\n", 
               (int) which, num, currentThread->getPriority());
        currentThread->Yield();
    }
    printf("Thread %d done!\n", (int) which);
}

//----------------------------------------------------------------------
// PriorityTest
// 	Test the priority scheduler by creating several threads with
//	different priorities and observing the execution order.
//----------------------------------------------------------------------

void
PriorityTest()
{
    DEBUG('t', "Entering PriorityTest");

    Thread *t1 = new Thread("high priority thread", 10);  // Higher priority (lower number)
    Thread *t2 = new Thread("low priority thread", 50);   // Lower priority (higher number) 
    Thread *t3 = new Thread("medium priority thread", 25); // Medium priority

    printf("Creating threads with different priorities:\n");
    printf("t1 (priority 10 - highest)\n");
    printf("t2 (priority 50 - lowest)\n");
    printf("t3 (priority 25 - medium)\n");

    t1->Fork(PriorityTestThread, 1);
    t2->Fork(PriorityTestThread, 2);
    t3->Fork(PriorityTestThread, 3);

    // Run the current thread (main) to allow other threads to execute
    PriorityTestThread(0);
}

//----------------------------------------------------------------------
// ComplexPriorityTestThread
// 	A more complex test thread that changes its priority during execution
//----------------------------------------------------------------------

void
ComplexPriorityTestThread(_int which)
{
    int num;
    
    for (num = 0; num < 3; num++) {
        printf("Thread %d, iteration %d, priority %d\n", 
               (int) which, num, currentThread->getPriority());
        
        // Change priority in the middle of execution
        if (num == 1) {
            int newPriority = currentThread->getPriority() + 60;  // Dramatically lower the priority
            printf("Thread %d changing priority from %d to %d\n", 
                   (int) which, currentThread->getPriority(), newPriority);
            currentThread->setPriority(newPriority);
        }
        
        currentThread->Yield();
    }
    printf("Thread %d done!\n", (int) which);
}

//----------------------------------------------------------------------
// ComplexPriorityTest
// 	Test priority changes during execution and how scheduler handles them
//----------------------------------------------------------------------

void
ComplexPriorityTest()
{
    DEBUG('t', "Entering ComplexPriorityTest");

    Thread *t1 = new Thread("changing priority thread", 5);   // Start with high priority
    Thread *t2 = new Thread("medium priority thread", 35);    // Medium priority
    Thread *t3 = new Thread("high priority thread", 15);      // High priority

    printf("Creating threads:\n");
    printf("t1 (starts at priority 5, changes to 65 during execution)\n");
    printf("t2 (priority 35)\n");
    printf("t3 (priority 15)\n");

    t1->Fork(ComplexPriorityTestThread, 1);
    t2->Fork(ComplexPriorityTestThread, 2);
    t3->Fork(ComplexPriorityTestThread, 3);

    ComplexPriorityTestThread(0);
}

//----------------------------------------------------------------------
// StarvationTestThread
// 	Thread that runs for a longer time to test if low priority threads
//	get starved
//----------------------------------------------------------------------

void
StarvationTestThread(_int which)
{
    int num;
    
    printf("Thread %d (priority %d) starting long execution...\n", 
           (int) which, currentThread->getPriority());
    
    // Simulate longer execution by having more iterations
    for (num = 0; num < 5; num++) {
        printf("Thread %d, long iteration %d\n", (int) which, num);
        // Don't yield every time to simulate longer execution
        if (num % 2 == 0) {
            currentThread->Yield();
        }
    }
    
    printf("Thread %d (priority %d) finished long execution\n", 
           (int) which, currentThread->getPriority());
}

//----------------------------------------------------------------------
// StarvationTest
// 	Test if low priority threads get a chance to execute when high 
//	priority threads are running
//----------------------------------------------------------------------

void
StarvationTest()
{
    DEBUG('t', "Entering StarvationTest");

    Thread *t1 = new Thread("high priority long thread", 5);  // High priority, long execution
    Thread *t2 = new Thread("low priority short thread", 80); // Low priority, should still get chance

    printf("Creating threads:\n");
    printf("t1 (high priority 5, long execution)\n");
    printf("t2 (low priority 80, short execution)\n");

    t1->Fork(StarvationTestThread, 1);
    t2->Fork(PriorityTestThread, 2); // Using the shorter test function

    PriorityTestThread(0);
}
