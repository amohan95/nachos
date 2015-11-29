 
















 




































 









 
void Halt();		
 

 

 
void Exit(int status);	

 
typedef int SpaceId;	
 
 


SpaceId Exec(char *name, int len);
 
 


int Join(SpaceId id); 	
 

 







 
 
typedef int OpenFileId;	

 







 
 
void Create(char *name, int size);

 


OpenFileId Open(char *name, int size);

 
void Write(char *buffer, int size, OpenFileId id);

 





int Read(char *buffer, int size, OpenFileId id);

 
void Close(OpenFileId id);



 



 


void Fork(void (*func)());

 


void Yield();


 


 


int CreateLock(char* name, int len);
 


int DestroyLock(int lock);
 


int CreateCondition(char* name, int len);
 


int DestroyCondition(int cv);
 


int Acquire(int lock);
 


int Release(int lock);
 



int Wait(int cv, int lock);
 



int Signal(int cv, int lock);
 



int Broadcast(int cv, int lock);

int Rand();
void Print(char* string, int len);
void PrintNum(int num);
int CreateMonitor(char* name, int len, int arr_size);
int SetMonitor(int mv, int index, int value);
int GetMonitor(int mv, int index);
int DestroyMonitor(int mv);

int Sprintf(char* dst, char* format, int len, int x);












typedef enum ClerkStates {
  kAvailable = 0,
  kBusy,
  kOnBreak,
} ClerkState;

typedef enum ClerkTypes {
  kApplication = 0,
  kPicture,
  kPassport,
  kCashier,
} ClerkType;

typedef enum CustomerTypes {
  kCustomer = 0,
  kSenator,  
} CustomerType;

struct Clerk;
struct Customer;
struct Manager;






typedef struct Customers {
  int money_;
  int ssn_;
  int join_line_lock_;
  int join_line_lock_cv_;
  int bribed_;
  int certified_;
  int completed_application_;
  int passport_verified_;
  int picture_taken_;
  int running_;
  CustomerType type_;
} Customer;

typedef struct Clerks {
  int lines_lock_;
  int bribe_line_lock_cv_;
  int bribe_line_lock_;
  int regular_line_lock_cv_;
  int regular_line_lock_;
  int wakeup_lock_cv_;
  int wakeup_lock_;
  int money_lock_;
  int customer_ssn_;
  Customer* current_customer_;
  int customer_money_;
  int customer_input_;
  ClerkType type_;
  ClerkState state_;
  int collected_money_;
  int identifier_;
  int running_;
} Clerk;

typedef struct Managers {
  int wakeup_condition_;
  int wakeup_condition_lock_;
  int elapsed_;
  int running_;
  int money_[4 ];
} Manager;












char nameBuf[128];
int nameLen;

int num_clerks_[4 ] 
    = {3 , 3 , 3 , 
        3 };



int breaking_clerks_lock_;
int senator_lock_;
int senator_condition_;
int customer_count_lock_;
int customers_served_lock_;
int customers_served_cv_;
int num_customers_being_served_;
int num_customers_waiting_lock_;
int num_customers_waiting_;
int num_senators_lock_;
int num_senators_;
int outside_line_lock_;
int outside_line_cv_;

int line_locks_[4 ];
Customer customers_[15  + 0 ];
int customers_size_;

Clerk clerks_[4 ][5 ];

int line_counts_[4 ];
int bribe_line_counts_[4 ];


Manager manager_;


int customer_index_;
int clerk_index_[4 ];


int NUM_INITIAL_MONEY_AMOUNTS = 4;
int INITIAL_MONEY_AMOUNTS[] = {100, 600, 1100, 1600};
int CURRENT_UNUSED_SSN = 0;
int SENATOR_UNUSED_SSN = 0;

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
int GetNumCustomersForClerkType(ClerkType type);

void WakeWaitingCustomers();
void WakeClerksForSenator();

void SetupPassportOffice();

void CreateCustomer(Customer* customer, CustomerType type, int money) {
  customer->type_ = type;
  customer->money_ =
      (money == 0 ? INITIAL_MONEY_AMOUNTS[Rand() % NUM_INITIAL_MONEY_AMOUNTS]
                  : money);
  customer->ssn_ = (type == kCustomer ?
      CURRENT_UNUSED_SSN++ : SENATOR_UNUSED_SSN++);
  customer->bribed_ = 0;
  customer->certified_ = 0;
  customer->completed_application_ = 0;
  customer->passport_verified_ = 0;
  customer->picture_taken_ = 0;
  customer->running_ = 0;
}


int GetNumCustomersForClerkType(ClerkType type) {
  int total = 0;
  int i;
  int blc, lc;
  for (i = 0; i < num_clerks_[type]; ++i) {
   /* lc = GetMonitor(line_counts_[type], i);
    if (lc == -1) {
      Print("Error getting line count\n", 25);
      break;
    }
    total += lc;
    blc =  GetMonitor(bribe_line_counts_[type], i);
    if (blc == -1) {
      Print("Error getting bribe line count\n", 31);
      break;
    }*/
    total += blc;
  }
  return total;
}


void DestroyCustomer(Customer* customer) {
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
      clerk->customer_money_ = 100 ;
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
    case 4 :
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

void WakeWaitingCustomers() {
  int i, j;
  Clerk* clerk;

  for (i = 0; i < 4 ; ++i) {
    for (j = 0; j < num_clerks_[i]; ++j) {
      clerk = &(clerks_[i][j]);
      Broadcast(clerk->bribe_line_lock_cv_, clerk->bribe_line_lock_);
      Broadcast(clerk->regular_line_lock_cv_, clerk->regular_line_lock_);
    }
  }
}

void SenatorRun(Customer* senator) {
  int i, j;
  ClerkType next_clerk;
  Clerk* clerk;
  int num_senators;



  senator->running_ = 1;
   

  Acquire(num_senators_lock_);

  SetMonitor(num_senators_, 0, GetMonitor(num_senators, 0) + 1);

  Release(num_senators_lock_);

  Acquire(senator_lock_);

   


  while (GetMonitor(num_customers_being_served_, 0) > 0) {

    WakeWaitingCustomers();
    Yield();
  }

   

  for (i = 0; i < 4 ; ++i) {
    Acquire(line_locks_[i]);
    for (j = 0; j < num_clerks_[i]; ++j) {

      SetMonitor(line_counts_[i], j, 0);
      SetMonitor(bribe_line_counts_[i], j, 0);

    }
    Release(line_locks_[i]);
  }
  
   
  for (i = 0; i < 500; ++i) { Yield(); }

  while (senator->running_ &&
        (!senator->passport_verified_ || !senator->picture_taken_ ||
         !senator->completed_application_ || !senator->certified_)) {
    if (!senator->completed_application_ && !senator->picture_taken_ && 
        3  > 0 &&
        3  > 0) {
      next_clerk = (ClerkType)(Rand() % 2);  
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

    SetMonitor(line_counts_[next_clerk], 0, GetMonitor(line_counts_[next_clerk], 0) + 1);

    Release(line_locks_[next_clerk]);

    Signal(manager_.wakeup_condition_, manager_.wakeup_condition_lock_);

    PrintLineJoin(senator, clerk, 0);
    JoinLine(clerk, 0);

    DoClerkWork(senator, clerk);

    Acquire(line_locks_[next_clerk]);

    SetMonitor(line_counts_[next_clerk], 0, GetMonitor(line_counts_[next_clerk], 0) - 1);

    Release(line_locks_[next_clerk]);

    clerk->current_customer_ = 0 ;
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

   


  Acquire(num_senators_lock_);

  num_senators = GetMonitor(num_senators_, 0) - 1;
  SetMonitor(num_senators_, 0, num_senators);

  Release(num_senators_lock_);
  if (num_senators == 0) {
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
  int num_customers_being_served;



  customer->running_ = 1;
  Print("Starting Customer\n", 18);
  while (customer->running_ &&
      (!customer->passport_verified_ || !customer->picture_taken_ ||
      !customer->completed_application_ || !customer->certified_)) {
    Acquire(num_senators_lock_);

    if (GetMonitor(num_senators_, 0) > 0) {

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

    SetMonitor(num_customers_being_served_, 0, GetMonitor(num_customers_being_served_, 0) + 1);

    Release(customers_served_lock_);

    customer->bribed_ = 0;
    if (!customer->completed_application_ && !customer->picture_taken_ && 3  > 0 && 3  > 0) {
      next_clerk = (ClerkType)(Rand() % 2);  
    } else if (!customer->completed_application_) {
      next_clerk = kApplication;
    } else if (!customer->picture_taken_) {
      next_clerk = kPicture;
    } else if (!customer->certified_) {
      next_clerk = kPassport;
    } else {
      next_clerk = kCashier;
    }
    clerk = 0 ;
    Acquire(line_locks_[next_clerk]);
    shortest = -1;
    for (i = 0; i < num_clerks_[next_clerk]; ++i) {
      if (shortest == -1 ||

          GetMonitor(line_counts_[next_clerk], i) <
          GetMonitor(line_counts_[next_clerk], shortest)) {

        shortest = i;
      }
    }
    if (customer->money_ >= 500  + 100 ) {
      bribe_shortest = -1;
      for (i = 0; i < num_clerks_[next_clerk]; ++i) {
        if (bribe_shortest == -1 ||

            GetMonitor(bribe_line_counts_[next_clerk], i)
            < GetMonitor(bribe_line_counts_[next_clerk], bribe_shortest)) {

          bribe_shortest = i;
        }
      }
      if (shortest == -1) {
        customer->running_ = 0;
        Release(line_locks_[next_clerk]);
        continue;
      }

      if (GetMonitor(bribe_line_counts_[next_clerk], bribe_shortest)
          < GetMonitor(line_counts_[next_clerk], shortest)) {

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
        3 ) {
      Signal(manager_.wakeup_condition_, manager_.wakeup_condition_lock_);
    }

    PrintLineJoin(customer, clerk, customer->bribed_);

    Acquire(num_customers_waiting_lock_);

    SetMonitor(num_customers_waiting_, 0, GetMonitor(num_customers_waiting_, 0) + 1);

    Release(num_customers_waiting_lock_);

    JoinLine(clerk, customer->bribed_);

    Acquire(num_customers_waiting_lock_);

    SetMonitor(num_customers_waiting_, 0, GetMonitor(num_customers_waiting_, 0) - 1);

    Release(num_customers_waiting_lock_);

    Acquire(customers_served_lock_);

    SetMonitor(num_customers_being_served_, 0, GetMonitor(num_customers_being_served_, 0) + 1);

    Release(customers_served_lock_);
    Acquire(num_senators_lock_);


    if (GetMonitor(num_senators_, 0) > 0) {

      Release(num_senators_lock_);
      Acquire(customers_served_lock_);

      if (GetMonitor(num_customers_being_served_, 0)  == 0) {

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
      clerk->customer_money_ = 500 ;
      customer->money_ -= 500 ;
      Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
    }
    clerk->current_customer_ = 0 ;
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

  num_customers_being_served = GetMonitor(num_customers_being_served_, 0) - 1;
  SetMonitor(num_customers_being_served_, 0, num_customers_being_served);

  if (num_customers_being_served == 0) {
    Broadcast(customers_served_cv_, customers_served_lock_);
  }
  Release(customers_served_lock_);

  Acquire(customer_count_lock_);

  SetMonitor(customers_size_, 0, GetMonitor(customers_size_, 0) - 1);

  Release(customer_count_lock_);
}

 
void PrintNameForClerkType(ClerkType type) {
  switch (type) {
    case kApplication:
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

void CreateClerk(Clerk* clerk, int identifier, ClerkType type) {
  clerk->customer_money_ = 0;
  clerk->customer_input_ = 0;
  clerk->state_ = kAvailable;
  clerk->collected_money_ = 0;
  clerk->identifier_ = identifier;
  clerk->running_ = 0;
  clerk->lines_lock_ = CreateLock("Clerk Lines Lock", 16);
  clerk->bribe_line_lock_cv_ = CreateCondition("Clerk Bribe Line Condition", 26); 
  clerk->bribe_line_lock_ = CreateLock("Clerk Bribe Line Lock", 21);
  clerk->regular_line_lock_cv_ = CreateCondition("Clerk Regular Line Condition", 28); 
  clerk->regular_line_lock_ = CreateLock("Clerk Regular Line Lock", 23);
  clerk->wakeup_lock_cv_ = CreateCondition("Clerk Wakeup Condition", 22);
  clerk->wakeup_lock_ = CreateLock("Clerk Wakeup Lock", 17);
  clerk->money_lock_ = CreateLock("Clerk Money Lock", 16);
  clerk->type_ = type;
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

    SetMonitor(bribe_line_counts_[clerk->type_], clerk->identifier_,
        GetMonitor(bribe_line_counts_[clerk->type_], clerk->identifier_) + 1);

    if (GetNumCustomersForClerkType(clerk->type_) > 
        3 ) {
      Signal(manager_.wakeup_condition_, manager_.wakeup_condition_lock_);
    }
    Wait(clerk->bribe_line_lock_cv_, clerk->bribe_line_lock_);
    Release(clerk->bribe_line_lock_);
  } else {
    Acquire(clerk->regular_line_lock_);

    SetMonitor(line_counts_[clerk->type_], clerk->identifier_,
        GetMonitor(line_counts_[clerk->type_], clerk->identifier_) + 1);

    if (GetNumCustomersForClerkType(clerk->type_) > 
        3 ) {
      Signal(manager_.wakeup_condition_, manager_.wakeup_condition_lock_);
    }
    Wait(clerk->regular_line_lock_cv_ , clerk->regular_line_lock_);
    Release(clerk->regular_line_lock_);
  }
}

int GetNumCustomersInLine(Clerk* clerk) {

  return GetMonitor(line_counts_[clerk->type_], clerk->identifier_) +
      GetMonitor(bribe_line_counts_[clerk->type_], clerk->identifier_);

}

void GetNextCustomer(Clerk* clerk) {
  int bribe_line_count;
  int regular_line_count;

  Acquire(line_locks_[clerk->type_]);
  Acquire(clerk->bribe_line_lock_);

  bribe_line_count = GetMonitor(bribe_line_counts_[clerk->type_], clerk->identifier_);

  Release(clerk->bribe_line_lock_);

  Acquire(clerk->regular_line_lock_);

  regular_line_count = GetMonitor(line_counts_[clerk->type_], clerk->identifier_);

  Release(clerk->regular_line_lock_);

  if (bribe_line_count > 0) {
    PrintClerkIdentifierString(clerk);
    Print(" has signalled a Customer to come to their counter.\n", 52);

    Acquire(clerk->bribe_line_lock_);
    Acquire(clerk->wakeup_lock_);
    Signal(clerk->bribe_line_lock_cv_, clerk->bribe_line_lock_);
    Release(clerk->bribe_line_lock_);
    clerk->state_ = kBusy;

    SetMonitor(bribe_line_counts_[clerk->type_], clerk->identifier_,
        GetMonitor(bribe_line_counts_[clerk->type_], clerk->identifier_) - 1);

  } else if (regular_line_count > 0) {
    PrintClerkIdentifierString(clerk);
    Print(" has signalled a Customer to come to their counter.\n", 52);

    Acquire(clerk->regular_line_lock_);
    Acquire(clerk->wakeup_lock_);
    Signal(clerk->regular_line_lock_cv_, clerk->regular_line_lock_);
    Release(clerk->regular_line_lock_);
    clerk->state_ = kBusy;

    SetMonitor(line_counts_[clerk->type_], clerk->identifier_,
        GetMonitor(line_counts_[clerk->type_], clerk->identifier_) - 1);

  } else {
    clerk->state_ = kOnBreak;
  }
  Release(line_locks_[clerk->type_]);
}

void ApplicationClerkWork(Clerk* clerk) {
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
   
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
   
  Print(" [", 2);
  PrintNum(clerk->identifier_);
  Print(" has taken a picture of ", 24);
  PrintCustomerIdentifierString(clerk->current_customer_);
  Print("\n", 1);

  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  picture_accepted = clerk->customer_input_;

   
  if (!picture_accepted) {
    PrintClerkIdentifierString(clerk);
    Print(" has been told that ", 20);
    PrintCustomerIdentifierString(clerk->current_customer_);
    Print(" does not like their picture.\n", 30);
  } else {
     
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
   
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  picture_taken_and_passport_verified = clerk->customer_input_;

   
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
   
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  Acquire(clerk->money_lock_);
  clerk->collected_money_ += clerk->customer_money_;
  clerk->customer_money_ = 0;
  Release(clerk->money_lock_);

   
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  certified = clerk->customer_input_;

   
  if (!certified) {
    PrintClerkIdentifierString(clerk);
    Print(" has received the $100 from ", 28);
    PrintCustomerIdentifierString(clerk->current_customer_); 
    Print(" before certification. They are to go to the back of my line.\n", 62);

     
    Acquire(clerk->money_lock_);
    clerk->collected_money_ -= 100;
    Release(clerk->money_lock_);
    clerk->current_customer_->money_ += 100;
  } else {
    PrintClerkIdentifierString(clerk);
    Print(" has received the $100 from ", 28);
    PrintCustomerIdentifierString(clerk->current_customer_); 
    Print(" after certification.\n", 23);

     
    PrintClerkIdentifierString(clerk);
    Print(" has provided ", 14);
    PrintCustomerIdentifierString(clerk->current_customer_); 
    Print(" their completed passport.\n", 28);

     
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



  clerk->running_ = 1;
  while (clerk->running_) {
    GetNextCustomer(clerk);
    if (clerk->state_ == kBusy || clerk->state_ == kAvailable) {
       
      Wait(clerk->wakeup_lock_cv_ , clerk->wakeup_lock_);
       
      PrintClerkIdentifierString(clerk);
      Print(" has received SSN ", 18);
      PrintNum(clerk->customer_ssn_);
      Print(" from ", 6);
      PrintCustomerIdentifierString(clerk->current_customer_);
      Print("\n", 1);
       
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
       
      for (i = 0; i < Rand() % 80 + 20; ++i) {
        Yield();
      }
       
    } else if (clerk->state_ == kOnBreak) {
      Acquire(clerk->wakeup_lock_);
       
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
    for (i = 0; i < 4 ; ++i) {
      for (j = 0; j < num_clerks_[i]; ++j) {
        m = CollectMoney(&(clerks_[i][j]));
        manager->money_[clerks_[i][j].type_] += m;
      }
    }
    total = 0;
    for (i = 0; i < 4 ; ++i) {
      total += manager->money_[i];
      Print("Manager has counted a total of $", 32);
      PrintNum(manager->money_[i]);
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

    for (i = 0; i < 4 ; ++i) {
      Acquire(line_locks_[i]);
    }
    for (i = 0; i < 4 ; ++i) {
      if (GetNumCustomersForClerkType((ClerkType)(i))

          > 3  || GetMonitor(num_senators_, 0) > 0) {

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
    for (i = 0; i < 4 ; ++i) {
      Release(line_locks_[i]);
    }
    Release(manager_.wakeup_condition_lock_);
  }
}

void WakeClerksForSenator() {
  int i;
  Clerk* clerk;

  for (i = 0; i < 4 ; ++i) {
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

 


void SetupPassportOffice() {
  int i;



  breaking_clerks_lock_ = CreateLock("bcl", 3);
  senator_lock_ = CreateLock("sl", 2);
  senator_condition_ = CreateCondition("slcv", 4);
  customer_count_lock_ = CreateLock("ccl", 3);
  customers_served_lock_ = CreateLock("csl", 3);
  customers_served_cv_ = CreateCondition("cslcv", 5);
  num_customers_waiting_lock_ = CreateLock("ncwl", 4);
  num_senators_lock_ = CreateLock("nsl", 3);
  outside_line_lock_ = CreateLock("oll", 3);
  outside_line_cv_ = CreateCondition("ollcv", 5);


  customers_size_ = CreateMonitor("cs", 2, 1);
  SetMonitor(customers_size_, 0, 15 );
  num_customers_being_served_ = CreateMonitor("ncs", 3, 1);
  num_customers_waiting_ = CreateMonitor("ncw", 3, 1);
  num_senators_ = CreateMonitor("ns", 2, 1);
  customer_index_ = CreateMonitor("ci", 2, 1);


  for (i = 0; i < 4 ; ++i) {
    nameLen = Sprintf(nameBuf, "ll%d", 4, i);
    line_locks_[i] = CreateLock(nameBuf, nameLen);

    nameLen = Sprintf(nameBuf, "lcmv%d", 6, i);
    line_counts_[i] = CreateMonitor(nameBuf, nameLen, num_clerks_[i]);
    nameLen = Sprintf(nameBuf, "blcmv%d", 6, i);
    bribe_line_counts_[i] = CreateMonitor(nameBuf, nameLen, num_clerks_[i]);
    nameLen = Sprintf(nameBuf, "cimv%d", 6, i);
    clerk_index_[i] = CreateMonitor(nameBuf, nameLen, num_clerks_[i]);

  }

  for (i = 0; i < 3 ; ++i) {
    CreateClerk(clerks_[kApplication] + i, i, kApplication);

  }

	for (i = 0; i < 3 ; ++i) {
    CreateClerk(clerks_[kPicture] + i, i, kPicture);

  }

  for (i = 0; i < 3 ; ++i) {
    CreateClerk(clerks_[kPassport] + i, i, kPassport);

  }

  for (i = 0; i < 3 ; ++i) {
    CreateClerk(clerks_[kCashier] + i, i, kCashier);

  }
  manager_ = CreateManager();
}



void WaitOnFinish() {
  int i, j, done;
     
  while (customers_size_ > 0) {
    for (i = 0; i < 400; ++i) { Yield(); }

    if (GetMonitor(num_senators_, 0) > 0)

    Acquire(num_customers_waiting_lock_);

    if (customers_size_ == GetMonitor(num_customers_waiting_, 0)) {

      Release(num_customers_waiting_lock_);
      done = 1;
      Acquire(breaking_clerks_lock_);
      for (i = 0; i < 4 ; ++i) {
        Acquire(line_locks_[i]);
        for (j = 0; j < num_clerks_[i]; ++j) {
          if (clerks_[i][j].state_ != kOnBreak ||
              GetNumCustomersForClerkType((ClerkType)(i)) > 
              3 ) {
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

   
  Print("Attempting to stop passport office\n", 35);
  while (customers_size_ > 0) {
    done = 1;
    Acquire(breaking_clerks_lock_);
    for (i = 0; i < 4 ; ++i) {
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
  for (i = 0; i < 4 ; ++i) {
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
  for (i = 0; i < 4 ; ++i) {
    DestroyLock(line_locks_[i]);
  }
}

typedef enum {
  CUSTOMER = 0,
  SENATOR,
  APPLICATION_CLERK,
  PASSPORT_CLERK,
  CASHIER_CLERK,
  PICTURE_CLERK,
  MANAGER,
  NUM_ENTITY_TYPES,
} EntityType;

void RunEntity(EntityType type, int entityId) {
  SetupPassportOffice();
  switch (type) {
    case CUSTOMER:
      CustomerRun(customers_ + entityId);
      break;
    case SENATOR:
      CustomerRun(customers_ + entityId);
      break;
    case APPLICATION_CLERK:
      ClerkRun(clerks_[kApplication] + entityId);
      break;
    case PASSPORT_CLERK:
      ClerkRun(clerks_[kPassport] + entityId);
      break;
    case CASHIER_CLERK:
      ClerkRun(clerks_[kCashier] + entityId);
      break;
    case PICTURE_CLERK:
      ClerkRun(clerks_[kPicture] + entityId);
      break;
    case MANAGER:
      ManagerRun(&manager_);
      break;
    default:
      break;
  }
}
