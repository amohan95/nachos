// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// RAII class that disables interrupts when initialized and restores them when
// the destructor is called.
class InterruptSetter {
 public:
  InterruptSetter() { old_level_ = interrupt->SetLevel(IntOff); }
  ~InterruptSetter() { interrupt->SetLevel(old_level_); }
 private:
  IntStatus old_level_;  
};

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
Lock::Lock(char* debug_name)
    : name(debug_name), lock_state_(LockState::kFree) {}

Lock::~Lock() {}

bool Lock::IsHeldByCurrentThread() {
  return currentThread == lock_owner_;
}

void Lock::AddToWaitQueue(Thread* thread) { wait_queue_.Append(thread); }

Thread* Lock::RemoveFromWaitQueue() { return (Thread*) wait_queue_.Remove(); }

void Lock::Acquire() {
  InterruptSetter is;
  if (IsHeldByCurrentThread()) {
    return;
  }
  if (lock_state_ == LockState::kFree) {
    lock_state_ = LockState::kBusy;
    lock_owner_ = currentThread;
  } else {
    AddToWaitQueue(currentThread);
    currentThread->Sleep();
  }
}

void Lock::Release() {
  InterruptSetter is;
  if (!IsHeldByCurrentThread()) {
    DEBUG('E', "Error: Thread %s trying to release lock owned by thread %s\n",
          currentThread->getName(),
          lock_owner_ ? lock_owner_->getName() : "null");
    return;
  }
  if (!wait_queue_.IsEmpty()) {
    Thread* to_wake = RemoveFromWaitQueue();
    lock_owner_ = to_wake;
    scheduler->ReadyToRun(lock_owner_);
  } else {
    lock_state_ = LockState::kFree;
    lock_owner_ = NULL;
  }
}

Condition::Condition(char* debug_name) : name(debug_name) { }

Condition::~Condition() { }

void Condition::Wait(Lock* lock) {
  InterruptSetter is;
  if (lock == NULL) {
      DEBUG('E', "Error: Thread %s trying to wait on null lock\n",
            currentThread->getName());    
    return;
  }
  if (waiting_lock_ == NULL) {
    waiting_lock_ = lock;
  }
  if (lock != waiting_lock_) {
    DEBUG('E', "Error: Thread %s trying to use condition %s "
               "with incorrect lock %s",
          currentThread->getName(), getName(), lock->getName());
    return;
  }
  lock->AddToWaitQueue(currentThread);
  lock->Release();
  currentThread->Sleep();
  lock->Acquire();
}

void Condition::Signal(Lock* lock) {
  InterruptSetter is;
  if (waiting_lock_ == NULL) {
    return;
  }
  if (waiting_lock_ != lock) {
    DEBUG('E', "Error: Thread %s trying to use condition %s "
               "with incorrect lock %s",
          currentThread->getName(), getName(), lock->getName());
    return;
  }
  Thread* to_wake = lock->RemoveFromWaitQueue();
  scheduler->ReadyToRun(to_wake);
  if (lock->wait_queue().IsEmpty()) {
    waiting_lock_ = NULL;
  }
}

void Condition::Broadcast(Lock* lock) {
  {
    InterruptSetter is;
    if (lock == NULL) {
      DEBUG('E', "Error: Thread %s trying to wait on null lock\n",
            currentThread->getName());    
      return;
    }
    if (waiting_lock_ != lock) {
      DEBUG('E', "Error: Thread %s trying to use condition %s "
                 "with incorrect lock %s",
            currentThread->getName(), getName(), lock->getName());
      return;
    }
  }
  while (!lock->wait_queue().IsEmpty()) {
    Signal(lock);
  }
}
