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

#include "../threads/copyright.h"
#include "../threads/system.h"
#include "../threads/synch.h"
#include "../machine/machine.h"
#include "addrspace.h"
#include "../bin/noff.h"
#include "table.h"

extern "C" { int bzero(char *, int); };

Table::Table(int s) : map(s), table(0), lock(0), size(s) {
    table = new void *[size];
    lock = new Lock("TableLock");
}

Table::~Table() {
    if (table) {
	delete table;
	table = 0;
    }
    if (lock) {
	delete lock;
	lock = 0;
    }
}

void *Table::Get(int i) {
    // Return the element associated with the given if, or 0 if
    // there is none.

    return (i >=0 && i < size && map.Test(i)) ? table[i] : 0;
}

int Table::Put(void *f) {
    // Put the element in the table and return the slot it used.  Use a
    // lock so 2 files don't get the same space.
    int i;	// to find the next slot

    lock->Acquire();
    i = map.Find();
    lock->Release();
    if ( i != -1)
	table[i] = f;
    return i;
}

void *Table::Remove(int i) {
    // Remove the element associated with identifier i from the table,
    // and return it.

    void *f =0;

    if ( i >= 0 && i < size ) {
	lock->Acquire();
	if ( map.Test(i) ) {
	    map.Clear(i);
	    f = table[i];
	    table[i] = 0;
	}
	lock->Release();
    }
    return f;
}

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
//	"executable" is the file containing the object code to load into memory
//
//      It's possible to fail to fully construct the address space for
//      several reasons, including being unable to allocate memory,
//      and being unable to read key parts of the executable.
//      Incompletely consretucted address spaces have the member
//      constructed set to false.
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable) : fileTable(MaxOpenFiles) {
  NoffHeader noffH;
  int size;

  // Don't allocate the input or output to disk files
  fileTable.Put(0);
  fileTable.Put(0);

  executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
  if ((noffH.noffMagic != NOFFMAGIC) && 
	(WordToHost(noffH.noffMagic) == NOFFMAGIC))
  	SwapHeader(&noffH);
  ASSERT(noffH.noffMagic == NOFFMAGIC);

  size = noffH.code.size + noffH.initData.size + noffH.uninitData.size;

  int data_code_pages = divRoundUp(size, PageSize); 
  numPages = data_code_pages + divRoundUp(UserStackSize, PageSize);

  size = numPages * PageSize;

  DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
				numPages, size);

  // first, set up the translation 
  pageTable = new TranslationEntry[numPages];
  for (int i = 0; i < numPages; i++) {
  	pageTable[i].virtualPage = i;
  	pageTable[i].physicalPage = page_manager->ObtainFreePage();
  	pageTable[i].valid = TRUE;
  	pageTable[i].use = FALSE;
  	pageTable[i].dirty = FALSE;
  	pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
				// a separate page, we could set its 
				// pages to be read-only
    
    if (i < data_code_pages) {
      executable->ReadAt(
        &(machine->mainMemory[pageTable[i].physicalPage * PageSize]),
        PageSize, PageSize * i + 40);
    }
  }
}


//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
//
//  Dealloate an address space.  release pages, page tables, files
//  and file tables
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    delete pageTable;
}

int AddrSpace::AllocateStackPages() {
  int num_new_pages = divRoundUp(UserStackSize, PageSize);
  
  TranslationEntry* new_page_table = 
      new TranslationEntry[numPages + num_new_pages];

  for (int i = 0; i < numPages; ++i) {
    new_page_table[i].virtualPage = pageTable[i].virtualPage;
    new_page_table[i].physicalPage = pageTable[i].physicalPage;
    new_page_table[i].valid = pageTable[i].valid;
    new_page_table[i].use = pageTable[i].use;
    new_page_table[i].dirty = pageTable[i].dirty;
    new_page_table[i].readOnly = pageTable[i].readOnly;
  }

  currentThread->stack_vaddr_bottom_ = numPages;

  for (int i = numPages; i < num_new_pages + numPages; ++i) {
    new_page_table[i].virtualPage = i;
    new_page_table[i].physicalPage = page_manager->ObtainFreePage();
    new_page_table[i].valid = TRUE;
    new_page_table[i].use = FALSE;
    new_page_table[i].dirty = FALSE;
    new_page_table[i].readOnly = FALSE;
  }

  delete [] pageTable;

  pageTable = new_page_table;

  numPages += num_new_pages;
  RestoreState();
  return numPages * PageSize - 16;
}

void AddrSpace::DeallocateStack() {
  int stack_bottom = currentThread->stack_vaddr_bottom_;
  int num_stack_pages = divRoundUp(UserStackSize, PageSize);
  for (int i = stack_bottom; i < stack_bottom + num_stack_pages; ++i) {
    // Invalidate the TLB entries for any stack pages deallocated.
    for (int j = 0; j < TLBSize; ++j) {
      TranslationEntry* entry = &(machine->tlb[j]);
      if (entry->valid && entry->virtualPage == i) {
        entry->valid = false;
      }
    }
    pageTable[i].valid = false;
    page_manager->FreePage(pageTable[i].physicalPage);
  }
}

void AddrSpace::DeallocateAllPages() {
  for (int i = 0; i < numPages; ++i) {
    // Invalidate the TLB entries for any stack pages deallocated.
    for (int j = 0; j < TLBSize; ++j) {
      TranslationEntry* entry = &(machine->tlb[j]);
      if (entry->valid && entry->virtualPage == i) {
        entry->valid = false;
      }
    }
    if (pageTable[i].valid) {
      page_manager->FreePage(pageTable[i].physicalPage);
    }
  }
}

void AddrSpace::PopulateTlbEntry(int page_num) {
  currentTlb = (++currentTlb) % TLBSize;
  machine->tlb[currentTlb].virtualPage = pageTable[page_num].virtualPage;
  machine->tlb[currentTlb].physicalPage = pageTable[page_num].physicalPage;  
  machine->tlb[currentTlb].valid = pageTable[page_num].valid;
  machine->tlb[currentTlb].use = pageTable[page_num].use;
  machine->tlb[currentTlb].dirty = pageTable[page_num].dirty;
  machine->tlb[currentTlb].readOnly = pageTable[page_num].readOnly;
}

void AddrSpace::InvalidateTlb() {
  for (int i = 0; i < TLBSize; ++i) {
    TranslationEntry* tlb_entry = &(machine->tlb[i]);
    if (tlb_entry->valid && tlb_entry->dirty) {
      pageTable[tlb_entry->virtualPage].dirty = true;
    }
    tlb_entry->valid = false;
  }
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

void
AddrSpace::InitRegisters()
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
    DEBUG('a', "Initializing stack register to %x\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    // machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}
