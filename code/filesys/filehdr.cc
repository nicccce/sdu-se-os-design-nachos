// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"
#include <sys/time.h>  // For time functions
#include <time.h>      // For time formatting functions

//----------------------------------------------------------------------
// FileHeader::PrintTime
// 	Print the modification time in a human-readable format.
//
//	"time" is the time in seconds since Unix epoch
//----------------------------------------------------------------------

void
FileHeader::PrintTime(int time)
{
    time_t rawtime = time;
    struct tm *timeinfo = localtime(&rawtime);
    char buffer[80];
    
    strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y", timeinfo);
    printf("%s", buffer);
}

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
    numBytes = fileSize;
    SetModifyTime();  // Set modification time to current time
    int numSectors = GetNumSectors();
    
    if (numSectors > (NumDirect + NumIndirect))
	return FALSE;		// file too large

    if (freeMap->NumClear() < numSectors)
	return FALSE;		// not enough space

    // Initialize dataSectors array
    memset(dataSectors, 0, sizeof(dataSectors));
    
    if (numSectors <= NumDirect) {
	// Only need direct blocks
	for (int i = 0; i < numSectors; i++)
	    dataSectors[i] = freeMap->Find();
	dataSectors[NumDirect] = -1;  // No indirect block
    } else {
	// Need both direct and indirect blocks
	int indirectSectors[NumIndirect];
	
	// Allocate direct blocks
	for (int i = 0; i < NumDirect; i++)
	    dataSectors[i] = freeMap->Find();
	
	// Allocate indirect block
	int indirectSector = freeMap->Find();
	dataSectors[NumDirect] = indirectSector;
	
	// Allocate sectors pointed to by indirect block
	int numIndirect = numSectors - NumDirect;
	for (int i = 0; i < numIndirect; i++)
	    indirectSectors[i] = freeMap->Find();
	
	// Write indirect block to disk
	synchDisk->WriteSector(indirectSector, (char *)indirectSectors);
    }
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
    int numSectors = GetNumSectors();
    
    if (numSectors <= NumDirect) {
	// Only direct blocks to deallocate
	for (int i = 0; i < numSectors; i++) {
	    ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
	    freeMap->Clear((int) dataSectors[i]);
	}
    } else {
	// Deallocate direct blocks
	for (int i = 0; i < NumDirect; i++) {
	    ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
	    freeMap->Clear((int) dataSectors[i]);
	}
	
	// Deallocate indirect block and its sectors
	int indirectSectors[NumIndirect];
	synchDisk->ReadSector(dataSectors[NumDirect], (char *)indirectSectors);
	
	int numIndirect = numSectors - NumDirect;
	for (int i = 0; i < numIndirect; i++) {
	    ASSERT(freeMap->Test((int) indirectSectors[i]));  // ought to be marked!
	    freeMap->Clear((int) indirectSectors[i]);
	}
	
	// Clear the indirect block itself
	ASSERT(freeMap->Test((int) dataSectors[NumDirect]));  // ought to be marked!
	freeMap->Clear((int) dataSectors[NumDirect]);
    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    int sectorNum = offset / SectorSize;
    
    if (sectorNum < NumDirect) {
	// Direct block
	return dataSectors[sectorNum];
    } else {
	// Indirect block
	int indirectSectors[NumIndirect];
	synchDisk->ReadSector(dataSectors[NumDirect], (char *)indirectSectors);
	return indirectSectors[sectorNum - NumDirect];
    }
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::SetModifyTime
// 	Set the modification time to the current time.
//----------------------------------------------------------------------

void
FileHeader::SetModifyTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);  // Get current time
    modifyTime = tv.tv_sec;  // Store seconds since Unix epoch
}

//----------------------------------------------------------------------
// FileHeader::GetModifyTime
// 	Get the modification time of the file.
//----------------------------------------------------------------------

int
FileHeader::GetModifyTime()
{
    return modifyTime;
}

//----------------------------------------------------------------------

//----------------------------------------------------------------------
// FileHeader::GetNumSectors
// 	Get the number of sectors.
//----------------------------------------------------------------------

int
FileHeader::GetNumSectors()
{
    // Calculate number of sectors from file size, not from modifyTime
    return divRoundUp(numBytes, SectorSize);
}

//----------------------------------------------------------------------
// FileHeader::UpdateFileLength
// 	Just update the file length without allocating new sectors.
//  Used for temporary updates or when sectors are already allocated.
//
//	"newLength" is the new length of the file in bytes
//----------------------------------------------------------------------

void
FileHeader::UpdateFileLength(int newLength)
{
    numBytes = newLength;
}

//----------------------------------------------------------------------
// FileHeader::ExtendFileSize
// 	Extend the size of the file to accommodate new data and allocate sectors.
//	Return TRUE if the extension is successful, FALSE otherwise.
//
//	"freeMap" is the bit map of free disk sectors
//	"newSize" is the new size of the file in bytes
//----------------------------------------------------------------------

bool
FileHeader::ExtendFileSize(BitMap *freeMap, int newSize)
{
    if (newSize <= numBytes) {
        return TRUE;  // No extension needed
    }

    int oldNumSectors = GetNumSectors();
    int newNumSectors = divRoundUp(newSize, SectorSize);
    int sectorsNeeded = newNumSectors - oldNumSectors;

    if (newNumSectors > (NumDirect + NumIndirect)) {
        return FALSE;  // Cannot extend beyond maximum file size
    }

    if (sectorsNeeded > 0) {
        if (oldNumSectors <= NumDirect && newNumSectors <= NumDirect) {
            // Case 1: Both old and new sizes use only direct blocks
            for (int i = oldNumSectors; i < newNumSectors; i++) {
                dataSectors[i] = freeMap->Find();
                if (dataSectors[i] == -1) {
                    // Not enough free sectors, deallocate the ones we just allocated
                    for (int j = oldNumSectors; j < i; j++) {
                        freeMap->Clear(dataSectors[j]);
                        dataSectors[j] = 0;
                    }
                    return FALSE;
                }
            }
        } else if (oldNumSectors <= NumDirect && newNumSectors > NumDirect) {
            // Case 2: Growing from direct-only to using indirect blocks
            int indirectSectors[NumIndirect];
            
            // Fill remaining direct blocks
            for (int i = oldNumSectors; i < NumDirect; i++) {
                dataSectors[i] = freeMap->Find();
                if (dataSectors[i] == -1) {
                    // Not enough free sectors, deallocate the ones we just allocated
                    for (int j = oldNumSectors; j < i; j++) {
                        freeMap->Clear(dataSectors[j]);
                        dataSectors[j] = 0;
                    }
                    return FALSE;
                }
            }
            
            // Allocate indirect block
            int indirectSector = freeMap->Find();
            if (indirectSector == -1) {
                // Not enough free sectors, deallocate direct blocks we just allocated
                for (int i = oldNumSectors; i < NumDirect; i++) {
                    freeMap->Clear(dataSectors[i]);
                    dataSectors[i] = 0;
                }
                return FALSE;
            }
            dataSectors[NumDirect] = indirectSector;
            
            // Allocate sectors for indirect block
            int numIndirect = newNumSectors - NumDirect;
            for (int i = 0; i < numIndirect; i++) {
                indirectSectors[i] = freeMap->Find();
                if (indirectSectors[i] == -1) {
                    // Not enough free sectors, deallocate everything
                    for (int j = oldNumSectors; j < NumDirect; j++) {
                        freeMap->Clear(dataSectors[j]);
                        dataSectors[j] = 0;
                    }
                    freeMap->Clear(indirectSector);
                    dataSectors[NumDirect] = -1;
                    return FALSE;
                }
            }
            
            // Write indirect block to disk
            synchDisk->WriteSector(indirectSector, (char *)indirectSectors);
        } else {
            // Case 3: Both old and new sizes use indirect blocks
            int indirectSectors[NumIndirect];
            synchDisk->ReadSector(dataSectors[NumDirect], (char *)indirectSectors);
            
            int oldIndirect = oldNumSectors - NumDirect;
            int newIndirect = newNumSectors - NumDirect;
            
            for (int i = oldIndirect; i < newIndirect; i++) {
                indirectSectors[i] = freeMap->Find();
                if (indirectSectors[i] == -1) {
                    // Not enough free sectors, deallocate the ones we just allocated
                    for (int j = oldIndirect; j < i; j++) {
                        freeMap->Clear(indirectSectors[j]);
                        indirectSectors[j] = 0;
                    }
                    return FALSE;
                }
            }
            
            // Write updated indirect block to disk
            synchDisk->WriteSector(dataSectors[NumDirect], (char *)indirectSectors);
        }
    }

    numBytes = newSize;
    SetModifyTime();  // Update modification time when file is extended
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];
    int numSectors = GetNumSectors();

    printf("FileHeader contents.  File size: %d.  File modification time: ", numBytes);
    PrintTime(modifyTime);
    printf(".  File blocks:\n");
    
    if (numSectors <= NumDirect) {
	// Only direct blocks
	for (i = 0; i < numSectors; i++)
	    printf("%d ", dataSectors[i]);
    } else {
	// Direct and indirect blocks
	for (i = 0; i < NumDirect; i++)
	    printf("%d ", dataSectors[i]);
	
	// Read and print indirect block sectors
	int indirectSectors[NumIndirect];
	synchDisk->ReadSector(dataSectors[NumDirect], (char *)indirectSectors);
	printf("Index2: ");
	for (i = 0; i < numSectors - NumDirect; i++)
	    printf("%d ", indirectSectors[i]);
    }
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) {
	int sector;
	if (i < NumDirect) {
	    sector = dataSectors[i];
	} else {
	    int indirectSectors[NumIndirect];
	    synchDisk->ReadSector(dataSectors[NumDirect], (char *)indirectSectors);
	    sector = indirectSectors[i - NumDirect];
	}
	synchDisk->ReadSector(sector, data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		printf("%c", data[j]);
            else
		printf("\\%x", (unsigned char)data[j]);
	}
        printf("\n"); 
    }
    delete [] data;
}
