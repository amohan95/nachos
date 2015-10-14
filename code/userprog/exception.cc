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

KernelLock* lockTable[NUM_SYSTEM_LOCKS] = {NULL};
KernelCondition* conditionTable[NUM_SYSTEM_CONDITIONS] = {NULL};
std::map<AddrSpace*, uint32_t> processThreadTable =
    std::map<AddrSpace*, uint32_t>();

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
  currentThread->stack_vaddr_bottom_ =
      currentThread->space->num_pages() - divRoundUp(UserStackSize, PageSize);

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
  processThreadTable[thread->space] += 1;
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

  Thread* thread = new Thread("New Process Thread");
  thread->space = new AddrSpace(executable);
  thread->stack_vaddr_bottom_ =
      thread->space->num_pages() - divRoundUp(UserStackSize, PageSize);
  delete executable;

  processThreadTable[thread->space] += 1;

  thread->Fork(KernelProcess, 0);
}

void Exit_Syscall(int status) {
  assert(processThreadTable.find(currentThread->space)
      != processThreadTable.end());
  if (processThreadTable[currentThread->space] > 1) {
    currentThread->space->DeallocateStack();
    processThreadTable[currentThread->space] -= 1;
  } else {
    currentThread->space->DeallocateAllPages();
    processThreadTable.erase(currentThread->space);
  }
  if (processThreadTable.size() > 0) {
    currentThread->Finish();
  } else {
    interrupt->Halt();
  }
}

void Yield_Syscall() {
  currentThread->Yield();
}

int CreateLock_Syscall(char* name) {
  for (int i = 0; i < NUM_SYSTEM_LOCKS; ++i) {
    if (lockTable[i] == NULL) {
      lockTable[i] = new KernelLock();
      lockTable[i]->lock = new Lock(name);
      lockTable[i]->addrSpace = currentThread->space;
      lockTable[i]->toBeDeleted = false;
      return i;
    } else if (lockTable[i]->toBeDeleted) {
      lockTable[i]->addrSpace = currentThread->space;
      lockTable[i]->toBeDeleted = false;
      return i;
    }
  }
  return -1;
}

void DestroyLock_Syscall(int lock) {
  if (lock < 0 || lock >= NUM_SYSTEM_LOCKS
      || lockTable[lock] == NULL || lockTable[lock]->toBeDeleted
      || lockTable[lock]->addrSpace != currentThread->space) {
    return;
  }
  lockTable[lock]->toBeDeleted = true;
}

int CreateCondition_Syscall(char* name) {
  for (int i = 0; i < NUM_SYSTEM_CONDITIONS; ++i) {
    if (conditionTable[i] == NULL) {
      conditionTable[i] = new KernelCondition();
      conditionTable[i]->condition = new Condition(name);
      conditionTable[i]->addrSpace = currentThread->space;
      conditionTable[i]->toBeDeleted = false;
      return i;
    } else if (conditionTable[i]->toBeDeleted) {
      conditionTable[i]->addrSpace = currentThread->space;
      conditionTable[i]->toBeDeleted = false;
      return i;
    }
  }
  return -1;
}

void DestroyCondition_Syscall(int cv) {
  if (cv < 0 || cv >= NUM_SYSTEM_CONDITIONS
      || conditionTable[cv] == NULL || conditionTable[cv]->toBeDeleted
      || conditionTable[cv]->addrSpace != currentThread->space) {
    return;
  }
  conditionTable[cv]->toBeDeleted = true;
}

void Acquire_Syscall(int lock) {
  if (lock < 0 || lock >= NUM_SYSTEM_LOCKS
      || lockTable[lock] == NULL || lockTable[lock]->toBeDeleted
      || lockTable[lock]->addrSpace != currentThread->space) {
    return;
  }
  lockTable[lock]->lock->Acquire();
}

void Release_Syscall(int lock) {
  if (lock < 0 || lock >= NUM_SYSTEM_LOCKS
      || lockTable[lock] == NULL || lockTable[lock]->toBeDeleted
      || lockTable[lock]->addrSpace != currentThread->space) {
    return;
  }
  lockTable[lock]->lock->Release();
}

void Wait_Syscall(int cv, int lock) {
  if (cv < 0 || cv >= NUM_SYSTEM_CONDITIONS
      || conditionTable[cv] == NULL || conditionTable[cv]->toBeDeleted
      || conditionTable[cv]->addrSpace != currentThread->space) {
    return;
  }
  if (lock < 0 || lock >= NUM_SYSTEM_LOCKS
      || lockTable[lock] == NULL || lockTable[lock]->toBeDeleted
      || lockTable[lock]->addrSpace != currentThread->space) {
    return;
  }
  conditionTable[cv]->condition->Wait(lockTable[lock]->lock);
}

void Signal_Syscall(int cv, int lock) {
  if (cv < 0 || cv >= NUM_SYSTEM_CONDITIONS
      || conditionTable[cv] == NULL || conditionTable[cv]->toBeDeleted
      || conditionTable[cv]->addrSpace != currentThread->space) {
    return;
  }
  if (lock < 0 || lock >= NUM_SYSTEM_LOCKS
      || lockTable[lock] == NULL || lockTable[lock]->toBeDeleted
      || lockTable[lock]->addrSpace != currentThread->space) {
    return;
  }
  conditionTable[cv]->condition->Signal(lockTable[lock]->lock);
}

void Broadcast_Syscall(int cv, int lock) {
  if (cv < 0 || cv >= NUM_SYSTEM_CONDITIONS
      || conditionTable[cv] == NULL || conditionTable[cv]->toBeDeleted
      || conditionTable[cv]->addrSpace != currentThread->space) {
    return;
  }
  if (lock < 0 || lock >= NUM_SYSTEM_LOCKS
      || lockTable[lock] == NULL || lockTable[lock]->toBeDeleted
      || lockTable[lock]->addrSpace != currentThread->space) {
    return;
  }
  conditionTable[cv]->condition->Broadcast(lockTable[lock]->lock);
}

int Rand_Syscall() {
  return rand();
}

void ExceptionHandler(ExceptionType which) {
  int type = machine->ReadRegister(2); // Which syscall?
  int rv=0; 	// the return value from a syscall

  if ( which == SyscallException ) {
    switch (type) {
      default:
        DEBUG('a', "Unknown syscall - shutting down.\n");
      case SC_Halt:
        DEBUG('a', "Shutdown, initiated by user program.\n");
        interrupt->Halt();
        break;
      case SC_Exit:
        DEBUG('a', "Exit syscall.\n");
        int status = machine->ReadRegister(4);
        Exit_Syscall(status);
        break;
      case SC_Exec:
        DEBUG('a', "Exec syscall.\n");
        Exec_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
        break;
      case SC_Create:
        DEBUG('a', "Create syscall.\n");
        Create_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
        break;
      case SC_Open:
        DEBUG('a', "Open syscall.\n");
        rv = Open_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
        break;
      case SC_Write:
        DEBUG('a', "Write syscall.\n");
        Write_Syscall(machine->ReadRegister(4),
            machine->ReadRegister(5),
            machine->ReadRegister(6));
        break;
      case SC_Read:
        DEBUG('a', "Read syscall.\n");
        rv = Read_Syscall(machine->ReadRegister(4),
            machine->ReadRegister(5),
            machine->ReadRegister(6));
        break;
      case SC_Close:
        DEBUG('a', "Close syscall.\n");
        Close_Syscall(machine->ReadRegister(4));
        break;
      case SC_Fork:
        DEBUG('a', "Fork syscall.\n");
        Fork_Syscall(machine->ReadRegister(4));
        break;
      case SC_Yield:
        DEBUG('a', "Yield syscall.\n");
        Yield_Syscall();
        break;
      case SC_CreateLock:
        DEBUG('a', "CreateLock syscall.\n");
        char* name = reinterpret_cast<char*>(machine->ReadRegister(4));
        rv = CreateLock_Syscall(name);
        break;
      case SC_DestroyLock:
        DEBUG('a', "DestroyLock syscall.\n");
        int lock = machine->ReadRegister(4);
        DestroyLock_Syscall(lock);
        break;
      case SC_CreateCondition:
        DEBUG('a', "CreateCondition syscall.\n");
        name = reinterpret_cast<char*>(machine->ReadRegister(4));
        rv =CreateCondition_Syscall(name);
        break;
      case SC_DestroyCondition:
        DEBUG('a', "DestroyCondition syscall.\n");
        int cv = machine->ReadRegister(4);
        DestroyCondition_Syscall(cv);
        break;
      case SC_Acquire:
        DEBUG('a', "Acquire syscall.\n");
        lock = machine->ReadRegister(4);
        Acquire_Syscall(lock);
        break;
      case SC_Release:
        DEBUG('a', "Release syscall.\n");
        lock = machine->ReadRegister(4);
        Release_Syscall(lock);
        break;
      case SC_Wait:
        DEBUG('a', "Wait syscall.\n");
        cv = machine->ReadRegister(4);
        lock = machine->ReadRegister(5);
        Wait_Syscall(cv, lock);
        break;
      case SC_Signal:
        DEBUG('a', "Signal syscall.\n");
        cv = machine->ReadRegister(4);
        lock = machine->ReadRegister(5);
        Signal_Syscall(cv, lock);
        break;
      case SC_Broadcast:
        DEBUG('a', "Broadcast syscall.\n");
        cv = machine->ReadRegister(4);
        lock = machine->ReadRegister(5);
        Broadcast_Syscall(cv, lock);
        break;
      case SC_Rand:
        DEBUG('a', "Rand syscall.\n");
        rv = Rand_Syscall();
        break;        
    }

    // Put in the return value and increment the PC
    machine->WriteRegister(2,rv);
    machine->WriteRegister(PrevPCReg,machine->ReadRegister(PCReg));
    machine->WriteRegister(PCReg,machine->ReadRegister(NextPCReg));
    machine->WriteRegister(NextPCReg,machine->ReadRegister(PCReg)+4);
    return;
  } else {
    cout<<"Unexpected user mode exception - which:"<<which<<"  type:"<< type<<endl;
    interrupt->Halt();
  }
}
