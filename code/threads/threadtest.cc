// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <deque>

#include "copyright.h"
#include "system.h"
//  Simple test cases for the threads assignment.
//
#ifdef CHANGED
#include "synch.h"
#endif

#ifdef CHANGED
// --------------------------------------------------
// Test Suite
// --------------------------------------------------


// --------------------------------------------------
// Test 1 - see TestSuite() for details
// --------------------------------------------------
Semaphore t1_s1("t1_s1",0);       // To make sure t1_t1 acquires the
                                  // lock before t1_t2
Semaphore t1_s2("t1_s2",0);       // To make sure t1_t2 Is waiting on the 
                                  // lock before t1_t3 releases it
Semaphore t1_s3("t1_s3",0);       // To make sure t1_t1 does not release the
                                  // lock before t1_t3 tries to acquire it
Semaphore t1_done("t1_done",0);   // So that TestSuite knows when Test 1 is
                                  // done
Lock t1_l1("t1_l1");      // the lock tested in Test 1

// --------------------------------------------------
// t1_t1() -- test1 thread 1
//     This is the rightful lock owner
// --------------------------------------------------
void t1_t1() {
    t1_l1.Acquire();
    t1_s1.V();  // Allow t1_t2 to try to Acquire Lock
 
    printf ("%s: Acquired Lock %s, waiting for t3\n",currentThread->getName(),
      t1_l1.getName());
    t1_s3.P();
    printf ("%s: working in CS\n",currentThread->getName());
    for (int i = 0; i < 1000000; i++) ;
    printf ("%s: Releasing Lock %s\n",currentThread->getName(),
      t1_l1.getName());
    t1_l1.Release();
    t1_done.V();
}

// --------------------------------------------------
// t1_t2() -- test1 thread 2
//     This thread will wait on the held lock.
// --------------------------------------------------
void t1_t2() {

    t1_s1.P();  // Wait until t1 has the lock
    t1_s2.V();  // Let t3 try to acquire the lock

    printf("%s: trying to acquire lock %s\n",currentThread->getName(),
      t1_l1.getName());
    t1_l1.Acquire();

    printf ("%s: Acquired Lock %s, working in CS\n",currentThread->getName(),
      t1_l1.getName());
    for (int i = 0; i < 10; i++)
  ;
    printf ("%s: Releasing Lock %s\n",currentThread->getName(),
      t1_l1.getName());
    t1_l1.Release();
    t1_done.V();
}

// --------------------------------------------------
// t1_t3() -- test1 thread 3
//     This thread will try to release the lock illegally
// --------------------------------------------------
void t1_t3() {

    t1_s2.P();  // Wait until t2 is ready to try to acquire the lock

    t1_s3.V();  // Let t1 do it's stuff
    for ( int i = 0; i < 3; i++ ) {
  printf("%s: Trying to release Lock %s\n",currentThread->getName(),
         t1_l1.getName());
  t1_l1.Release();
    }
}

// --------------------------------------------------
// Test 2 - see TestSuite() for details
// --------------------------------------------------
Lock t2_l1("t2_l1");    // For mutual exclusion
Condition t2_c1("t2_c1"); // The condition variable to test
Semaphore t2_s1("t2_s1",0); // To ensure the Signal comes before the wait
Semaphore t2_done("t2_done",0);     // So that TestSuite knows when Test 2 is
                                  // done

// --------------------------------------------------
// t2_t1() -- test 2 thread 1
//     This thread will signal a variable with nothing waiting
// --------------------------------------------------
void t2_t1() {
    t2_l1.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
     t2_l1.getName(), t2_c1.getName());
    t2_c1.Signal(&t2_l1);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
     t2_l1.getName());
    t2_l1.Release();
    t2_s1.V();  // release t2_t2
    t2_done.V();
}

// --------------------------------------------------
// t2_t2() -- test 2 thread 2
//     This thread will wait on a pre-signalled variable
// --------------------------------------------------
void t2_t2() {
    t2_s1.P();  // Wait for t2_t1 to be done with the lock
    t2_l1.Acquire();
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
     t2_l1.getName(), t2_c1.getName());
    t2_c1.Wait(&t2_l1);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
     t2_l1.getName());
    t2_l1.Release();
}
// --------------------------------------------------
// Test 3 - see TestSuite() for details
// --------------------------------------------------
Lock t3_l1("t3_l1");    // For mutual exclusion
Condition t3_c1("t3_c1"); // The condition variable to test
Semaphore t3_s1("t3_s1",0); // To ensure the Signal comes before the wait
Semaphore t3_done("t3_done",0); // So that TestSuite knows when Test 3 is
                                // done

// --------------------------------------------------
// t3_waiter()
//     These threads will wait on the t3_c1 condition variable.  Only
//     one t3_waiter will be released
// --------------------------------------------------
void t3_waiter() {
    t3_l1.Acquire();
    t3_s1.V();    // Let the signaller know we're ready to wait
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
     t3_l1.getName(), t3_c1.getName());
    t3_c1.Wait(&t3_l1);
    printf("%s: freed from %s\n",currentThread->getName(), t3_c1.getName());
    t3_l1.Release();
    t3_done.V();
}


// --------------------------------------------------
// t3_signaller()
//     This threads will signal the t3_c1 condition variable.  Only
//     one t3_signaller will be released
// --------------------------------------------------
void t3_signaller() {

    // Don't signal until someone's waiting
    
    for ( int i = 0; i < 5 ; i++ ) 
  t3_s1.P();
    t3_l1.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
     t3_l1.getName(), t3_c1.getName());
    t3_c1.Signal(&t3_l1);
    printf("%s: Releasing %s\n",currentThread->getName(), t3_l1.getName());
    t3_l1.Release();
    t3_done.V();
}
 
// --------------------------------------------------
// Test 4 - see TestSuite() for details
// --------------------------------------------------
Lock t4_l1("t4_l1");    // For mutual exclusion
Condition t4_c1("t4_c1"); // The condition variable to test
Semaphore t4_s1("t4_s1",0); // To ensure the Signal comes before the wait
Semaphore t4_done("t4_done",0); // So that TestSuite knows when Test 4 is
                                // done

// --------------------------------------------------
// t4_waiter()
//     These threads will wait on the t4_c1 condition variable.  All
//     t4_waiters will be released
// --------------------------------------------------
void t4_waiter() {
    t4_l1.Acquire();
    t4_s1.V();    // Let the signaller know we're ready to wait
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
     t4_l1.getName(), t4_c1.getName());
    t4_c1.Wait(&t4_l1);
    printf("%s: freed from %s\n",currentThread->getName(), t4_c1.getName());
    t4_l1.Release();
    t4_done.V();
}


// --------------------------------------------------
// t2_signaller()
//     This thread will broadcast to the t4_c1 condition variable.
//     All t4_waiters will be released
// --------------------------------------------------
void t4_signaller() {

    // Don't broadcast until someone's waiting
    
    for ( int i = 0; i < 5 ; i++ ) 
  t4_s1.P();
    t4_l1.Acquire();
    printf("%s: Lock %s acquired, broadcasting %s\n",currentThread->getName(),
     t4_l1.getName(), t4_c1.getName());
    t4_c1.Broadcast(&t4_l1);
    printf("%s: Releasing %s\n",currentThread->getName(), t4_l1.getName());
    t4_l1.Release();
    t4_done.V();
}
// --------------------------------------------------
// Test 5 - see TestSuite() for details
// --------------------------------------------------
Lock t5_l1("t5_l1");    // For mutual exclusion
Lock t5_l2("t5_l2");    // Second lock for the bad behavior
Condition t5_c1("t5_c1"); // The condition variable to test
Semaphore t5_s1("t5_s1",0); // To make sure t5_t2 acquires the lock after
                                // t5_t1

// --------------------------------------------------
// t5_t1() -- test 5 thread 1
//     This thread will wait on a condition under t5_l1
// --------------------------------------------------
void t5_t1() {
    t5_l1.Acquire();
    t5_s1.V();  // release t5_t2
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
     t5_l1.getName(), t5_c1.getName());
    t5_c1.Wait(&t5_l1);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
     t5_l1.getName());
    t5_l1.Release();
}

// --------------------------------------------------
// t5_t1() -- test 5 thread 1
//     This thread will wait on a t5_c1 condition under t5_l2, which is
//     a Fatal error
// --------------------------------------------------
void t5_t2() {
    t5_s1.P();  // Wait for t5_t1 to get into the monitor
    t5_l1.Acquire();
    t5_l2.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
     t5_l2.getName(), t5_c1.getName());
    t5_c1.Signal(&t5_l2);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
     t5_l2.getName());
    t5_l2.Release();
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
     t5_l1.getName());
    t5_l1.Release();
}

#include "../passport_office/utilities.h"
#include "../passport_office/passport_office.h"
#include "../passport_office/clerks.h"
#include "../passport_office/customer.h"
#include "../passport_office/manager.h"
#include "../passport_office/passport_office.cc"
#include "../passport_office/clerks.cc"
#include "../passport_office/customer.cc"
#include "../passport_office/manager.cc"

void PTest1() {
  printf("Starting Test 1 - Customers always take the shortest line, but no 2 "
         "customers ever choose the same shortest line at the same time.\n");
  PassportOffice po(2, 0, 0, 0);
  po.Start();
  for (int i = 0; i < 3; ++i) {
    po.AddNewCustomer(new Customer(&po, 100));
  }
  po.WaitOnFinish();
  po.Stop();
  printf("################ Finished Test 1 ################\n");
}

void PTest2() {
  printf("Starting Test 2 - Managers only read one from one Clerk's total money received, at a time.\n");
  PassportOffice po(1, 1, 1, 1);
  po.Start();
  for (int i = 0; i < 7; ++i) {
    Customer* c = new Customer(&po, 600);
    po.AddNewCustomer(c);
  }
  po.WaitOnFinish();
  po.Stop();
  printf("################ Finished Test 2 ################\n");
}

void PTest3() {
  printf("Starting Test 3 - Customers do not leave until they are given their passport by the Cashier. The Cashier does not start on another customer until they know that the last Customer has left their area\n");
  PassportOffice po(1, 1, 1, 1);
  po.Start();
  for (int i = 0; i < 4; ++i) {
    Customer* c = new Customer(&po, 100);
    po.AddNewCustomer(c);
  }
  po.WaitOnFinish();  po.Stop();
  printf("################ Finished Test 3 ################\n");
}

void PTest4() {
  printf("Starting Test 4 - Clerks go on break when they have no one waiting in their line\n");
  PassportOffice po(1, 1, 1, 1);
  po.Start();
    po.WaitOnFinish();po.Stop();
  printf("################ Finished Test 4 ################\n");
}

void PTest5() {
  printf("Starting Test 5 - Managers get Clerks off their break when lines get too long\n");
  PassportOffice po(1, 0, 0, 0);
  po.Start();
  po.clerks_[clerk_types::kApplication][0]->wakeup_lock_.Acquire();
  po.clerks_[clerk_types::kApplication][0]->wakeup_lock_cv_.Wait(&po.clerks_[clerk_types::kApplication][0]->wakeup_lock_);
  po.clerks_[clerk_types::kApplication][0]->wakeup_lock_.Release();
  for (int i = 0; i < 4; ++i) {
    Customer* c = new Customer(&po, 100);
    po.AddNewCustomer(c);
  }
  po.WaitOnFinish();
  po.Stop();
  printf("################ Finished Test 5 ################\n");
}

void PTest6() {
  printf("Starting Test 6 - Total sales never suffers from a race condition\n");
  PassportOffice po(1, 1, 1, 1);
  po.Start();
  for (int i = 0; i < 10; ++i) {
    Customer* c = new Customer(&po, 100);
    po.AddNewCustomer(c);
  }
    po.WaitOnFinish();po.Stop();
  printf("################ Finished Test 6 ################\n");
}

void PTest7() {
  printf("Starting Test 7 - The behavior of Customers is proper when Senators arrive. This is before, during, and after.\n");
  PassportOffice po(1, 1, 1, 1);
  po.Start();
  for (int i = 0; i < 3; ++i) {
    Customer* c = new Customer(&po, 1600);
    po.AddNewCustomer(c);
  }
  Senator* s = new Senator(&po);
  po.AddNewSenator(s);
    po.WaitOnFinish();po.Stop();
  printf("################ Finished Test 7 ################\n");
}

// --------------------------------------------------
// TestSuite()
//     This is the main thread of the test suite.  It runs the
//     following tests:
//
//       1.  Show that a thread trying to release a lock it does not
//       hold does not work
//
//       2.  Show that Signals are not stored -- a Signal with no
//       thread waiting is ignored
//
//       3.  Show that Signal only wakes 1 thread
//
//   4.  Show that Broadcast wakes all waiting threads
//
//       5.  Show that Signalling a thread waiting under one lock
//       while holding another is a Fatal error
//
//     Fatal errors terminate the thread in question.
// --------------------------------------------------
void TestSuite() {
    Thread *t;
    char *name;
    int i;
    
    // Test 1

    printf("Starting Test 1\n");

    t = new Thread("t1_t1");
    t->Fork((VoidFunctionPtr)t1_t1,0);

    t = new Thread("t1_t2");
    t->Fork((VoidFunctionPtr)t1_t2,0);

    t = new Thread("t1_t3");
    t->Fork((VoidFunctionPtr)t1_t3,0);

    // Wait for Test 1 to complete
    for (  i = 0; i < 2; i++ )
  t1_done.P();

    // Test 2

    printf("Starting Test 2.  Note that it is an error if thread t2_t2\n");
    printf("completes\n");

    t = new Thread("t2_t1");
    t->Fork((VoidFunctionPtr)t2_t1,0);

    t = new Thread("t2_t2");
    t->Fork((VoidFunctionPtr)t2_t2,0);

    // Wait for Test 2 to complete
    t2_done.P();

    // Test 3

    printf("Starting Test 3\n");

    for (  i = 0 ; i < 5 ; i++ ) {
  name = new char [20];
  sprintf(name,"t3_waiter%d",i);
  t = new Thread(name);
  t->Fork((VoidFunctionPtr)t3_waiter,0);
    }
    t = new Thread("t3_signaller");
    t->Fork((VoidFunctionPtr)t3_signaller,0);

    // Wait for Test 3 to complete
    for (  i = 0; i < 2; i++ )
  t3_done.P();

    // Test 4

    printf("Starting Test 4\n");

    for (  i = 0 ; i < 5 ; i++ ) {
  name = new char [20];
  sprintf(name,"t4_waiter%d",i);
  t = new Thread(name);
  t->Fork((VoidFunctionPtr)t4_waiter,0);
    }
    t = new Thread("t4_signaller");
    t->Fork((VoidFunctionPtr)t4_signaller,0);

    // Wait for Test 4 to complete
    for (  i = 0; i < 6; i++ )
  t4_done.P();

    // Test 5

    printf("Starting Test 5.  Note that it is an error if thread t5_t1\n");
    printf("completes\n");

    t = new Thread("t5_t1");
    t->Fork((VoidFunctionPtr)t5_t1,0);

    t = new Thread("t5_t2");
    t->Fork((VoidFunctionPtr)t5_t2,0);

    for (i = 0; i < 500; ++i) {
      currentThread->Yield();
    }
    printf("##################################\n");
    PTest1();
    PTest2();
    PTest3();
    PTest4();
    PTest5();
    PTest6();
}
#endif


//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest()
{
    DEBUG('t', "Entering SimpleTest");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}

void Problem2() {
  int num_customers, num_senators, num_application_clerks, num_picture_clerks,
      num_passport_clerks, num_cashier_clerks;
  while (true) {
    std::cout << "Number of customers to simulate: ";
    std::cin >> num_customers;
    if (num_customers < 0 || num_customers > 50) {
      std::cout << "Invalid range for customers" << std::endl;
      continue;
    } else {
      break;
    }
  }

  while (true) {
    std::cout << "Number of senators to simulate: ";
    std::cin >> num_senators;
    if (num_senators < 0 || num_senators > 10) {
      std::cout << "Invalid range for senators" << std::endl;
      continue;
    } else {
      break;
    }
  }

  while (true) {
    std::cout << "Number of application clerks to simulate: ";
    std::cin >> num_application_clerks;
    if (num_application_clerks < 1 || num_application_clerks > 5) {
      std::cout << "Invalid range for application clerks" << std::endl;
      continue;
    } else {
      break;
    }
  }

  while (true) {
    std::cout << "Number of picture clerks to simulate: ";
    std::cin >> num_picture_clerks;
    if (num_picture_clerks < 1 || num_picture_clerks > 5) {
      std::cout << "Invalid range for picture clerks" << std::endl;
      continue;
    } else {
      break;
    }
  }

  while (true) {
    std::cout << "Number of passport clerks to simulate: ";
    std::cin >> num_passport_clerks;
    if (num_passport_clerks < 1 || num_passport_clerks > 5) {
      std::cout << "Invalid range for passport clerks" << std::endl;
      continue;
    } else {
      break;
    }
  }

  while (true) {
    std::cout << "Number of cashier clerks to simulate: ";
    std::cin >> num_cashier_clerks;
    if (num_cashier_clerks < 1 || num_cashier_clerks > 5) {
      std::cout << "Invalid range for cashier clerks" << std::endl;
      continue;
    } else {
      break;
    }
  }

  PassportOffice* passport_office
      = new PassportOffice(num_application_clerks, num_picture_clerks,
                           num_passport_clerks, num_cashier_clerks);
  passport_office->Start();
  for (int i = 0; i < num_customers + num_senators; ++i) {
    int next_customer_type = rand() % (num_customers + num_senators);
    if (next_customer_type >= num_customers) {
      passport_office->AddNewSenator(new Senator(passport_office));
      --num_senators;
    } else {
      passport_office->AddNewCustomer(new Customer(passport_office));
      --num_customers;
    }
  }
  currentThread->Finish();
}
