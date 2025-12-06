// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "addrspace.h"
#include "thread.h"

//----------------------------------------------------------------------
// AdvancePC
// 	Advance program counter after a system call.
//----------------------------------------------------------------------
static void AdvancePC()
{
    int pc = machine->ReadRegister(PCReg);
    machine->WriteRegister(PrevPCReg, pc);
    pc = machine->ReadRegister(NextPCReg);
    machine->WriteRegister(PCReg, pc);
    machine->WriteRegister(NextPCReg, pc + 4);
}

// Function to start user program execution in a new thread
static void UserThreadStart(_int arg)
{
    // Restore the address space state (page tables)
    currentThread->space->RestoreState();
    
    // Restore user register state (should have been initialized by space->InitRegisters())
    currentThread->RestoreUserState();
    
    // Start user program execution
    machine->Run();
    ASSERT(FALSE); // Should never reach here
}

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
        DEBUG('a', "Shutdown, initiated by user program.\n");
        interrupt->Halt();
    } 
    else if ((which == SyscallException) && (type == SC_Exec)) {
        // Exec system call implementation
        printf("Execute system call of Exec() \n");
        char filename[50];
        int addr = machine->ReadRegister(4);
        int i = 0;
        do {
            machine->ReadMem(addr + i, 1, (int *)&filename[i]);
        } while (filename[i++] != '\0');

        printf("Exec(%s):\n", filename);

        OpenFile *executable = fileSystem->Open(filename);
        if (executable == NULL) {
            printf("Unable to open file %s\n", filename);
            machine->WriteRegister(2, -1);  // return -1 on failure
            AdvancePC();
            return;
        }

        AddrSpace *space = new AddrSpace(executable);
        delete executable;

        Thread *thread = new Thread("exec thread");
        thread->space = space;

          
        // space->RestoreState();

        machine->WriteRegister(2, (int)space);  // return space pointer as ID
        AdvancePC();
        // Save current thread's user state before forking
        currentThread->SaveUserState();

        scheduler->ReadyToRun(currentThread);

        currentThread = thread;
        space->InitRegisters();  
        space->RestoreState();

        machine->Run();
        // Use Fork to properly initialize the new thread
        // Fork will automatically put the new thread on ready queue
        // thread->Fork((VoidFunctionPtr)UserThreadStart, 0);
        
        // // Put current thread on ready queue and yield
        // // scheduler->ReadyToRun(currentThread);
        // currentThread->Yield();                 // current thread yields, scheduler will pick next thread
    
    }
    else if ((which == SyscallException) && (type == SC_PrintInt)) {
        // PrintInt system call implementation
        int value = machine->ReadRegister(4);  // get argument from register r4
        printf("%d", value);
        machine->WriteRegister(2, value);      // return value
        AdvancePC();
        return;
    }
    else {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
    }
}
