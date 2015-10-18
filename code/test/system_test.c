#include "../userprog/syscall.h"

#define NUM_CUSTOMERS 15
#define NUM_SENATORS 2
#define NUM_APPLICATION_CLERKS 3
#define NUM_PICTURE_CLERKS 3
#define NUM_PASSPORT_CLERKS 3
#define NUM_CASHIER_CLERKS 3
#define NUM_TOTAL_CLERKS NUM_APPLICATION_CLERKS + NUM_PICTURE_CLERKS + \
    NUM_PASSPORT_CLERKS + NUM_CASHIER_CLERKS;

#include "simulation.c"

int main() {
  int i, j, index = 0;
  int done;
  int num_customers = NUM_CUSTOMERS;
  int num_senators = NUM_SENATORS;

  SetupPassportOffice();
  StartPassportOffice();

  while (NUM_CUSTOMERS + NUM_SENATORS > 0) {
    if (Rand() % (NUM_CUSTOMERS + NUM_SENATORS) >= NUM_CUSTOMERS) {
      AddNewSenator(CreateCustomer(kSenator, 0), index);
      --num_senators;
    } else {
      AddNewCustomer(CreateCustomer(kCustomer, 0), index);
      --num_customers;
    }
    ++index;
  }

  /* WaitOnFinish for the Passport Office */
  while (customers_size_ > 0) {
    for (i = 0; i < 400; ++i) { Yield(); }
    if (num_senators_ > 0) continue;
    Acquire(num_customers_waiting_lock_);
    if (customers_size_ == num_customers_waiting_) {
      Release(num_customers_waiting_lock_);
      done = 1;
      Acquire(breaking_clerks_lock_);
      for (i = 0; i < NUM_CLERK_TYPES; ++i) {
        Acquire(line_locks_[i]);
        for (j = 0; j < num_clerks_[i]; ++j) {
          if (clerks_[i][j].state_ != kOnBreak ||
              GetNumCustomersForClerkType((ClerkType)(i)) > 
              CLERK_WAKEUP_THRESHOLD) {
            done = 0;
            break;
          }
        }
        Release(line_locks_[i]);
        if (done) break;
      }
      Release(breaking_clerks_lock_);
      if (done) break;
    } else {
      Release(num_customers_waiting_lock_);
    }
  }
  for (i = 0; i < 1000; ++i) { Yield(); }

  /* Stop the Passport Office */
  Print("Attempting to stop passport office\n", 35);
  while (customers_size_ > 0) {
    done = 1;
    Acquire(breaking_clerks_lock_);
    for (i = 0; i < NUM_CLERK_TYPES; ++i) {
      for (j = 0; j < num_clerks_[i]; ++j) {
        if (clerks_[i][j].state_ != kOnBreak) {
          done = 0;
        }
      }
    }
    Release(breaking_clerks_lock_);
    if (done) {
      break;
    } else {
      for (i = 0; i < 100; ++i) { Yield(); }
    }
  }
  for (i = 0; i < customers_size_; ++i) {
    customers_[i].running_ = 0;
  }
  for (i = 0; i < NUM_CLERK_TYPES; ++i) {
    Acquire(line_locks_[i]);
    for (j = 0; j < num_clerks_[i]; ++j) {
      Acquire(clerks_[i][j].regular_line_lock_);
      Broadcast(clerks_[i][j].regular_line_lock_cv_, clerks_[i][j].regular_line_lock_);
      Release(clerks_[i][j].regular_line_lock_);
      Acquire(clerks_[i][j].bribe_line_lock_);
      Broadcast(clerks_[i][j].bribe_line_lock_cv_, clerks_[i][j].bribe_line_lock_);
      Release(clerks_[i][j].bribe_line_lock_);
      clerks_[i][j].running_ = 0;
      Acquire(clerks_[i][j].wakeup_lock_);
      Broadcast(clerks_[i][j].wakeup_lock_cv_, clerks_[i][j].wakeup_lock_);
      Release(clerks_[i][j].wakeup_lock_);
    }
    Release(line_locks_[i]);
  }
  manager_.running_ = 0;
  Print("Set manager running false\n", 26);
  Acquire(manager_.wakeup_condition_lock_);
  Signal(manager_.wakeup_condition_, manager_.wakeup_condition_lock_);
  Release(manager_.wakeup_condition_lock_);
  for (i = 0; i < 1000; ++i) { Yield(); }
  Print("Passport office stopped\n", 24);

  /* Delete Locks and CVs */
  DestroyLock(breaking_clerks_lock_);
  DestroyLock(senator_lock_);
  DestroyCondition(senator_condition_);
  DestroyLock(customer_count_lock_);
  DestroyLock(customers_served_lock_);
  DestroyCondition(customers_served_cv_);
  DestroyLock(num_customers_waiting_lock_);
  DestroyLock(num_senators_lock_);
  DestroyLock(outside_line_lock_);
  DestroyCondition(outside_line_cv_);
  for (i = 0; i < NUM_CLERK_TYPES; ++i) {
    DestroyLock(line_locks_[i]);
  }
  
  Exit(0);
  return 0;
}
