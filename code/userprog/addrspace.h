// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "table.h"
#include "swappable_translation_entry.h"

#define UserStackSize		1024 	// increase this as necessary!

#define MaxOpenFiles 256
#define MaxChildSpaces 256

class AddrSpace {
  public:
    AddrSpace(char* execFile);	// Create an address space,
					// initializing it with the program
					// stored in the file "executable"
    ~AddrSpace();			// De-allocate an address space

    void InitRegisters();		// Initialize user-level CPU registers,
					// before jumping to user code

    // Adds new stack pages to a thread for Forking. This copies over the memory
    // from the old page table and adds eight more pages of stack. Returns -1
    // if there are not enough pages available to allocate stack. Otherwise,
    // it returns the starting position of the new stack for the thread.
    int AllocateStackPages();

    // Deallocates the stack assigned to the current thread.
    void DeallocateStack();

    // Deallocates all pages assigned to the current process.
    void DeallocateAllPages();

    // Copies the values for a virtual page into the TLB and increments the
    // currentTlb counter.
    void PopulateTlbEntry(int page_num);

    // Used when a context switch occurs and the address spaces change.
    // Invalidates all the entries in the TLB, and if there is a dirty page
    // it propagates this bit to its own page table.
    void InvalidateTlb();

    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch
    Table fileTable;			// Table of openfiles

    int num_pages() { return numPages; }

		void LoadPage(int vpn, int ppn);
 private:
    SwappableTranslationEntry* pageTable;
		// Assume linear page table translation
					// for now!
    int numPages;		// Number of pages in the virtual address space

    // Storage to keep the state of the registers when context switching.
    int program_registers_[NumTotalRegs];

		OpenFile* executable;

};

#endif // ADDRSPACE_H
