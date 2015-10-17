#include "../userprog/syscall.h"
#include "utitlities.h"

int num_customers = 15;
int num_senators = 1;
int num_application_clerks = 3;
int num_picture_clerks = 3;
int num_passport_clerks = 3;
int num_cashier_clerks = 3;
int total_clerks = num_application_clerks + num_picture_clerks 
    + num_passport_clerks + num_cashier_clerks;
int num_clerks_[clerk_types::Size] 
    = {num_application_clerks, num_picture_clerks, num_passport_clerks, 
        num_cashier_clerks};

int breaking_clerks_lock_ = CreateLock("breaking clerks lock"));
int senator_lock_ = CreateLock("senator lock"));
int senator_condition_ = CreateCondition("senator condition"));
int customer_count_lock_ = CreateLock("customer count lock"));
int customers_served_lock_ = CreateLock("num customers being served lock"));
int customers_served_cv_ = CreateCondition("num customers being served cv"));
int num_customers_being_served_ = 0;
int num_customers_waiting_lock_ = CreateLock("customer waiting counter"));
int num_customers_waiting_ = 0;
int num_senators_lock_ = CreateLock("num senators lock"));
int num_senators_ = 0;
int outside_line_lock_ = CreateLock("outside line lock"));
int outside_line_cv_ = CreateCondition("outside line condition"));

Clerk breaking_clerks[total_clerks];
int line_locks_[clerk_types::Size];
Customer* customers[num_customers+num_senators];
int customers_size = 0;
Thread* thread_list[num_customers + num_senators + total_clerks + 1];
int num_threads = 0;

Clerk* clerks_[clerk_types::Size];
int* line_counts_[clerk_types::Size];
int* bribe_line_counts_[clerk_types::Size];

static const int INITIAL_MONEY_AMOUNTS[] = {100, 600, 1100, 1600};
static int CURRENT_UNUSED_SSN = 0;
static int SENATOR_UNUSED_SSN = 0;

/* Gets the total number of customers waiting in line for a clerk type. */
int GetNumCustomersForClerkType(clerk_types::Type type) const {
  int total = 0;
  for (unsigned int j = 0; j < num_clerks_[type]; ++j) {
    total += line_counts_[type][j];
    total += bribe_line_counts_[type][j];
  }
  return total;
}

/* Adds a new customer to the passport office by creating a new thread and
  forking it. Additionally, this will add the customer to the customers_ set.*/
void AddNewCustomer(Customer* customer) {
  std::string id = CustomerIdentifierString(&customer);
  char* id_str = new char[id.length()];
  std::strcpy(id_str, id.c_str());
  Thread* thread = new Thread(id_str);
  thread->Fork(thread_runners::RunCustomer, reinterpret_cast<int>(customer));
  thread_list_[num_threads++] = thread;
  Acquire(customer_count_lock_);
  customers[customers_size++] = customer;
  Release(customer_count_lock_);
}

/* Adds a new senator to the passport office by creating a new thread and
  forking it. */
void AddNewSenator(Senator* senator) {
  Thread* thread = new Thread("senator thread");
  thread->Fork(thread_runners::RunSenator, reinterpret_cast<int>(senator));
  thread_list_[num_threads++] = thread;
}

/* ######## Customer Functionality ######## */
Customer CreateCustomer(customer_types::Type type = customer_types::kCustomer) {
  Customer customer;
  customer.type_ = type;
  customer.money_ = INITIAL_MONEY_AMOUNTS[Rand() % NUM_INITIAL_MONEY_AMOUNTS];
  customer.ssn_ = (type == customer_types::kCustomer ?
      CURRENT_UNUSED_SSN++ : SENATOR_UNUSED_SSN++);
  customer.bribed_ = 0;
  customer.certified_ = 0;
  customer.completed_application_ = 0;
  customer.passport_verified_ = 0;
  customer.picture_taken_ = 0;
  customer.running_ = 0;
  customer.join_line_lock_ = CreateLock("jll");
  customer.join_line_lock_cv_ = CreateCondition("jllcv");
  return customer;
}

Customer CreateCustomer(int money__) {
  Customer customer;
  customer.money_ = money__;
  customer.ssn_ = CURRENT_UNUSED_SSN++; 
  customer.bribed_ = 0;
  customer.certified_ = 0;
  customer.completed_application_ = 0;
  customer.passport_verified_ = 0;
  customer.picture_taken_ = 0;
  customer.running_ = 0;
  customer.join_line_lock_ = CreateLock("jll");
  customer.join_line_lock_cv_ = CreateCondition("jllcv");
  return customer;
}

void DestroyCustomer(Customer* customer) {
  DestroyLock(customer->join_line_lock_);
  DestroyCondition(customer->join_line_lock_cv_);
}

std::string CustomerIdentifierString(Customer* customer) const {
  std::stringstream ss;
  ss << "Customer [" << customer->ssn_ << ']';
  return ss.str();
}

void DoClerkWork(Customer* customer, Clerk* clerk) {
  Acquire(clerk->wakeup_lock_);
  clerk->customer_ssn_ = customer->ssn_;
  clerk->current_customer_ = customer;
  Print(CustomerIdentifierString(customer));
  Print(" has given SSN [", 16);
  PrintNum(customer->ssn_);
  Print("] to ", 5);
  Print(ClerkIdentifierString(clerk))
  Print(".\n", 2);
      << std::endl;
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  
  switch (clerk->type_) {
    case clerk_types::kApplication:
      Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      break;
    case clerk_types::kPicture:
      clerk->customer_input_ = (Rand() % 10) > 0;
      Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      break;
    case clerk_types::kCashier:
      clerk->customer_money_ = PASSPORT_FEE;
      Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      clerk->customer_input_ = 1;
      Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      break;
    case clerk_types::kPassport:
      clerk->customer_input_ = 1;
      Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      break;
    case clerk_types::Size:
      break;
  }
}

void PrintLineJoin(Customer* customer, Clerk* clerk, int bribed) const {
  Print(CustomerIdentifierString(customer));
  Print(" has gotten in ", 15);
  Print((bribed ? "bribe" : "regular"), (bribed ? 5 : 7));
  Print(" line for "); 
  Print(ClerkIdentifierString(clerk));
  Print(".\n", 2);
}

std::string SenatorIdentifierString(Customer* customer) const {
  std::stringstream ss;
  Print("Senator [");
  PrintNum(customer->ssn_);
  Print(']');
  return ss.str();
}

void SenatorRun(Customer* customer) {
  customer->running_ = 1;
  /* Increment the number of senators in the office so that others know 
     that a senator is there. */
  Acquire(num_senators_lock_);
  ++num_senators_;
  Release(num_senators_lock_);

  Acquire(senator_lock_);

  /* Wake up customers that are currently in line in the passport office so that
    they can join the outside line. */
  while (num_customers_being_served_ > 0) {
    manager_->WakeWaitingCustomers();
    currentThread->Yield();
  }

  /* Reset the line counts to 0 since there are no customers in the office
    at this point. */
  for (unsigned i = 0; i < clerk_types::Size; ++i) {
    Acquire(line_locks_[i]);
    for (unsigned j = 0; j < num_clerks_[i]; ++j) {
      line_counts_[i][j] = 0;
      bribe_line_counts_[i][j] = 0;      
    }
    Release(line_locks_[i]);
  }
  
  /* Wait for all clerks to get off of their breaks, if necessary. */
  for (int i = 0; i < 500; ++i) { currentThread->Yield(); }

  while (customer->running_ &&
        (!customer->passport_verified_ || !customer->picture_taken_ ||
         !customer->completed_application_ || !customer->certified_) {
    clerk_types::Type next_clerk;
    if (!customer->completed_application_ && !customer->picture_taken_ && 
        num_application_clerks > 0 &&
        num_picture_clerks > 0) {
      next_clerk = (clerk_types::Type)(Rand() % 2); /* either kApplication (0) or kPicture (1)*/
    } else if (!customer->completed_application_ {
      next_clerk = clerk_types::kApplication;
    } else if (!customer->picture_taken_ {
      next_clerk = clerk_types::kPicture;
    } else if (!customer->certified_ {
      next_clerk = clerk_types::kPassport;
    } else {
      next_clerk = clerk_types::kCashier;
    }
    if (clerks_[next_clerk].empty()) {
      break;
    }
    Clerk* clerk = clerks_[next_clerk][0];
    Acquire(line_locks_[next_clerk]);
    ++line_counts_[next_clerk][0];
    Release(line_locks_[next_clerk]);
    
    Signal(manager_->wakeup_condition_, manager_->wakeup_condition_lock_);

    PrintLineJoin(clerk, bribed_);
    JoinLine(clerk, bribed_);

    DoClerkWork(clerk);

    Acquire(line_locks_[next_clerk]);
    --line_counts_[next_clerk][0];
    Release(line_locks_[next_clerk]);

    clerk->current_customer_ = NULL;
    Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
    Release(clerk->wakeup_lock_);
  }

  if (customer->passport_verified_ {
    Print(SenatorIdentifierString());
    Print(" is leaving the Passport Office.\n", 33);
  } else {
    Print(SenatorIdentifierString());
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
  customer->running_ = 1;
  while (customer->running_ &&
      (!customer->passport_verified_ || !customer->picture_taken_ ||
      !customer->completed_application_ || !customer->certified_) {
    Acquire(num_senators_lock_);
    if (num_senators_ > 0) {
      Release(num_senators_lock_);
      Acquire(outside_line_lock_);
      Print(customer->CustomerIdentifierString());
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
    clerk_types::Type next_clerk;
    if (!customer->completed_application_ && !customer->picture_taken_ && num_application_clerks > 0 && num_picture_clerks > 0) {
      next_clerk = (clerk_types::Type>)(Rand() % 2); /*either kApplication (0) or kPicture (1) */
    } else if (!customer->completed_application_) {
      next_clerk = clerk_types::kApplication;
    } else if (!customer->picture_taken_) {
      next_clerk = clerk_types::kPicture;
    } else if (!customer->certified_) {
      next_clerk = clerk_types::kPassport;
    } else {
      next_clerk = clerk_types::kCashier;
    }
    Clerk* clerk = NULL;
    Acquire(line_locks_[next_clerk]);
    int32_t shortest = -1;
    for (int i = 0; i < num_clerks_[next_clerk]; ++i) {
      if (shortest == -1 ||
          line_counts_[next_clerk][i] < line_counts_[next_clerk][shortest]) {
        shortest = i;
      }
    }
    if (customer->money_ >= CLERK_BRIBE_AMOUNT + PASSPORT_FEE) {
      int32_t bribe_shortest = -1;
      for (int i = 0; i < num_clerks_[next_clerk]; ++i) {
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
        clerk = clerks_[next_clerk][bribe_shortest];
        customer->bribed_ = 1;
      } else {
        clerk = clerks_[next_clerk][shortest];
      }
    } else {
      if (shortest == -1) {
        Release(line_locks_[next_clerk]);
        customer->running_ = 0;
        continue;
      }
      clerk = clerks_[next_clerk][shortest];
    }
    Release(line_locks_[next_clerk]);

    if (GetNumCustomersForClerkType(next_clerk) > 
        CLERK_WAKEUP_THRESHOLD) {
      Signal(manager_->wakeup_condition_, manager_->wakeup_condition_lock_);
    }

    PrintLineJoin(clerk, customer->bribed_);

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
  if (customer->passport_verified_ {
    Print(customer->CustomerIdentifierString());
    Print(" is leaving the Passport Office.\n", 33);
  } else {
    Print(customer->CustomerIdentifierString());
    Print(" terminated early because it is impossible to get a passport.\n", 62);
  }
  Acquire(customers_served_lock_);
  --num_customers_being_served_;
  if (num_customers_being_served_ == 0) {
    Broadcast(customers_served_cv_, customers_served_lock_);
  }
  Release(customers_served_lock_);
  
  Acquire(customer_count_lock_);
  customers_.erase(this);
  Release(customer_count_lock_);
}

/* ######## Clerk Functionality ######## */
const char* NameForClerkType(clerk_types::Type type) {
  static const char* NAMES[] = {
    "ApplicationClerk",
    "PictureClerk",
    "PassportClerk",
    "Cashier"
  };
  return NAMES[type];
}

std::string ClerkIdentifierString(Clerk* clerk) const {
  std::stringstream ss;
  ss << NameForClerkType(clerk->type_) << " [" << clerk->identifier_ << ']';
  return ss.str();
}

Clerk CreateClerk(int identifier, clerk_types::Type type) {
  Clerk clerk;
  clerk.customer_money_ = 0;
  clerk.customer_input_ = 0;
  clerk.state_ = clerk_states::kAvailable;
  clerk.collected_money_ = 0;
  clerk.identifier_ = identifier;
  clerk.running_ = 0;
  clerk.lines_lock_ = CreateLock("Clerk Lines Lock");
  clerk.bribe_line_lock_cv_ = CreateCondition("Clerk Bribe Line Condition"); 
  clerk.bribe_line_lock_ = CreateLock("Clerk Bribe Line Lock");
  clerk.regular_line_lock_cv_ = CreateCondition("Clerk Regular Line Condition"); 
  clerk.regular_line_lock_ = CreateLock("Clerk Regular Line Lock");
  clerk.wakeup_lock_cv_ = CreateCondition("Clerk Wakeup Condition");
  clerk.wakeup_lock_ = CreateLock("Clerk Wakeup Lock");
  clerk.money_lock_ = CreateLock("Clerk Money Lock");
  clerk.type_ = type;

  switch(type) {
    case clerk_types::kApplication :     
      clerk.clerk_type_ = "ApplicationClerk";
      break;
    case clerk_types::kPicture :
      clerk.clerk_type_ = "PictureClerk";
      break;
    case clerk_types::kPassport :
      clerk.clerk_type_ = "PassportClerk";
      break;
    case clerk_types::kCashier :
      clerk.clerk_type_ = "Cashier";
      break;
  }

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
  Acquire(clerk->money_lock_);
  int money = clerk->collected_money_;
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
      Signal(manager_->wakeup_condition_, manager_->wakeup_condition_lock_);
    }
    Wait(clerk->bribe_line_lock_cv_, clerk->bribe_line_lock_);
    Release(clerk->bribe_line_lock_);
  } else {
    Acquire(clerk->regular_line_lock_);
    ++clerk->line_counts_[clerk->type_][clerk->identifier_];
    if (GetNumCustomersForClerkType(clerk->type_) > 
        CLERK_WAKEUP_THRESHOLD) {
      Signal(manager_->wakeup_condition_, manager_->wakeup_condition_lock_);
    }
    Wait(clerk->regular_line_lock_cv_ , clerk->regular_line_lock_);
    Release(clerk->regular_line_lock_);
  }
}

int GetNumCustomersInLine(Clerk* clerk) const {
  return line_counts_[clerk->type_][clerk->identifier_] +
      bribe_line_counts_[clerk->type_][clerk->identifier_];
}

void GetNextCustomer(Clerk* clerk) {
  Acquire(line_locks_[clerk->type_]);
  Acquire(clerk->bribe_line_lock_);
  int bribe_line_count = bribe_line_counts_[clerk->type_][clerk->identifier_];
  Release(clerk->bribe_line_lock_);

  Acquire(clerk->regular_line_lock_);
  int regular_line_count = line_counts_[clerk->type_][clerk->identifier_];
  Release(clerk->regular_line_lock_);

  if (bribe_line_count > 0) {
    Print(clerk->clerk_type_);
    Print(" [", 2);
    PrintNum(clerk->identifier_);
    Print("] has signalled a Customer to come to their counter.\n", 53);

    Acquire(clerk->bribe_line_lock_);
    Acquire(clerk->wakeup_lock_);
    Signal(clerk->bribe_line_lock_cv_, clerk->bribe_line_lock_);
    Release(clerk->bribe_line_lock_);
    clerk->state_ = clerk_states::kBusy;
    bribe_line_counts_[clerk->type_][clerk->identifier_]--;
  } else if (regular_line_count > 0) {
    Print(clerk->clerk_type_);
    Print(" [", 2);
    PrintNum(clerk->identifier_);
    Print("] has signalled a Customer to come to their counter.\n", 53);

    Acquire(clerk->regular_line_lock_);
    Acquire(clerk->wakeup_lock_);
    Signal(clerk->regular_line_lock_cv_, clerk->regular_line_lock_);
    Release(clerk->regular_line_lock_);
    clerk->state_ = clerk_states::kBusy;
    line_counts_[clerk->type_][clerk->identifier_]--;
  } else {
    clerk->state_ = clerk_states::kOnBreak;
  }
  Release(line_locks_[clerk->type_]);
}

void ApplicationClerkWork(Clerk* clerk) {
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  /* Wait for customer to put passport on counter. */
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  clerk->current_customer_->completed_application_ = 1;
  Print(clerk->clerk_type_);
  Print(" [", 2);
  PrintNum(clerk->identifier);
  Print("] has recorded a completed application for ", 43);
  Print(clerk->current_customer_->IdentifierString());
  Print("\n", 1);
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
}

void PictureClerkWork(Clerk* clerk) {
  /* Take Customer's picture and wait to hear if they like it. */
  Print(" [", 2);
  PrintNum(clerk->identifier_);
  Print("] has taken a picture of ", 25);
  Print(clerk->current_customer_->IdentifierString());
  Print("\n", 1);

  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  int picture_accepted = customer_input_;

  /* If they don't like their picture don't set their picture to taken.  They go back in line. */
  if (!picture_accepted) {
    Print(clerk_type_);
    Print(" [", 2);
    PrintNum(clerk->identifier_);
    Print("] has been told that ", 21);
    Print(clerk->current_customer_->IdentifierString());
    Print(" does not like their picture\n", 28);
  } else {
    /* Set picture taken. */
    clerk->current_customer_->picture_taken_ = 1;
    Print(clerk->clerk_type_);
    Print(" [", 2);
    PrintNum(clerk->identifier_); 
    Print("] has been told that ", 21);
    Print(clerk->current_customer_->IdentifierString());
    Print(" does like their picture\n", 25);
  }
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
}

void PassportClerkWork(Clerk* clerk) {
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  /* Wait for customer to show whether or not they got their picture taken and passport verified. */
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  int picture_taken_and_passport_verified = clerk->customer_input_;

  /* Check to make sure their picture has been taken and passport verified. */
  if (!picture_taken_and_passport_verified) {
    Print(clerk->clerk_type_);
    Print(" [", 2);
    PrintNum(clerk->identifier_);
    Print("] has determined that ", 22);
    Print(clerk->current_customer_->IdentifierString());
    Print(" does not have both their application and picture completed\n", 60);
  } else {
    Print(clerk->clerk_type_);
    Print(" [", 2);
    PrintNum(clerk->identifier_); 
    Print("] has determined that ", 22);
    Print(clerk->current_customer_->IdentifierString());
    Print(" does have both their application and picture completed\n", 46);
    clerk->current_customer_->certified_ = 1;
    Print(clerk->clerk_type_);
    Print(" [", 2);
    PrintNum(clerk->identifier_); 
    Print("] has recorded ", 16);
    Print(clerk->current_customer_->IdentifierString());
    Print(" passport documentation\n", 24);
  }
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
}

void CashierClerkWork(Clerk* clerk) {
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
  int certified = clerk->customer_input_;

  /* Check to make sure they have been certified. */
  if (!certified) {
    std::cout << clerk->clerk_type_ << " [" << clerk->identifier_ 
        << "] has received the $100 from "
        << clerk->current_customer_->IdentifierString() 
        << " before certification. They are to go to the back of my line." 
        << std::endl;

    /* Give money back. */
    Acquire(clerk->money_lock_);
    clerk->collected_money_ -= 100;
    Release(clerk->money_lock_);
    clerk->current_customer_->money_ += 100;
  } else {
    std::cout << clerk->clerk_type_ << " [" << clerk->identifier_ 
        << "] has received the $100 from "
        << clerk->current_customer_->IdentifierString() 
        << " after certification." << std::endl;

    /* Give customer passport. */
    std::cout << clerk->clerk_type_ << " [" << clerk->identifier_ 
        << "] has provided "
        << clerk->current_customer_->IdentifierString() 
        << " their completed passport." << std::endl;

    /* Record passport was given to customer. */
    std::cout << clerk->clerk_type_ << " [" << clerk->identifier_ 
        << "] has recorded that "
        << clerk->current_customer_->IdentifierString() 
        << " has been given their completed passport." << std::endl;

    clerk->current_customer_->passport_verified_ = 1;
  }
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
}

void ClerkRun(Clerk* clerk) {
  clerk->running_ = 1;
  while (clerk->running_) {
    GetNextCustomer(clerk);
    if (clerk->state_ == clerk_states::kBusy || clerk->state_ == clerk_states::kAvailable) {
      /* Wait for customer to come to counter and give SSN. */
      Wait(clerk->wakeup_lock_cv_ , clerk->wakeup_lock_);
      /* Take Customer's SSN and verify passport. */
      std::cout << IdentifierString() << " has received SSN "
                << clerk->customer_ssn_ << " from "
                << clerk->current_customer_->IdentifierString()
                << std::endl;
      /* Do work specific to the type of clerk. */
      switch(clerk->type_) {
        case clerk_types::kApplication :     
          ApplicationClerkWork(clerk);
          break;
        case clerk_types::kPicture :
          PictureClerkWork(clerk);
          break;
        case clerk_types::kPassport :
          PassportClerkWork(clerk);
          break;
        case clerk_types::kCashier :
          CashierClerkWork(clerk);
          break;
      }

      /* Collect bribe money. */
      if (clerk->current_customer_->bribed_) {
        Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
        int bribe = clerk->customer_money_;
        clerk->customer_money_ = 0;
        Acquire(clerk->money_lock_);
        clerk->collected_money_ += bribe;
        Release(clerk->money_lock_);
        std::cout << IdentifierString() << " has received $"
                  << bribe << " from Customer " << clerk->customer_ssn_
                  << std::endl;
        Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      }
      Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      /* Random delay. */
      int random_time = Rand() % 80 + 20;
      for (int i = 0; i < random_time; ++i) {
        currentThread->Yield();
      }
      /* Wakeup customer. */
    } else if (clerk->state_ == clerk_states::kOnBreak) {
      Acquire(clerk->wakeup_lock_);
      /* Wait until woken up. */
      std::cout << IdentifierString() << " is going on break" << std::endl;
      Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      clerk->state_ = clerk_states::kAvailable;
      if (!clerk->running_) {
        break;
      }
      std::cout << IdentifierString() << " is coming off break" << std::endl;
      for (int i = 0; i < 25; ++i) { currentThread->Yield(); }
    }
    Release(clerk->wakeup_lock_);
  }
}

/* ######## Manager Functionality ######## */
Manager CreateManager() {
  Manager manager;
  manager.wakeup_condition_ = CreateCondition("Manager Wakeup Lock Condition");
  manager.wakeup_condition_lock_ = CreateLock("Manager Wakeup Lock");
  running_ = true;
  elapsed_ = 0;
  return manager;
}

void ManagerPrintMoneyReport(Manager* manager) {
  while (manager->running_) {
    for (int j = 0; j < clerk_types::Size; ++j) {
      for (int i = 0; i < num_clerks_[j]; ++i) {
        int m = clerks_[j][i]->CollectMoney();
        manager->money_[clerks_[j][i]->type_] += m;
      }
    }
    int total = 0;
    for (int i = 0; i < clerk_types::Size; ++i) {
      total += manager->money_[i];
      std::cout << "Manager has counted a total of $" << manager->money_[i] << " for "
                << Clerk::NameForClerkType(static_cast<clerk_types::Type>(i)) << 's' << std::endl;
    }
    std::cout << "Manager has counted a total of $" << total
              <<  " for the passport office" << std::endl;
    for(int i = 0; i < 200; ++i) {
      if (!manager->running_) return;
      Yield();
    }
  }
}

void ManagerRun(Manager* manager) {
  manager->running_ = true;
  Thread* report_timer_thread = new Thread("Report timer thread");
  report_timer_thread->Fork(&RunPrintMoney, reinterpret_cast<int>(this));
  while(manager->running_) {
    Acquire(manager->wakeup_condition_lock_);
    Wait(manager->wakeup_condition_, manager->wakeup_condition_lock_);
    if (!manager->running_) {
      break;
    }
    for (int i = 0; i < clerk_types::Size; ++i) {
      Acquire(line_locks_[i]);
    }
    for (int i = 0; i < clerk_types::Size; ++i) {
      if (GetNumCustomersForClerkType(
          static_cast<clerk_types::Type>(i)) > CLERK_WAKEUP_THRESHOLD ||
          num_senators_ > 0) {
        for (int j = 0; j < num_clerks[i]; ++j) {
          Clerk* clerk = clerks_[i][j];
          if (clerk->state_ == clerk_states::kOnBreak) {
            Acquire(clerk->wakeup_lock_);
            Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
            clerk->state_ = clerk_states::kAvailable;
            Release(clerk->wakeup_lock_);
            std::cout << "Manager has woken up a"
              << (clerk->type_ == clerk_types::kApplication ? "n " : " ")
              << Clerk::NameForClerkType(clerk->type_) << std::endl;
          }
        }
      }
    }
    for (int i = 0; i < clerk_types::Size; ++i) {
      Release(line_locks_[i]);
    }
    Release(wakeup_condition_lock_);
  }
}

void WakeWaitingCustomers() {
  for (int i = 0; i < clerk_type_;:Size; ++i) {
    for (int j = 0; j < num_clerks_[i]; ++j) {
      Clerk* clerk = clerks_[i][j];
      Broadcast(clerk->bribe_line_lock_cv_, clerk->bribe_line_lock_);
      Broadcast(clerk->regular_line_lock_cv_, clerk->regular_line_lock_);
    }
  }
}

void WakeClerksForSenator() {
  for (int i = 0; i < clerk_types::Size; ++i) {
    if (!clerks_[i].empty()) {
      Acquire(line_locks_[i]);
      Clerk* clerk = clerks_[i][0];
      if (clerk->state_ == clerk_states::kOnBreak) {
        clerk->state_ = clerk_states::kAvailable;
        Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      }
      Release(line_locks_[i]);
    }
  }
}

/* ######## Thread Runners ######## */
namespace thread_runners {

  void RunManager(int arg) {
    Manager* manager = reinterpret_cast<Manager*>(arg);
    ManagerRun(manager);
  }

  void RunClerk(int arg) {
    Clerk* clerk = reinterpret_cast<Clerk*>(arg);
    ClerkRun(clerk);
  }

  void RunCustomer(int arg) {
    Customer* customer = reinterpret_cast<Customer*>(arg);
    CustomerRun(customer);
  }

  void RunSenator(int arg) {
    Senator* senator = reinterpret_cast<Senator*>(arg);
    SenatorRun(senator);
  }
}


int main() {
  for (int i = 0; i < clerk_types::Size; ++i) {
    char* name = new char[80];
    sprintf(name, "Line Lock %d", i);
    line_locks_[i] = CreateLock(name);
  }

  Clerk temp_app[num_application_clerks];
  clerks_[clerk_types::kApplication] = temp_app;
  int temp_line_app[num_application_clerks];
  line_counts_[clerk_types::kApplication] = temp_line_app;
  int temp_bribe_app[num_application_clerks];
  bribe_line_counts_[clerk_types::kApplication] = temp_bribe_app;

  for (int i = 0; i < num_application_clerks; ++i) {
    clerks_[clerk_types::kApplication][i] = CreateClerk(i, clerk_types::kApplication));
    line_counts_[clerk_types::kApplication][i] = 0;
    bribe_line_counts_[clerk_types::kApplication][i] = 0;
  }

  Clerk temp_picture[num_picture_clerks];
  clerks_[clerk_types::kPicture] = temp_picture;
  int temp_line_picture[num_picture_clerks];
  line_counts_[clerk_types::kPicture] = temp_line_picture;
  int temp_bribe_picture[num_picture_clerks];
  bribe_line_counts_[clerk_types::kPicture] = temp_bribe_picture;

  for (int i = 0; i < num_picture_clerks; ++i) {
    clerks_[clerk_types::kPicture][i] = CreateClerk(i, clerk_types::kPicture);
    line_counts_[clerk_types::kPicture][i] = 0;
    bribe_line_counts_[clerk_types::kPicture][i] = 0;
  }

  Clerk temp_passport[num_paassport_clerks];
  clerks_[clerk_types::kPassport] = temp_passport;
  int temp_line_passport[num_passport_clerks];
  line_counts_[clerk_types::kPassport] = temp_line_passport;
  int temp_bribe_passport[num_passport_clerks];
  bribe_line_counts_[clerk_types::kPassport] = temp_bribe_passport;

  for (int i = 0; i < num_passport_clerks; ++i) {
    clerks_[clerk_types::kPassport][i] = CreateClerk(i, clerk_types::kPassport);
    line_counts_[clerk_types::kPassport][i] = 0;
    bribe_line_counts_[clerk_types::kPassport][i] = 0;
  }

  Clerk temp_cashier[num_cashier_clerks];
  clerks_[clerk_types::kCashier] = temp_cashier;
  int temp_line_cashier[num_cashier_clerks];
  line_counts_[clerk_types::kCashier] = temp_line_cashier;
  int temp_bribe_cashier[num_cashier_clerks];
  bribe_line_counts_[clerk_types::kCashier] = temp_bribe_cashier;

  for (int i = 0; i < num_cashier_clerks; ++i) {
    clerks_[clerk_types::kCashier][i] = CreateClerk(i, clerk_types::kCashier);
    line_counts_[clerk_types::kCashier][i] = 0;
    bribe_line_counts_[clerk_types::kCashier][i] = 0;
  }

  // TODO make manager a thread.
  manager_ = CreateManager(this);

  /* Start the Passport Office */
  manager_thread_.Fork(
      thread_runners::RunManager, reinterpret_cast<int>(manager_));
  for (unsigned i = 0; i < clerk_types::Size; ++i) {
    for (unsigned j = 0; j < num_clerks_[i]; ++j) {
      std::string id = ClerkIdentifierString(clerks_[i][j]);
      char* id_str = new char[id.length()];
      std::strcpy(id_str, id.c_str());
      Thread* thread = new Thread(id_str);
      thread_list_[num_threads++] = thread;
      thread->Fork(
          thread_runners::RunClerk, reinterpret_cast<int>(clerks_[i][j]));
    }
  }

  while (num_customers + num_senators > 0) {
    int next_customer_type = Rand() % (num_customers + num_senators);
    if (next_customer_type >= num_customers) {
      AddNewSenator(CreateCustomer(customer_types::kSenator));
      --num_senators;
    } else {
      AddNewCustomer(CreateCustomer());
      --num_customers;
    }
  }

  /* WaitOnFinish for the Passport Office */
  while (customers_size > 0) {
    for (int i = 0; i < 400; ++i) {
      currentThread->Yield();
    }
    if (num_senators_ > 0) continue;
    Acquire(num_customers_waiting_lock_);
    if (customers_size == num_customers_waiting_) {
      Release(num_customers_waiting_lock_);
      bool done = true;
      Acquire(breaking_clerks_lock_);
      for (unsigned int i = 0; i < clerk_types::Size; ++i) {
        Acquire(line_locks_[i]);
        for (unsigned int j = 0; j < num_clerks_[i]; ++j) {
          if (clerks_[i][j].state_ != clerk_states::kOnBreak ||
              GetNumCustomersForClerkType(static_cast<clerk_types::Type>(i)) > 
              CLERK_WAKEUP_THRESHOLD) {
            done = false;
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
  for (int i = 0; i < 1000; ++i) {
    currentThread->Yield();
  }

  /* Stop the Passport Office */
  Print("Attempting to stop passport office\n");
  while (customers_size > 0) {
    bool done = true;
    Acquire(breaking_clerks_lock_);
    for (unsigned int i = 0; i < clerk_types::Size; ++i) {
      for (unsigned int j = 0; j < num_clerks_[i]; ++j) {
        if (clerks_[i][j].state_ != clerk_states::kOnBreak) {
          done = false;
        }
      }
    }
    Release(breaking_clerks_lock_);
    if (done) {
      break;
    } else {
      for (int i = 0; i < 100; ++i) {
        currentThread->Yield();
      }
    }
  }
  for (int i = 0; i < customers_size; ++itr) {
    customers[i].running_ = false;
  }
  for (unsigned int i = 0; i < clerk_types::Size; ++i) {
    Acquire(line_locks_[i]);
    for (unsigned int j = 0; j < num_clerks_[i]; ++j) {
      Acquire(clerks_[i][j].regular_line_lock_);
      Broadcast(clerks_[i][j].regular_line_lock_cv_, clerks_[i][j].regular_line_lock_);
      Release(clerks_[i][j].regular_line_lock_);
      Acquire(clerks_[i][j].bribe_line_lock_);
      Broadcast(clerks_[i][j].bribe_line_lock_cv_, clerks_[i][j].bribe_line_lock_);
      Release(clerks_[i][j].bribe_line_lock_);
      clerks_[i][j].running_ = false;
      Acquire(clerks_[i][j].wakeup_lock_);
      Broadcast(clerks_[i][j].wakeup_lock_cv_, clerks_[i][j].wakeup_lock_);
      Release(clerks_[i][j].wakeup_lock_);
    }
    Release(line_locks_[i]);
  }
  manager_.running = false;
  Print("Set manager running false\n");
  Acquire(manager_.wakeup_condition_lock_);
  Signal(manager_.wakeup_condition_, manager_.wakeup_condition_lock_);
  Release(manager_.wakeup_condition_lock_);
  for (int i = 0; i < 1000; ++i) {
    currentThread->Yield();
  }
  Print("Passport office stopped\n");
  /*for (unsigned int i = 0 ; i < thread_list_.size(); ++i) {
    thread_list_[i]->Finish();
  }
  manager_thread_.Finish();*/


  currentThread->Finish();

  /* Delete Locks and CVs */
  DeleteLock(breaking_clerks_lock_);
  DeleteLock(senator_lock_);
  DeleteCondition(senator_condition_;
  DeleteLock(customer_count_lock_;
  DeleteLock(customers_served_lock_);
  DeleteCondition(customers_served_cv_);
  DeleteLock(num_customers_waiting_lock_);
  DeleteLock(num_senators_lock_);
  DeleteLock(outside_line_lock_);
  DeleteCondition(outside_line_cv_);
  for (int i = 0; i < clerk_types::Size; ++i) {
    DeleteLock(line_locks_[i]);
  }

  for (int i = 0; i < clerk_types::Size; ++i) {
    for (int j = 0; j < num_clerks_[i]; ++j) {
      DeleteClerk(clerks[i][j]);
    }
  }

  for (int i = 0; i < num_customers; ++i) {
    DeleteCustomer(customers[i]);
  }

  DeleteManager(manager_);
  return 0;
}
