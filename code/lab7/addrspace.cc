// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include "bitmap.h"

static int nextSpaceId = 0; // Simple counter for allocating spaceId

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------
// Static bitmap to keep track of physical page allocation 
static BitMap* physPageBitmap = NULL;

BitMap *
    AddrSpace::freeMap = new BitMap(NumPhysPages);
BitMap *
    AddrSpace::swapMap = new BitMap(NumPhysPages);

OpenFile *AddrSpace::swapFile = NULL; // Will be opened when needed

AddrSpace::AddrSpace(OpenFile *execFile)
{
    pagenum = 0;
    spaceId = nextSpaceId++; // Simple increment for spaceId allocation

    // Store the executable file pointer in the member variable
    executable = execFile;

    NoffHeader noffH;
    unsigned int size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    printf("Max frames per user process: %d, Swap file: SWAP0, Page replacement algorithm: FIFO\n", MemPages);

    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// Initialize the physical page bitmap if it hasn't been initialized yet
    if (physPageBitmap == NULL) {
        physPageBitmap = new BitMap(NumPhysPages);
    }

    InitPageTable();
    InitInFileAddr();
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Deallocate an address space, freeing physical pages.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    // Free physical pages allocated to this address space
    for (unsigned int i = 0; i < numPages; i++) {
        if(pageTable[i].valid) {
            physPageBitmap->Clear(pageTable[i].physicalPage);
        }
    }
    delete [] pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
        machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we don't
    // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState()
{
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState()
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}
////----------------------------------------------------------------------
// AddrSpace::Print
// 	Print address space information for debugging
//----------------------------------------------------------------------

void AddrSpace::Print()
{
    printf("--- Address Space Information ---\n");
    printf("SpaceId: %d, Number of pages: %d\n", spaceId, numPages);
    printf("Page Table:\n");
    printf("\tPage, \tFrame,\tValid,\tUse,\tDirty\n");
    
    // Check if pageTable is valid before accessing
    if (pageTable == NULL) {
        printf("ERROR: pageTable is NULL!\n");
        return;
    }
    
    for (unsigned int i = 0; i < numPages; i++)
    {
        printf("\t%d,\t%d,\t%d,\t%d,\t%d\n", pageTable[i].virtualPage, pageTable[i].physicalPage, pageTable[i].valid,
               pageTable[i].use, pageTable[i].dirty);
    }
    printf("------------------------------\n");
}

void AddrSpace::InitPageTable()
{
    point_vm = 0;
    pageTable = new TranslationEntry[numPages];

    for (unsigned int i = 0; i < numPages; i++)
    {
        pageTable[i].virtualPage = i;
        pageTable[i].use = false;
        pageTable[i].dirty = false;
        pageTable[i].readOnly = false;
        pageTable[i].inFileAddr = -1;

        if (i >= numPages - StackPages)
            pageTable[i].type = vmuserStack;
        else if (i < 1) // code pages
            pageTable[i].type = vmcode;
        else
            pageTable[i].type = vmuninitData;
            
        pageTable[i].physicalPage = -1;
        pageTable[i].valid = false;
    }
}

void AddrSpace::InitInFileAddr()
{
    NoffHeader noffH;
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) &&
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);
    
    if (noffH.code.size > 0) {
        unsigned int numP = divRoundUp(noffH.code.size, PageSize);
        unsigned int firstP = noffH.code.virtualAddr / PageSize;
        for (unsigned int i = firstP; i < firstP + numP && i < numPages; i++) {
            pageTable[i].inFileAddr = noffH.code.inFileAddr + (i - firstP) * PageSize;
            pageTable[i].type = vmcode;
        }
    }
    if (noffH.initData.size > 0) {
        unsigned int numP, firstP;
        numP = divRoundUp(noffH.initData.size, PageSize);
        firstP = noffH.initData.virtualAddr / PageSize;
        for (unsigned int i = firstP; i < numP + firstP && i < numPages; i++) {
            pageTable[i].inFileAddr = noffH.initData.inFileAddr + (i - firstP) * PageSize;
            pageTable[i].type = vminitData;
        }
    }
    if (noffH.uninitData.size > 0) {
        unsigned int numP, firstP;
        numP = divRoundUp(noffH.uninitData.size, PageSize);
        firstP = noffH.uninitData.virtualAddr / PageSize;
        for (unsigned int i = firstP; i < numP + firstP && i < numPages; i++) {
            pageTable[i].type = vmuninitData;
            pageTable[i].inFileAddr = -1; // Uninitialized data doesn't have file address
        }
    }
}

void AddrSpace::Translate(int addr, unsigned int *vpn, unsigned int *offset)
{
    unsigned int page = addr / PageSize;
    unsigned int off = addr % PageSize;

    *vpn = page;
    *offset = off;
}

int AddrSpace::FIFO(int badVAddr)
{
    //use one num to show the point
    unsigned int oldPage;
    unsigned int newPage;
    unsigned int tmp;
    
    Translate(badVAddr, &newPage, &tmp);
    
    // Check if the new page is within valid range
    if (newPage >= numPages) {
        printf("Error: Page number %d exceeds maximum pages %d\n", newPage, numPages);
        return 0;
    }
    
    pagenum+=1;
    
    if (pagenum<=5)
    {
        pageTable[newPage].virtualPage = newPage;
        pageTable[newPage].use = true;
        pageTable[newPage].dirty = false;
        pageTable[newPage].readOnly = false;
        pageTable[newPage].physicalPage = pagenum-1;
        pageTable[newPage].valid = true;
        printf(" in(frame %d)\n", pagenum-1);
        virtualMem[pagenum-1] = newPage;
        
        ReadIn(newPage);
        Print();
        
        return 0;
    }
    else
    {
        oldPage = virtualMem[point_vm];
        printf(" in(frame %d)\n", point_vm);
        ASSERT(newPage < numPages);
        virtualMem[point_vm] = newPage;
        point_vm = (point_vm + 1) % MemPages;
        return Swap(oldPage, newPage);
    }
}   

int AddrSpace::Swap(unsigned int oldPage, unsigned int newPage)
{
    // Check if both pages are within valid range
    if (oldPage >= numPages || newPage >= numPages) {
        printf("Error: Swap page numbers oldPage=%d or newPage=%d exceed maximum pages %d\n", oldPage, newPage, numPages);
        return 0;
    }
    
    int error = WriteBack(oldPage);

    if (oldPage == newPage)
    {
        pageTable[newPage].physicalPage = pageTable[oldPage].physicalPage;
    }
    else
    {
        pageTable[newPage].physicalPage = pageTable[oldPage].physicalPage;
        pageTable[oldPage].physicalPage = -1;
        pageTable[oldPage].valid = FALSE;
    }

    pageTable[newPage].valid = true;
    pageTable[newPage].use = true;
    pageTable[newPage].dirty = false;

    ReadIn(newPage);
    Print();
    return error;
}

int AddrSpace::WriteBack(unsigned int oldPage)
{
    // Check if the old page is within valid range
    if (oldPage >= numPages) {
        printf("Error: WriteBack page number %d exceeds maximum pages %d\n", oldPage, numPages);
        return 0;
    }

    
    // Check if the page has a valid physical frame
    if (pageTable[oldPage].physicalPage < 0 || pageTable[oldPage].physicalPage >= NumPhysPages) {
        printf("Error: Invalid physical page %d for virtual page %d\n", pageTable[oldPage].physicalPage, oldPage);
        return 0;
    }

    if (pageTable[oldPage].dirty)
    {
        switch (pageTable[oldPage].type)
        {
        case vmcode:
            executable->WriteAt(&(machine->mainMemory[pageTable[oldPage].physicalPage * PageSize]), PageSize,
                                pageTable[oldPage].inFileAddr);
            break;
        case vminitData:
            executable->WriteAt(&(machine->mainMemory[pageTable[oldPage].physicalPage * PageSize]), PageSize,
                                pageTable[oldPage].inFileAddr);
            break;
        case vmuninitData:
        case vmuserStack:
            if (swapFile == NULL) {
                swapFile = fileSystem->Open(const_cast<char*>("SWAP0"));
                ASSERT(swapFile != NULL);  // Ensure swap file can be opened
            }
            // Use a simpler approach: find any available slot in swap space
            int swapPage = swapMap->Find();
            if (swapPage >= 0) {
                pageTable[oldPage].inFileAddr = swapPage * PageSize;
                swapFile->WriteAt(&(machine->mainMemory[pageTable[oldPage].physicalPage * PageSize]), PageSize,
                                  pageTable[oldPage].inFileAddr);
            } else {
                printf("Error: No available swap space\n");
            }
            break;
        }
        pageTable[oldPage].dirty = false;

        return 1;
    }
    return 0;
}

void AddrSpace::ReadIn(unsigned int newPage)
{
    // Check if the new page is within valid range
    if (newPage >= numPages) {
        printf("Error: ReadIn page number %d exceeds maximum pages %d\n", newPage, numPages);
        return;
    }
    
    // Check if the page has a valid physical frame
    if (pageTable[newPage].physicalPage < 0 || pageTable[newPage].physicalPage >= NumPhysPages) {
        printf("Error: Invalid physical page %d for virtual page %d\n", pageTable[newPage].physicalPage, newPage);
        return;
    }
    
    switch (pageTable[newPage].type)
    {
    case vmcode:
    case vminitData:
        // Check if executable is valid
        if (executable == NULL) {
            printf("ERROR: executable is NULL in ReadIn!\n");
            return;
        }
        executable->ReadAt(&(machine->mainMemory[pageTable[newPage].physicalPage * PageSize]), PageSize,
                           pageTable[newPage].inFileAddr);
        break;
    case vmuninitData:
    case vmuserStack:
        if (swapFile == NULL) {
            swapFile = fileSystem->Open(const_cast<char*>("SWAP0"));
            ASSERT(swapFile != NULL);  // Ensure swap file can be opened
        }
        if (pageTable[newPage].inFileAddr >= 0)
        {
            swapFile->ReadAt(&(machine->mainMemory[pageTable[newPage].physicalPage * PageSize]), PageSize,
                             pageTable[newPage].inFileAddr);
            swapMap->Clear(pageTable[newPage].inFileAddr / PageSize);
            pageTable[newPage].inFileAddr = -1;
        }
        break;
    default:
        printf("Error: Unknown page type %d\n", pageTable[newPage].type);
        break;
    }
}
