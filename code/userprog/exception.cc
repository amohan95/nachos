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
#include "../threads/system.h"
#include "syscall.h"
#include "../threads/synch.h"
#include <stdio.h>
#include <iostream>

using namespace std;

int copyin(unsigned int vaddr, int len, char *buf) {
  // Copy len bytes from the current thread's virtual address vaddr.
  // Return the number of bytes so read, or -1 if an error occors.
  // Errors can generally mean a bad virtual address was passed in.
  bool result;
  int n=0;			// The number of bytes copied in
  int *paddr = new int;

  while ( n >= 0 && n < len) {
    result = machine->ReadMem( vaddr, 1, paddr );
    while(!result) // FALL 09 CHANGES
    {
      result = machine->ReadMem( vaddr, 1, paddr ); // FALL 09 CHANGES: TO HANDLE PAGE FAULT IN THE ReadMem SYS CALL
    }	

    buf[n++] = *paddr;

    if ( !result ) {
      //translation failed
      return -1;
    }

    vaddr++;
  }

  delete paddr;
  return len;
}

int copyout(unsigned int vaddr, int len, char *buf) {
  // Copy len bytes to the current thread's virtual address vaddr.
  // Return the number of bytes so written, or -1 if an error
  // occors.  Errors can generally mean a bad virtual address was
  // passed in.
  bool result;
  int n=0;			// The number of bytes copied in

  while ( n >= 0 && n < len) {
    // Note that we check every byte's address
    result = machine->WriteMem( vaddr, 1, (int)(buf[n++]) );

    if ( !result ) {
      //translation failed
      return -1;
    }

    vaddr++;
  }

  return n;
}

void Create_Syscall(unsigned int vaddr, int len) {
  // Create the file with the name in the user buffer pointed to by
  // vaddr.  The file name is at most MAXFILENAME chars long.  No
  // way to return errors, though...
  char *buf = new char[len+1];	// Kernel buffer to put the name in

  if (!buf) return;

  if( copyin(vaddr,len,buf) == -1 ) {
    printf("%s","Bad pointer passed to Create\n");
    delete buf;
    return;
  }

  buf[len]='\0';

  fileSystem->Create(buf,0);
  delete[] buf;
  return;
}

int Open_Syscall(unsigned int vaddr, int len) {
  // Open the file with the name in the user buffer pointed to by
  // vaddr.  The file name is at most MAXFILENAME chars long.  If
  // the file is opened successfully, it is put in the address
  // space's file table and an id returned that can find the file
  // later.  If there are any errors, -1 is returned.
  char *buf = new char[len+1];	// Kernel buffer to put the name in
  OpenFile *f;			// The new open file
  int id;				// The openfile id

  if (!buf) {
    printf("%s","Can't allocate kernel buffer in Open\n");
    return -1;
  }

  if( copyin(vaddr,len,buf) == -1 ) {
    printf("%s","Bad pointer passed to Open\n");
    delete[] buf;
    return -1;
  }

  buf[len]='\0';

  f = fileSystem->Open(buf);
  delete[] buf;

  if ( f ) {
    if ((id = currentThread->space->fileTable.Put(f)) == -1 )
      delete f;
    return id;
  }
  else
    return -1;
}

void Write_Syscall(unsigned int vaddr, int len, int id) {
  // Write the buffer to the given disk file.  If ConsoleOutput is
  // the fileID, data goes to the synchronized console instead.  If
  // a Write arrives for the synchronized Console, and no such
  // console exists, create one. For disk files, the file is looked
  // up in the current address space's open file table and used as
  // the target of the write.

  char *buf;		// Kernel buffer for output
  OpenFile *f;	// Open file for output

  if ( id == ConsoleInput) return;

  if ( !(buf = new char[len]) ) {
    printf("%s","Error allocating kernel buffer for write!\n");
    return;
  } else {
    if ( copyin(vaddr,len,buf) == -1 ) {
      printf("%s","Bad pointer passed to to write: data not written\n");
      delete[] buf;
      return;
    }
  }

  if ( id == ConsoleOutput) {
    for (int ii=0; ii<len; ii++) {
      printf("%c",buf[ii]);
    }

  } else {
    if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
      f->Write(buf, len);
    } else {
      printf("%s","Bad OpenFileId passed to Write\n");
      len = -1;
    }
  }

  delete[] buf;
}

int Read_Syscall(unsigned int vaddr, int len, int id) {
  // Write the buffer to the given disk file.  If ConsoleOutput is
  // the fileID, data goes to the synchronized console instead.  If
  // a Write arrives for the synchronized Console, and no such
  // console exists, create one.    We reuse len as the number of bytes
  // read, which is an unnessecary savings of space.
  char *buf;		// Kernel buffer for input
  OpenFile *f;	// Open file for output

  if ( id == ConsoleOutput) return -1;

  if ( !(buf = new char[len]) ) {
    printf("%s","Error allocating kernel buffer in Read\n");
    return -1;
  }

  if ( id == ConsoleInput) {
    //Reading from the keyboard
    scanf("%s", buf);

    if ( copyout(vaddr, len, buf) == -1 ) {
      printf("%s","Bad pointer passed to Read: data not copied\n");
    }
  } else {
    if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
      len = f->Read(buf, len);
      if ( len > 0 ) {
        //Read something from the file. Put into user's address space
        if ( copyout(vaddr, len, buf) == -1 ) {
          printf("%s","Bad pointer passed to Read: data not copied\n");
        }
      }
    } else {
      printf("%s","Bad OpenFileId passed to Read\n");
      len = -1;
    }
  }

  delete[] buf;
  return len;
}

void Close_Syscall(int fd) {
  // Close the file associated with id fd.  No error reporting.
  OpenFile *f = (OpenFile *) currentThread->space->fileTable.Remove(fd);

  if ( f ) {
    delete f;
  } else {
    printf("%s","Tried to close an unopen file\n");
  }
}

void KernelThread(int vaddr) {
  int stack_addr = currentThread->space->AllocateStackPages();
  if (stack_addr == -1) { 
    printf("Unable to allocate more stack for thread %s.",
           currentThread->getName());
    return;
  }
  
  currentThread->space->InitRegisters();
  currentThread->space->RestoreState();

  machine->WriteRegister(PCReg, vaddr);
  machine->WriteRegister(NextPCReg, machine->ReadRegister(PCReg) + 4);
  machine->WriteRegister(StackReg, stack_addr);

  machine->Run();
  ASSERT(false); // Should never get past machine->Run()
}

void Fork_Syscall(int vaddr) {
  Thread* thread = new Thread("Forked Thread");
  thread->space = currentThread->space;
  processTableLock->Acquire();
  processThreadTable[thread->space] += 1;
  processTableLock->Release();
  thread->Fork(KernelThread, vaddr);
}

void KernelProcess(int dummy) {
  currentThread->space->InitRegisters();   // set the initial register values
  currentThread->space->RestoreState();    // load page table register

  machine->Run();     // jump to the user progam
  ASSERT(FALSE);      // machine->Run never returns;
        // the address space exits
        // by doing the syscall "exit"
}

void Exec_Syscall(int vaddr, int len) {
  char* name = new char[len + 1];
  int res = copyin(vaddr, len, name);
  if (res == -1) {
    printf("Incorrect virtual address passed to Exec.\n");
    delete name;
    return;
  }

  OpenFile* executable = fileSystem->Open(name);
  if (executable == NULL) {
    printf("Unable to open file %s\n", name);
    return;
  }
  delete executable;

  Thread* thread = new Thread("New Process Thread");
  thread->space = new AddrSpace(name);
  thread->stack_vaddr_bottom_ =
      thread->space->num_pages() - divRoundUp(UserStackSize, PageSize);

  processTableLock->Acquire();
  processThreadTable[thread->space] += 1;
  processTableLock->Release();

  thread->Fork(KernelProcess, 0);
}

void Exit_Syscall(int status) {
  // A thread cannot be executing if it doesn't belong to a process in the system
  //   process table
  processTableLock->Acquire();
  ASSERT(processThreadTable.find(currentThread->space)
      != processThreadTable.end());
  if (processThreadTable[currentThread->space] > 1) {
    // Not the last thread in the process
    currentThread->space->DeallocateStack();
    processThreadTable[currentThread->space] -= 1;
  } else {
    currentThread->space->DeallocateAllPages();
    processThreadTable.erase(currentThread->space);
  }
  if (processThreadTable.size() > 0) {
    // Not the last process running
    processTableLock->Release();
    currentThread->Finish();
  } else {
    processTableLock->Release();
    interrupt->Halt();
  }
}

void Yield_Syscall() {
  currentThread->Yield();
}

int CreateLock_Syscall(int name, int len) {
  lockTableLock->Acquire();
  for (int i = 0; i < NUM_SYSTEM_LOCKS; ++i) {
    if (lockTable[i] == NULL) {
      lockTable[i] = new KernelLock();
      // Allocate memory to store name of lock in kernel memory
      lockTable[i]->name = new char[len + 1];
      copyin(name, len, lockTable[i]->name);
      // printf("0x%x %d %s\n", currentThread->space, i, lockTable[i]->name);
      lockTable[i]->lock = new Lock(lockTable[i]->name);
      lockTable[i]->addrSpace = currentThread->space;
      lockTable[i]->toBeDeleted = false;
      lockTable[i]->threadsUsing = 0;
      lockTableLock->Release();
      return i;
    }
  }
  // Didn't find any available entries in system lock table
  lockTableLock->Release();
  return UNSUCCESSFUL_SYSCALL;
}

int DestroyLock_Syscall(int lock) {
  lockTableLock->Acquire();
  // Lock must:
  //   - be a valid index in the system lock table
  //   - be allocated
  //   - not marked for deletion already
  //   - owned by the current process
  if (lock < 0 || lock >= NUM_SYSTEM_LOCKS
      || lockTable[lock] == NULL || lockTable[lock]->toBeDeleted
      || lockTable[lock]->addrSpace != currentThread->space) {
    lockTableLock->Release();
    return UNSUCCESSFUL_SYSCALL;
  }
  if (lockTable[lock]->threadsUsing == 0) {
    // Deallocate since no more threads are using lock
    delete lockTable[lock]->lock;
    delete [] lockTable[lock]->name;
    delete lockTable[lock];
    lockTable[lock] = NULL;
  } else {
    // Mark for deletion when no longer in use
    lockTable[lock]->toBeDeleted = true;
  }
  lockTableLock->Release();
  return 0;
}

int CreateCondition_Syscall(int name, int len) {
  conditionTableLock->Acquire();
  for (int i = 0; i < NUM_SYSTEM_CONDITIONS; ++i) {
    if (conditionTable[i] == NULL) {
      conditionTable[i] = new KernelCondition();
      conditionTable[i]->name = new char[len];
      copyin(name, len, conditionTable[i]->name);
      conditionTable[i]->condition = new Condition(conditionTable[i]->name);
      conditionTable[i]->addrSpace = currentThread->space;
      conditionTable[i]->toBeDeleted = false;
      conditionTable[i]->threadsUsing = 0;
      conditionTableLock->Release();
      return i;
    }
  }
  conditionTableLock->Release();
  return UNSUCCESSFUL_SYSCALL;
}

int DestroyCondition_Syscall(int cv) {
  conditionTableLock->Acquire();
  if (cv < 0 || cv >= NUM_SYSTEM_CONDITIONS
      || conditionTable[cv] == NULL || conditionTable[cv]->toBeDeleted
      || conditionTable[cv]->addrSpace != currentThread->space) {
    conditionTableLock->Release();
    return UNSUCCESSFUL_SYSCALL;
  }
  if (conditionTable[cv]->threadsUsing == 0) {
    delete conditionTable[cv]->condition;
    delete [] conditionTable[cv]->name;
    delete conditionTable[cv];
    conditionTable[cv] = NULL;
  } else {
    conditionTable[cv]->toBeDeleted = true;
  }
  conditionTableLock->Release();
  return 0;
}

int Acquire_Syscall(int lock) {
  lockTableLock->Acquire();
  if (lock < 0 || lock >= NUM_SYSTEM_LOCKS
      || lockTable[lock] == NULL || lockTable[lock]->toBeDeleted
      || lockTable[lock]->addrSpace != currentThread->space) {
    lockTableLock->Release();
    return UNSUCCESSFUL_SYSCALL;
  }
  int threadsUsing = ++lockTable[lock]->threadsUsing;
  lockTableLock->Release();
  lockTable[lock]->lock->Acquire();
  return threadsUsing;
}

int Release_Syscall(int lock) {
  lockTableLock->Acquire();
  if (lock < 0 || lock >= NUM_SYSTEM_LOCKS
      || lockTable[lock] == NULL || lockTable[lock]->toBeDeleted
      || lockTable[lock]->addrSpace != currentThread->space) {
    lockTableLock->Release();
    return UNSUCCESSFUL_SYSCALL;
  }
  lockTable[lock]->lock->Release();
  int threadsUsing = --lockTable[lock]->threadsUsing;
  if (lockTable[lock]->threadsUsing == 0 && lockTable[lock]->toBeDeleted) {
    // Deallocate if marked for deletion and no more threads using it
    delete lockTable[lock]->lock;
    delete [] lockTable[lock]->name;
    delete lockTable[lock];
    lockTable[lock] = NULL;
  }
  lockTableLock->Release();
  return threadsUsing;
}

int Wait_Syscall(int cv, int lock) {
  conditionTableLock->Acquire();
  if (cv < 0 || cv >= NUM_SYSTEM_CONDITIONS
      || conditionTable[cv] == NULL || conditionTable[cv]->toBeDeleted
      || conditionTable[cv]->addrSpace != currentThread->space) {
    conditionTableLock->Release();
    return UNSUCCESSFUL_SYSCALL;
  }
  if (lock < 0 || lock >= NUM_SYSTEM_LOCKS
      || lockTable[lock] == NULL || lockTable[lock]->toBeDeleted
      || lockTable[lock]->addrSpace != currentThread->space) {
    conditionTableLock->Release();
    return UNSUCCESSFUL_SYSCALL;
  }
  int threadsUsing = ++conditionTable[cv]->threadsUsing;
  conditionTableLock->Release();
  lockTableLock->Acquire();
  // Need to increment number of threads using lock as well,
  //   because we cannot allow lock to be deallocated while
  //   condition is still using it
  ++lockTable[lock]->threadsUsing;
  lockTableLock->Release();
  conditionTable[cv]->condition->Wait(lockTable[lock]->lock);
  return threadsUsing;
}

int Signal_Syscall(int cv, int lock) {
  conditionTableLock->Acquire();
  if (cv < 0 || cv >= NUM_SYSTEM_CONDITIONS
      || conditionTable[cv] == NULL || conditionTable[cv]->toBeDeleted
      || conditionTable[cv]->addrSpace != currentThread->space) {
    return UNSUCCESSFUL_SYSCALL;
  }
  if (lock < 0 || lock >= NUM_SYSTEM_LOCKS
      || lockTable[lock] == NULL || lockTable[lock]->toBeDeleted
      || lockTable[lock]->addrSpace != currentThread->space) {
    return UNSUCCESSFUL_SYSCALL;
  }
  int threadsUsing = --conditionTable[cv]->threadsUsing;
  conditionTable[cv]->condition->Signal(lockTable[lock]->lock);
  if (conditionTable[cv]->threadsUsing == 0 && conditionTable[cv]->toBeDeleted) {
    delete conditionTable[cv]->condition;
    delete [] conditionTable[cv]->name;
    delete conditionTable[cv];
    conditionTable[cv] = NULL;
  }
  conditionTableLock->Release();
  lockTableLock->Acquire();
  --lockTable[lock]->threadsUsing;
  if (lockTable[lock]->threadsUsing == 0 && lockTable[lock]->toBeDeleted) {
    delete lockTable[lock]->lock;
    delete [] lockTable[lock]->name;
    delete lockTable[lock];
    lockTable[lock] = NULL;
  }
  lockTableLock->Release();
  return threadsUsing;
}

int Broadcast_Syscall(int cv, int lock) {
  conditionTableLock->Acquire();
  if (cv < 0 || cv >= NUM_SYSTEM_CONDITIONS
      || conditionTable[cv] == NULL || conditionTable[cv]->toBeDeleted
      || conditionTable[cv]->addrSpace != currentThread->space) {
    return UNSUCCESSFUL_SYSCALL;
  }
  if (lock < 0 || lock >= NUM_SYSTEM_LOCKS
      || lockTable[lock] == NULL || lockTable[lock]->toBeDeleted
      || lockTable[lock]->addrSpace != currentThread->space) {
    return UNSUCCESSFUL_SYSCALL;
  }
  int formerThreadsUsing = conditionTable[cv]->threadsUsing;
  conditionTable[cv]->threadsUsing = 0;
  conditionTable[cv]->condition->Broadcast(lockTable[lock]->lock);
  if (conditionTable[cv]->toBeDeleted) {
    delete conditionTable[cv]->condition;
    delete [] conditionTable[cv]->name;
    delete conditionTable[cv];
    conditionTable[cv] = NULL;
  }
  conditionTableLock->Release();
  lockTableLock->Acquire();
  // Lock might be in use by other threads not related to this condition,
  //   so we must be careful to only decrement by number of threads that were using
  //   condition
  lockTable[lock]->threadsUsing -= formerThreadsUsing;
  if (lockTable[lock]->threadsUsing == 0 && lockTable[lock]->toBeDeleted) {
    delete lockTable[lock]->lock;
    delete [] lockTable[lock]->name;
    delete lockTable[lock];
    lockTable[lock] = NULL;
  }
  lockTableLock->Release();
  return 0;
}

int Rand_Syscall() {
  return rand();
}

void Print_Syscall(unsigned int c, int len) {
  char *buf;
  if ( !(buf = new char[len]) ) {
    printf("%s","Error allocating kernel buffer for write!\n");
    return;
  } else {
    if ( copyin(c,len,buf) == -1 ) {
      printf("%s","Bad pointer passed to to write: data not written\n");
      delete[] buf;
      return;
    }
  }
  for (int i=0; i<len; i++) {
    printf("%c",buf[i]);
  }
  delete[] buf;
}

void PrintNum_Syscall(int num) {
  printf("%d", num);
}

int HandleMemoryFull() {
  int ppn = -1;
  switch (evictionPolicy) {
    case FIFO:
      for (int i = 0; i < NumPhysPages; ++i) {
        
      }
      break;
    case RAND:
      break;
    default:
      printf("Unknown eviction policy %d.", evictionPolicy);
      break;
  }
  return ppn;
}

int HandleIPTMiss(int vpn) {
  int ppn = page_manager->ObtainFreePage();

  if (ppn == -1) {
    ppn = HandleMemoryFull();
  }

  currentThread->space->LoadPage(vpn, ppn);

  return ppn;
}

void HandlePageFault(int vpn) {
  int page_num = vpn / PageSize;

  // Set interrupts off for the entire page fault handling process, as per
  // Crowley's instructions.
  InterruptSetter is;

  int ppn = -1;
  // Search IPT
  if (ppn = -1) {
    ppn = HandleIPTMiss(page_num);
  }

  currentThread->space->PopulateTlbEntry(page_num);
}

void ExceptionHandler(ExceptionType which) {
  int type = machine->ReadRegister(2); // Which syscall?
  int rv=0; 	// the return value from a syscall

  if ( which == SyscallException ) {
    switch (type) {
      default:
        DEBUG('s', "Unknown syscall - shutting down.\n");
      case SC_Halt:
        DEBUG('s', "Shutdown, initiated by user program.\n");
        interrupt->Halt();
        break;
      case SC_Exit:
        DEBUG('s', "Exit syscall.\n");
        int status = machine->ReadRegister(4);
        Exit_Syscall(status);
        break;
      case SC_Exec:
        DEBUG('s', "Exec syscall.\n");
        Exec_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
        break;
      case SC_Create:
        DEBUG('s', "Create syscall.\n");
        Create_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
        break;
      case SC_Open:
        DEBUG('s', "Open syscall.\n");
        rv = Open_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
        break;
      case SC_Write:
        DEBUG('s', "Write syscall.\n");
        Write_Syscall(machine->ReadRegister(4),
            machine->ReadRegister(5),
            machine->ReadRegister(6));
        break;
      case SC_Read:
        DEBUG('s', "Read syscall.\n");
        rv = Read_Syscall(machine->ReadRegister(4),
            machine->ReadRegister(5),
            machine->ReadRegister(6));
        break;
      case SC_Close:
        DEBUG('s', "Close syscall.\n");
        Close_Syscall(machine->ReadRegister(4));
        break;
      case SC_Fork:
        DEBUG('s', "Fork syscall.\n");
        Fork_Syscall(machine->ReadRegister(4));
        break;
      case SC_Yield:
        DEBUG('s', "Yield syscall.\n");
        Yield_Syscall();
        break;
      case SC_CreateLock:
        DEBUG('s', "CreateLock syscall.\n");
        rv = CreateLock_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
        break;
      case SC_DestroyLock:
        DEBUG('s', "DestroyLock syscall.\n");
        int lock = machine->ReadRegister(4);
        rv = DestroyLock_Syscall(lock);
        break;
      case SC_CreateCondition:
        DEBUG('s', "CreateCondition syscall.\n");
        rv = CreateCondition_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
        break;
      case SC_DestroyCondition:
        DEBUG('s', "DestroyCondition syscall.\n");
        int cv = machine->ReadRegister(4);
        rv = DestroyCondition_Syscall(cv);
        break;
      case SC_Acquire:
        DEBUG('s', "Acquire syscall.\n");
        lock = machine->ReadRegister(4);
        rv = Acquire_Syscall(lock);
        break;
      case SC_Release:
        DEBUG('s', "Release syscall.\n");
        lock = machine->ReadRegister(4);
        rv = Release_Syscall(lock);
        break;
      case SC_Wait:
        DEBUG('s', "Wait syscall.\n");
        cv = machine->ReadRegister(4);
        lock = machine->ReadRegister(5);
        rv = Wait_Syscall(cv, lock);
        break;
      case SC_Signal:
        DEBUG('s', "Signal syscall.\n");
        cv = machine->ReadRegister(4);
        lock = machine->ReadRegister(5);
        rv = Signal_Syscall(cv, lock);
        break;
      case SC_Broadcast:
        DEBUG('s', "Broadcast syscall.\n");
        cv = machine->ReadRegister(4);
        lock = machine->ReadRegister(5);
        rv = Broadcast_Syscall(cv, lock);
        break;
      case SC_Rand:
        DEBUG('s', "Rand syscall.\n");
        rv = Rand_Syscall();
        break;
      case SC_Print:
        DEBUG('s', "Print string syscall.\n");
        Print_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
        break;
      case SC_PrintNum:
        DEBUG('s', "Print number syscall.\n");
        PrintNum_Syscall(machine->ReadRegister(4));
        break; 
    }

    // Put in the return value and increment the PC
    machine->WriteRegister(2,rv);
    machine->WriteRegister(PrevPCReg,machine->ReadRegister(PCReg));
    machine->WriteRegister(PCReg,machine->ReadRegister(NextPCReg));
    machine->WriteRegister(NextPCReg,machine->ReadRegister(PCReg)+4);
    return;
  } else if (which == PageFaultException) {
    HandlePageFault(machine->ReadRegister(BadVAddrReg));
  } else {
    cout<<"Unexpected user mode exception - which:"<<which<<"  type:"<< type<<endl;
    interrupt->Halt();
  }
}
