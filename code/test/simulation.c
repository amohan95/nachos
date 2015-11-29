#include "../userprog/syscall.h"
#include "utilities.h"

void* memcpy(void* dst, void* src, int size) {
  int i;
  char* cpy_dst = (char*) dst; 
  char* cpy_src = (char*) src;
  for (i = 0; i < size; ++i) {
    cpy_dst[i] = cpy_src[i];
  }
  return (void*) cpy_dst;
}

int num_clerks_[NUM_CLERK_TYPES] 
    = {NUM_APPLICATION_CLERKS, NUM_PICTURE_CLERKS, NUM_PASSPORT_CLERKS, 
        NUM_CASHIER_CLERKS};

int application_clerk_init_lock_;
int application_clerk_init_count_ = 0;

int picture_clerk_init_lock_;
int picture_clerk_init_count_ = 0;

int passport_clerk_init_lock_;
int passport_clerk_init_count_ = 0;

int cashier_clerk_init_lock_;
int cashier_clerk_init_count_ = 0;

int breaking_clerks_lock_;
int senator_lock_;
int senator_condition_;
int customer_count_lock_;
int customers_served_lock_;
int customers_served_cv_;
int num_customers_being_served_ = 0;
int num_customers_waiting_lock_;
int num_customers_waiting_ = 0;
int num_senators_lock_;
int num_senators_ = 0;
int outside_line_lock_;
int outside_line_cv_;

int line_locks_[NUM_CLERK_TYPES];
Customer customers_[NUM_CUSTOMERS + NUM_SENATORS];
int customers_size_ = NUM_CUSTOMERS;
int customers_init_size_ = 0;
int num_threads = 0;

Clerk clerks_[NUM_CLERK_TYPES][MAX_NUM_CLERKS];
int line_counts_[NUM_CLERK_TYPES][MAX_NUM_CLERKS];
int bribe_line_counts_[NUM_CLERK_TYPES][MAX_NUM_CLERKS];

Manager manager_;

int NUM_INITIAL_MONEY_AMOUNTS = 4;
int INITIAL_MONEY_AMOUNTS[] = {100, 600, 1100, 1600};
int CURRENT_UNUSED_SSN = 0;
int SENATOR_UNUSED_SSN = 0;

void AddNewCustomer(Customer customer, int index);
void AddNewSenator(Customer senator, int index);
void DestroyCustomer(Customer* customer);
void PrintCustomerIdentifierString(Customer* customer);
void DoClerkWork(Customer* customer, Clerk* clerk);
void PrintLineJoin(Customer* customer, Clerk* clerk, int bribed);
void PrintSenatorIdentifierString(Customer* customer);
void SenatorRun(Customer* customer);
void CustomerRun(Customer* customer);
void PrintNameForClerkType(ClerkType type);
void PrintClerkIdentifierString(Clerk* clerk);
void DestroyClerk(Clerk* clerk);
void JoinLine(Clerk* clerk, int bribe);
void GetNextCustomer(Clerk* clerk);
void ApplicationClerkWork(Clerk* clerk);
void PictureClerkWork(Clerk* clerk);
void PassportClerkWork(Clerk* clerk);
void CashierClerkWork(Clerk* clerk);
void ClerkRun(Clerk* clerk);
void ManagerPrintMoneyReport(Manager* manager);
void ManagerRun(Manager* manager);

void RunManager();
void RunManagerMoneyReport();
void RunClerk();
void RunCustomer();
void RunSenator();

void WakeWaitingCustomers();
void WakeClerksForSenator();

void SetupPassportOffice();
void StartPassportOffice();

/* Gets the total number of customers waiting in line for a clerk type. */
int GetNumCustomersForClerkType(ClerkType type) {
  int total = 0;
  int i;
  for (i = 0; i < num_clerks_[type]; ++i) {
    total += line_counts_[type][i];
    total += bribe_line_counts_[type][i];
  }
  return total;
}

/* Adds a new customer to the passport office by creating a new thread and
  forking it. Additionally, this will add the customer to the customers_ set.*/
void AddNewCustomer(Customer customer, int index) {
  Acquire(customer_count_lock_);
  customers_[index] = customer;
  Fork(RunCustomer);
  Release(customer_count_lock_);
}

/* Adds a new senator to the passport office by creating a new thread and
  forking it. */
void AddNewSenator(Customer senator, int index) {
  Acquire(customer_count_lock_);
  customers_[index] = senator;
  Fork(RunSenator);
  Release(customer_count_lock_);
/*  Thread* thread = new Thread("senator thread");
  thread->Fork(thread_runners::RunSenator, reinterpret_cast<int>(senator));
*/}

/* ######## Customer Functionality ######## */
Customer CreateCustomer(CustomerType type, int money) {
  Customer customer;
  customer.type_ = type;
  customer.money_ =
      (money == 0 ? INITIAL_MONEY_AMOUNTS[Rand() % NUM_INITIAL_MONEY_AMOUNTS]
                  : money);
  customer.ssn_ = (type == kCustomer ?
      CURRENT_UNUSED_SSN++ : SENATOR_UNUSED_SSN++);
  customer.bribed_ = 0;
  customer.certified_ = 0;
  customer.completed_application_ = 0;
  customer.passport_verified_ = 0;
  customer.picture_taken_ = 0;
  customer.running_ = 0;
  customer.join_line_lock_ = CreateLock("jll", 3);
  customer.join_line_lock_cv_ = CreateCondition("jllcv", 5);
  return customer;
}

void DestroyCustomer(Customer* customer) {
  DestroyLock(customer->join_line_lock_);
  DestroyCondition(customer->join_line_lock_cv_);
}

void PrintCustomerIdentifierString(Customer* customer) {
  if (customer->type_ == kCustomer) {
    Print("Customer [", 10);
  } else {
    Print("Senator [", 9);
  }
  PrintNum(customer->ssn_);
  Print("]", 1);
}

void DoClerkWork(Customer* customer, Clerk* clerk) {
  Acquire(clerk->wakeup_lock_);
  clerk->customer_ssn_ = customer->ssn_;
  clerk->current_customer_ = customer;
  PrintCustomerIdentifierString(customer);
  Print(" has given SSN [", 16);
  PrintNum(customer->ssn_);
  Print("] to ", 5);
  PrintClerkIdentifierString(clerk);
  Print(".\n", 2);
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  
  switch (clerk->type_) {
    case kApplication:
      Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      break;
    case kPicture:
      clerk->customer_input_ = (Rand() % 10) > 0;
      Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      break;
    case kCashier:
      clerk->customer_money_ = PASSPORT_FEE;
      Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      clerk->customer_input_ = 1;
      Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      break;
    case kPassport:
      clerk->customer_input_ = 1;
      Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      break;
    case NUM_CLERK_TYPES:
      break;
  }
}

void PrintLineJoin(Customer* customer, Clerk* clerk, int bribed) {
  PrintCustomerIdentifierString(customer);
  Print(" has gotten in ", 15);
  Print((bribed ? "bribe" : "regular"), (bribed ? 5 : 7));
  Print(" line for ", 10); 
  PrintClerkIdentifierString(clerk);
  Print(".\n", 2);
}

void PrintSenatorIdentifierString(Customer* customer) {
  Print("Senator [", 9);
  PrintNum(customer->ssn_);
  Print("]", 1);
}

void SenatorRun(Customer* senator) {
  int i, j;
  ClerkType next_clerk;
  Clerk* clerk;
  
  Release(customer_count_lock_);

  senator->running_ = 1;
  /* Increment the number of senators in the office so that others know 
     that a senator is there. */
  Acquire(num_senators_lock_);
  ++num_senators_;
  Release(num_senators_lock_);

  Acquire(senator_lock_);

  /* Wake up customers that are currently in line in the passport office so that
    they can join the outside line. */
  while (num_customers_being_served_ > 0) {
    WakeWaitingCustomers();
    Yield();
  }

  /* Reset the line counts to 0 since there are no customers in the office
    at this point. */
  for (i = 0; i < NUM_CLERK_TYPES; ++i) {
    Acquire(line_locks_[i]);
    for (j = 0; j < num_clerks_[i]; ++j) {
      line_counts_[i][j] = 0;
      bribe_line_counts_[i][j] = 0;      
    }
    Release(line_locks_[i]);
  }
  
  /* Wait for all clerks to get off of their breaks, if necessary. */
  for (i = 0; i < 500; ++i) { Yield(); }

  while (senator->running_ &&
        (!senator->passport_verified_ || !senator->picture_taken_ ||
         !senator->completed_application_ || !senator->certified_)) {
    if (!senator->completed_application_ && !senator->picture_taken_ && 
        NUM_APPLICATION_CLERKS > 0 &&
        NUM_PICTURE_CLERKS > 0) {
      next_clerk = (ClerkType)(Rand() % 2); /* either kApplication (0) or kPicture (1)*/
    } else if (!senator->completed_application_) {
      next_clerk = kApplication;
    } else if (!senator->picture_taken_) {
      next_clerk = kPicture;
    } else if (!senator->certified_) {
      next_clerk = kPassport;
    } else {
      next_clerk = kCashier;
    }
    if (num_clerks_[next_clerk] < 1) {
      break;
    }
    clerk = &(clerks_[next_clerk][0]);
    Acquire(line_locks_[next_clerk]);
    ++line_counts_[next_clerk][0];
    Release(line_locks_[next_clerk]);
    
    Signal(manager_.wakeup_condition_, manager_.wakeup_condition_lock_);

    PrintLineJoin(senator, clerk, 0);
    JoinLine(clerk, 0);

    DoClerkWork(senator, clerk);

    Acquire(line_locks_[next_clerk]);
    --line_counts_[next_clerk][0];
    Release(line_locks_[next_clerk]);

    clerk->current_customer_ = NULL;
    Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
    Release(clerk->wakeup_lock_);
  }

  if (senator->passport_verified_) {
    PrintSenatorIdentifierString(senator);
    Print(" is leaving the Passport Office.\n", 33);
  } else {
    PrintSenatorIdentifierString(senator);
    Print(" terminated early because it is impossible to get a passport.\n", 62);
  }

  /* Leaving the office - if there are no more senators left waiting, then
    tell all the customers outside to come back in.*/
  --num_senators_;
  if (num_senators_ == 0) {
    Broadcast(outside_line_cv_, outside_line_lock_);
  }
  Release(senator_lock_);
}

void CustomerRun(Customer* customer) {
  int shortest;
  int bribe_shortest;
  ClerkType next_clerk;
  int i;
  Clerk* clerk;

  Release(customer_count_lock_);

  customer->running_ = 1;
  while (customer->running_ &&
      (!customer->passport_verified_ || !customer->picture_taken_ ||
      !customer->completed_application_ || !customer->certified_)) {
    Acquire(num_senators_lock_);
    if (num_senators_ > 0) {
      Release(num_senators_lock_);
      Acquire(outside_line_lock_);
      PrintCustomerIdentifierString(customer);
      Print(" is going outside the Passport Office because there is a Senator present.\n", 74);
      Wait(outside_line_cv_, outside_line_lock_);
      Release(outside_line_lock_);
      continue;
    } else {
      Release(num_senators_lock_);
    }

    Acquire(customers_served_lock_);
    ++num_customers_being_served_;
    Release(customers_served_lock_);

    customer->bribed_ = 0;
    if (!customer->completed_application_ && !customer->picture_taken_ && NUM_APPLICATION_CLERKS > 0 && NUM_PICTURE_CLERKS > 0) {
      next_clerk = (ClerkType)(Rand() % 2); /*either kApplication (0) or kPicture (1) */
    } else if (!customer->completed_application_) {
      next_clerk = kApplication;
    } else if (!customer->picture_taken_) {
      next_clerk = kPicture;
    } else if (!customer->certified_) {
      next_clerk = kPassport;
    } else {
      next_clerk = kCashier;
    }
    clerk = NULL;
    Acquire(line_locks_[next_clerk]);
    shortest = -1;
    for (i = 0; i < num_clerks_[next_clerk]; ++i) {
      if (shortest == -1 ||
          line_counts_[next_clerk][i] < line_counts_[next_clerk][shortest]) {
        shortest = i;
      }
    }
    if (customer->money_ >= CLERK_BRIBE_AMOUNT + PASSPORT_FEE) {
      bribe_shortest = -1;
      for (i = 0; i < num_clerks_[next_clerk]; ++i) {
        if (bribe_shortest == -1 ||
            bribe_line_counts_[next_clerk][i]
            < bribe_line_counts_[next_clerk][bribe_shortest]) {
          bribe_shortest = i;
        }
      }
      if (shortest == -1) {
        customer->running_ = 0;
        Release(line_locks_[next_clerk]);
        continue;
      }
      if (bribe_line_counts_[next_clerk][bribe_shortest]
          < line_counts_[next_clerk][shortest]) {
        clerk = &(clerks_[next_clerk][bribe_shortest]);
        customer->bribed_ = 1;
      } else {
        clerk = &(clerks_[next_clerk][shortest]);
      }
    } else {
      if (shortest == -1) {
        Release(line_locks_[next_clerk]);
        customer->running_ = 0;
        continue;
      }
      clerk = &(clerks_[next_clerk][shortest]);
    }
    Release(line_locks_[next_clerk]);

    if (GetNumCustomersForClerkType(next_clerk) > 
        CLERK_WAKEUP_THRESHOLD) {
      Signal(manager_.wakeup_condition_, manager_.wakeup_condition_lock_);
    }

    PrintLineJoin(customer, clerk, customer->bribed_);

    Acquire(num_customers_waiting_lock_);
    ++num_customers_waiting_;
    Release(num_customers_waiting_lock_);

    JoinLine(clerk, customer->bribed_);

    Acquire(num_customers_waiting_lock_);
    --num_customers_waiting_;
    Release(num_customers_waiting_lock_);

    Acquire(customers_served_lock_);
    --num_customers_being_served_;
    Release(customers_served_lock_);
    Acquire(num_senators_lock_);
    
    if (num_senators_ > 0) {
      Release(num_senators_lock_);
      Acquire(customers_served_lock_);
      if (num_customers_being_served_ == 0) {
        Broadcast(customers_served_cv_, customers_served_lock_);
      }
      Release(customers_served_lock_);
      continue;
    }
    Release(num_senators_lock_);

    if (!customer->running_) {
      break;
    }
    
    DoClerkWork(customer, clerk);

    if (customer->bribed_) {
      clerk->customer_money_ = CLERK_BRIBE_AMOUNT;
      customer->money_ -= CLERK_BRIBE_AMOUNT;
      Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
    }
    clerk->current_customer_ = NULL;
    Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
    Release(clerk->wakeup_lock_);
  }
  if (customer->passport_verified_) {
    PrintCustomerIdentifierString(customer);
    Print(" is leaving the Passport Office.\n", 33);
  } else {
    PrintCustomerIdentifierString(customer);
    Print(" terminated early because it is impossible to get a passport.\n", 62);
  }
  Acquire(customers_served_lock_);
  --num_customers_being_served_;
  if (num_customers_being_served_ == 0) {
    Broadcast(customers_served_cv_, customers_served_lock_);
  }
  Release(customers_served_lock_);
  
  Acquire(customer_count_lock_);
  --customers_size_;
  Release(customer_count_lock_);
}

/* ######## Clerk Functionality ######## */
void PrintNameForClerkType(ClerkType type) {
  switch (type) {
    case kApplication :     
      Print("ApplicationClerk", 16);
      break;
    case kPicture :
      Print("PictureClerk", 12);
      break;
    case kPassport :
      Print("PassportClerk", 13);
      break;
    case kCashier :
      Print("Cashier", 7);
      break;
  }
}

void PrintClerkIdentifierString(Clerk* clerk) {
  PrintNameForClerkType(clerk->type_);
  Print(" [", 2);
  PrintNum(clerk->identifier_);
  Print("]", 1);
}

Clerk CreateClerk(int identifier, ClerkType type) {
  Clerk clerk;
  clerk.customer_money_ = 0;
  clerk.customer_input_ = 0;
  clerk.state_ = kAvailable;
  clerk.collected_money_ = 0;
  clerk.identifier_ = identifier;
  clerk.running_ = 0;
  clerk.lines_lock_ = CreateLock("Clerk Lines Lock", 16);
  clerk.bribe_line_lock_cv_ = CreateCondition("Clerk Bribe Line Condition", 26); 
  clerk.bribe_line_lock_ = CreateLock("Clerk Bribe Line Lock", 21);
  clerk.regular_line_lock_cv_ = CreateCondition("Clerk Regular Line Condition", 28); 
  clerk.regular_line_lock_ = CreateLock("Clerk Regular Line Lock", 23);
  clerk.wakeup_lock_cv_ = CreateCondition("Clerk Wakeup Condition", 22);
  clerk.wakeup_lock_ = CreateLock("Clerk Wakeup Lock", 17);
  clerk.money_lock_ = CreateLock("Clerk Money Lock", 16);
  clerk.type_ = type;

  return clerk;
}

void DestroyClerk(Clerk* clerk) {
  DestroyLock(clerk->lines_lock_);
  DestroyCondition(clerk->bribe_line_lock_cv_); 
  DestroyLock(clerk->bribe_line_lock_);
  DestroyCondition(clerk->regular_line_lock_cv_); 
  DestroyLock(clerk->regular_line_lock_);
  DestroyCondition(clerk->wakeup_lock_cv_);
  DestroyLock(clerk->wakeup_lock_);
  DestroyLock(clerk->money_lock_);
}

int CollectMoney(Clerk* clerk) {
  int money;
  Acquire(clerk->money_lock_);
  money = clerk->collected_money_;
  clerk->collected_money_ = 0;
  Release(clerk->money_lock_);
  return money;
}

void JoinLine(Clerk* clerk, int bribe) {
  if (bribe) {
    Acquire(clerk->bribe_line_lock_);
    ++bribe_line_counts_[clerk->type_][clerk->identifier_];
    if (GetNumCustomersForClerkType(clerk->type_) > 
        CLERK_WAKEUP_THRESHOLD) {
      Signal(manager_.wakeup_condition_, manager_.wakeup_condition_lock_);
    }
    Wait(clerk->bribe_line_lock_cv_, clerk->bribe_line_lock_);
    Release(clerk->bribe_line_lock_);
  } else {
    Acquire(clerk->regular_line_lock_);
    ++line_counts_[clerk->type_][clerk->identifier_];
    if (GetNumCustomersForClerkType(clerk->type_) > 
        CLERK_WAKEUP_THRESHOLD) {
      Signal(manager_.wakeup_condition_, manager_.wakeup_condition_lock_);
    }
    Wait(clerk->regular_line_lock_cv_ , clerk->regular_line_lock_);
    Release(clerk->regular_line_lock_);
  }
}

int GetNumCustomersInLine(Clerk* clerk) {
  return line_counts_[clerk->type_][clerk->identifier_] +
      bribe_line_counts_[clerk->type_][clerk->identifier_];
}

void GetNextCustomer(Clerk* clerk) {
  int bribe_line_count;
  int regular_line_count;

  Acquire(line_locks_[clerk->type_]);
  Acquire(clerk->bribe_line_lock_);
  bribe_line_count = bribe_line_counts_[clerk->type_][clerk->identifier_];
  Release(clerk->bribe_line_lock_);

  Acquire(clerk->regular_line_lock_);
  regular_line_count = line_counts_[clerk->type_][clerk->identifier_];
  Release(clerk->regular_line_lock_);

	/*PrintNum(clerk->identifier_);
	Print(" ", 1);
	PrintNum(bribe_line_count);
	Print(" ", 1);
	PrintNum(regular_line_count);
	Print("\n", 1);*/
  if (bribe_line_count > 0) {
    PrintClerkIdentifierString(clerk);
    Print(" has signalled a Customer to come to their counter.\n", 52);

    Acquire(clerk->bribe_line_lock_);
    Acquire(clerk->wakeup_lock_);
    Signal(clerk->bribe_line_lock_cv_, clerk->bribe_line_lock_);
    Release(clerk->bribe_line_lock_);
    clerk->state_ = kBusy;
    bribe_line_counts_[clerk->type_][clerk->identifier_]--;
  } else if (regular_line_count > 0) {
    PrintClerkIdentifierString(clerk);
    Print(" has signalled a Customer to come to their counter.\n", 52);

    Acquire(clerk->regular_line_lock_);
    Acquire(clerk->wakeup_lock_);
    Signal(clerk->regular_line_lock_cv_, clerk->regular_line_lock_);
    Release(clerk->regular_line_lock_);
    clerk->state_ = kBusy;
    line_counts_[clerk->type_][clerk->identifier_]--;
  } else {
    clerk->state_ = kOnBreak;
  }
  Release(line_locks_[clerk->type_]);
}

void ApplicationClerkWork(Clerk* clerk) {
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  /* Wait for customer to put passport on counter. */
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  clerk->current_customer_->completed_application_ = 1;
  PrintClerkIdentifierString(clerk);
  Print(" has recorded a completed application for ", 42);
  PrintCustomerIdentifierString(clerk->current_customer_);
  Print("\n", 1);
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
}

void PictureClerkWork(Clerk* clerk) {
  int picture_accepted;
  /* Take Customer's picture and wait to hear if they like it. */
  Print(" [", 2);
  PrintNum(clerk->identifier_);
  Print(" has taken a picture of ", 24);
  PrintCustomerIdentifierString(clerk->current_customer_);
  Print("\n", 1);

  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  picture_accepted = clerk->customer_input_;

  /* If they don't like their picture don't set their picture to taken.  They go back in line. */
  if (!picture_accepted) {
    PrintClerkIdentifierString(clerk);
    Print(" has been told that ", 20);
    PrintCustomerIdentifierString(clerk->current_customer_);
    Print(" does not like their picture.\n", 30);
  } else {
    /* Set picture taken. */
    clerk->current_customer_->picture_taken_ = 1;
    PrintClerkIdentifierString(clerk); 
    Print(" has been told that ", 20);
    PrintCustomerIdentifierString(clerk->current_customer_);
    Print(" does like their picture.\n", 26);
  }
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
}

void PassportClerkWork(Clerk* clerk) {
  int picture_taken_and_passport_verified;
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  /* Wait for customer to show whether or not they got their picture taken and passport verified. */
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  picture_taken_and_passport_verified = clerk->customer_input_;

  /* Check to make sure their picture has been taken and passport verified. */
  if (!picture_taken_and_passport_verified) {
    PrintClerkIdentifierString(clerk);
    Print(" has determined that ", 21);
    PrintCustomerIdentifierString(clerk->current_customer_);
    Print(" does not have both their application and picture completed\n", 60);
  } else {
    PrintClerkIdentifierString(clerk); 
    Print(" has determined that ", 21);
    PrintCustomerIdentifierString(clerk->current_customer_);
    Print(" does have both their application and picture completed\n", 56);
    clerk->current_customer_->certified_ = 1;
    PrintClerkIdentifierString(clerk); 
    Print(" has recorded ", 15);
    PrintCustomerIdentifierString(clerk->current_customer_);
    Print(" passport documentation\n", 24);
  }
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
}

void CashierClerkWork(Clerk* clerk) {
  int certified;
  /* Collect application fee. */
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  Acquire(clerk->money_lock_);
  clerk->collected_money_ += clerk->customer_money_;
  clerk->customer_money_ = 0;
  Release(clerk->money_lock_);

  /* Wait for the customer to show you that they are certified. */
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  certified = clerk->customer_input_;

  /* Check to make sure they have been certified. */
  if (!certified) {
    PrintClerkIdentifierString(clerk);
    Print(" has received the $100 from ", 28);
    PrintCustomerIdentifierString(clerk->current_customer_); 
    Print(" before certification. They are to go to the back of my line.\n", 62);

    /* Give money back. */
    Acquire(clerk->money_lock_);
    clerk->collected_money_ -= 100;
    Release(clerk->money_lock_);
    clerk->current_customer_->money_ += 100;
  } else {
    PrintClerkIdentifierString(clerk);
    Print(" has received the $100 from ", 28);
    PrintCustomerIdentifierString(clerk->current_customer_); 
    Print(" after certification.\n", 23);

    /* Give customer passport. */
    PrintClerkIdentifierString(clerk);
    Print(" has provided ", 14);
    PrintCustomerIdentifierString(clerk->current_customer_); 
    Print(" their completed passport.\n", 28);

    /* Record passport was given to customer. */
    PrintClerkIdentifierString(clerk);
    Print(" has recorded that ", 19);
    PrintCustomerIdentifierString(clerk->current_customer_); 
    Print(" has been given their completed passport.\n", 43);

    clerk->current_customer_->passport_verified_ = 1;
  }
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
}

void ClerkRun(Clerk* clerk) {
  int i;
  int bribe;

  switch(clerk->type_) {
    case kApplication :     
      Release(application_clerk_init_lock_);
      break;
    case kPicture :
      Release(picture_clerk_init_lock_);
      break;
    case kPassport :
      Release(passport_clerk_init_lock_);
      break;
    case kCashier :
      Release(cashier_clerk_init_lock_);
      break;
  }

  clerk->running_ = 1;
  while (clerk->running_) {
    GetNextCustomer(clerk);
    if (clerk->state_ == kBusy || clerk->state_ == kAvailable) {
      /* Wait for customer to come to counter and give SSN. */
      Wait(clerk->wakeup_lock_cv_ , clerk->wakeup_lock_);
      /* Take Customer's SSN and verify passport. */
      PrintClerkIdentifierString(clerk);
      Print(" has received SSN ", 18);
      PrintNum(clerk->customer_ssn_);
      Print(" from ", 6);
      PrintCustomerIdentifierString(clerk->current_customer_);
      Print("\n", 1);
      /* Do work specific to the type of clerk. */
      switch(clerk->type_) {
        case kApplication :     
          ApplicationClerkWork(clerk);
          break;
        case kPicture :
          PictureClerkWork(clerk);
          break;
        case kPassport :
          PassportClerkWork(clerk);
          break;
        case kCashier :
          CashierClerkWork(clerk);
          break;
      }

      /* Collect bribe money. */
      if (clerk->current_customer_->bribed_) {
        Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
        bribe = clerk->customer_money_;
        clerk->customer_money_ = 0;
        Acquire(clerk->money_lock_);
        clerk->collected_money_ += bribe;
        Release(clerk->money_lock_);
        PrintClerkIdentifierString(clerk);
        Print(" has received $", 15);
        PrintNum(bribe);
        Print(" from ", 6);
        PrintCustomerIdentifierString(clerk->current_customer_);
        Print("\n", 1);
        Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      }
      Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      /* Random delay. */
      for (i = 0; i < Rand() % 80 + 20; ++i) {
        Yield();
      }
      /* Wakeup customer. */
    } else if (clerk->state_ == kOnBreak) {
      Acquire(clerk->wakeup_lock_);
      /* Wait until woken up. */
      PrintClerkIdentifierString(clerk);
      Print(" is going on break\n", 19);
      Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      clerk->state_ = kAvailable;
      if (!clerk->running_) {
        break;
      }
      PrintClerkIdentifierString(clerk);
      Print(" is coming off break\n", 21);
      for (i = 0; i < 25; ++i) { Yield(); }
    }
    Release(clerk->wakeup_lock_);
  }
}

/* ######## Manager Functionality ######## */
Manager CreateManager() {
  Manager manager;
  manager.wakeup_condition_ = CreateCondition("Manager Wakeup Lock Condition", 29);
  manager.wakeup_condition_lock_ = CreateLock("Manager Wakeup Lock", 19);
manager.running_ = 1;
  manager.elapsed_ = 0;
  return manager;
}

void ManagerPrintMoneyReport(Manager* manager) {
  int i, j, m, total;
  while (manager->running_) {
    for (i = 0; i < NUM_CLERK_TYPES; ++i) {
      for (j = 0; j < num_clerks_[i]; ++j) {
        m = CollectMoney(&(clerks_[i][j]));
        m->money_[clerks_[i][j].type_] += m;
      }
    }
    total = 0;
    for (i = 0; i < NUM_CLERK_TYPES; ++i) {
      total += m->money_[i];
      Print("Manager has counted a total of $", 32);
      PrintNum(m->money_[i]);
      Print(" for ", 5);
      PrintNameForClerkType((ClerkType)(i));
      Print("s\n", 2);
    }
    Print("Manager has counted a total of $", 32);
    PrintNum(total);
    Print(" for the passport office\n", 26);
    for(i = 0; i < 200; ++i) {
      if (!manager->running_) return;
      Yield();
    }
  }
}

void ManagerRun(Manager* manager) {
  int i, j;
  Clerk* clerk;

  manager->running_ = 1;
  while(manager->running_) {
    Acquire(manager->wakeup_condition_lock_);
    Wait(manager->wakeup_condition_, manager->wakeup_condition_lock_);
    if (!manager->running_) {
      break;
    }

    for (i = 0; i < NUM_CLERK_TYPES; ++i) {
      Acquire(line_locks_[i]);
    }
    for (i = 0; i < NUM_CLERK_TYPES; ++i) {
      if (GetNumCustomersForClerkType((ClerkType)(i))
          > CLERK_WAKEUP_THRESHOLD || num_senators_ > 0) {
				/*Print("Customers for clerk type ", 25);
				PrintNum(i);
				Print("\n", 1);*/
        for (j = 0; j < num_clerks_[i]; ++j) {
          clerk = &(clerks_[i][j]);
          if (clerk->state_ == kOnBreak) {
            Acquire(clerk->wakeup_lock_);
            Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
            clerk->state_ = kAvailable;
            Release(clerk->wakeup_lock_);
            Print("Manager has woken up a", 22);
            if (clerk->type_ == kApplication) {
              Print("n ", 2);
            }
            Print(" ", 1);
            PrintNameForClerkType(clerk->type_);
            Print("\n", 1);
          }
        }
      }
    }
    for (i = 0; i < NUM_CLERK_TYPES; ++i) {
      Release(line_locks_[i]);
    }
    Release(manager_.wakeup_condition_lock_);
  }
}

void WakeWaitingCustomers() {
  int i, j;
  Clerk* clerk;

  for (i = 0; i < NUM_CLERK_TYPES; ++i) {
    for (j = 0; j < num_clerks_[i]; ++j) {
      clerk = &(clerks_[i][j]);
      Broadcast(clerk->bribe_line_lock_cv_, clerk->bribe_line_lock_);
      Broadcast(clerk->regular_line_lock_cv_, clerk->regular_line_lock_);
    }
  }
}

void WakeClerksForSenator() {
  int i;
  Clerk* clerk;

  for (i = 0; i < NUM_CLERK_TYPES; ++i) {
    if (num_clerks_[i] > 0) {
      Acquire(line_locks_[i]);
      clerk = &(clerks_[i][0]);
      if (clerk->state_ == kOnBreak) {
        clerk->state_ = kAvailable;
        Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      }
      Release(line_locks_[i]);
    }
  }
}

/* ######## Thread Runners ######## */
void RunManager() {
  ManagerRun(&manager_);
  Exit(0);
}

void RunManagerMoneyReport() {
  ManagerPrintMoneyReport(&manager_);
  Exit(0);
}

void RunApplicationClerk() {
  Acquire(application_clerk_init_lock_);
  ClerkRun(&(clerks_[kApplication][application_clerk_init_count_++]));
  Exit(0);
}

void RunPictureClerk() {
  Acquire(picture_clerk_init_lock_);
  ClerkRun(&(clerks_[kPicture][picture_clerk_init_count_++]));
  Exit(0);
}

void RunPassportClerk() {
  Acquire(passport_clerk_init_lock_);
  ClerkRun(&(clerks_[kPassport][passport_clerk_init_count_++]));
  Exit(0);
}

void RunCashierClerk() {
  Acquire(cashier_clerk_init_lock_);
  ClerkRun(&(clerks_[kCashier][cashier_clerk_init_count_++]));
  Exit(0);
}

void RunCustomer() {
  Acquire(customer_count_lock_);
  CustomerRun(&(customers_[customers_init_size_++]));
  Exit(0);
}

void RunSenator() {
  Acquire(customer_count_lock_);
  SenatorRun(&(customers_[customers_init_size_++]));
  Exit(0);
}

void SetupPassportOffice() {
  int i;

	application_clerk_init_lock_ = CreateLock("clerk initialization lock", 25);
  picture_clerk_init_lock_ = CreateLock("clerk initialization lock", 25);
  passport_clerk_init_lock_ = CreateLock("clerk initialization lock", 25);
  cashier_clerk_init_lock_ = CreateLock("clerk initialization lock", 25);

  breaking_clerks_lock_ = CreateLock("breaking clerks lock", 20);
  senator_lock_ = CreateLock("senator lock", 10);
  senator_condition_ = CreateCondition("senator condition", 17);
  customer_count_lock_ = CreateLock("customer count lock", 19);
  customers_served_lock_ = CreateLock("num customers being served lock", 31);
  customers_served_cv_ = CreateCondition("num customers being served cv", 29);
  num_customers_waiting_lock_ = CreateLock("customer waiting counter", 24);
  num_senators_lock_ = CreateLock("num senators lock", 17);
  outside_line_lock_ = CreateLock("outside line lock", 17);
  outside_line_cv_ = CreateCondition("outside line condition", 22);

  for (i = 0; i < NUM_CLERK_TYPES; ++i) {
    line_locks_[i] = CreateLock("Line Lock", 9);
  }

  /*PrintNum(NUM_APPLICATION_CLERKS);
  Print("\n", 1);
  PrintNum(NUM_PICTURE_CLERKS);
  Print("\n", 1);
  PrintNum(NUM_PASSPORT_CLERKS);
  Print("\n", 1);
  PrintNum(NUM_CASHIER_CLERKS);
  Print("\n", 1);*/

  for (i = 0; i < NUM_APPLICATION_CLERKS; ++i) {
    clerks_[kApplication][i] = CreateClerk(i, kApplication);
    line_counts_[kApplication][i] = 0;
    bribe_line_counts_[kApplication][i] = 0;
  }

	for (i = 0; i < NUM_PICTURE_CLERKS; ++i) {
    clerks_[kPicture][i] = CreateClerk(i, kPicture);
    line_counts_[kPicture][i] = 0;
    bribe_line_counts_[kPicture][i] = 0;
  }

  for (i = 0; i < NUM_PASSPORT_CLERKS; ++i) {
    clerks_[kPassport][i] = CreateClerk(i, kPassport);
    line_counts_[kPassport][i] = 0;
    bribe_line_counts_[kPassport][i] = 0;
  }

  for (i = 0; i < NUM_CASHIER_CLERKS; ++i) {
    clerks_[kCashier][i] = CreateClerk(i, kCashier);
    line_counts_[kCashier][i] = 0;
    bribe_line_counts_[kCashier][i] = 0;
  }
  manager_ = CreateManager();
}

void StartPassportOffice() {
  int i, j;

  /*Print("I love Jesus?\n", 14);*/

  for (i = 0; i < num_clerks_[kApplication]; ++i) {
    Fork(RunApplicationClerk);
  }
  for (i = 0; i < num_clerks_[kPicture]; ++i) {
    Fork(RunPictureClerk);
  }
  for (i = 0; i < num_clerks_[kPassport]; ++i) {
    Fork(RunPassportClerk);
  }
  for (i = 0; i < num_clerks_[kCashier]; ++i) {
    Fork(RunCashierClerk);
  }

  for (i = 0; i < 500; ++i) { Yield(); }

  Fork(RunManager);
  Fork(RunManagerMoneyReport);
}

void WaitOnFinish() {
  int i, j, done;
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
}
