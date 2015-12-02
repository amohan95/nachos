#ifndef TEST_SIMULATION_C_
#define TEST_SIMULATION_C_

/* Gets the total number of customers waiting in line for a clerk type. */
int GetNumCustomersForClerkType(ClerkType type) {
  int total = 0;
  int i;
#ifdef NETWORK
  int blc, lc;
#endif
  for (i = 0; i < num_clerks_[type]; ++i) {
#ifdef NETWORK
    lc = GetMonitor(line_counts_[type], i);
    if (lc == -1) {
      Print("Error getting line count\n", 25);
      break;
    }
    total += lc;
    blc =  GetMonitor(bribe_line_counts_[type], i);
    if (blc == -1) {
      Print("Error getting bribe line count\n", 31);
      break;
    }
    total += blc;
#else
    total += line_counts_[type][i];
    total += bribe_line_counts_[type][i];
#endif
  }
  return total;
}

/* ######## Customer Functionality ######## */
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

#ifndef NETWORK
/* Adds a new customer to the passport office by creating a new thread and
  forking it. Additionally, this will add the customer to the customers_ set.*/
void AddNewCustomer(int index) {
  Acquire(customer_count_lock_);
  CreateCustomer(customers_ + index, kCustomer, 0);
  Fork(RunCustomer);
  Release(customer_count_lock_);
}

/* Adds a new senator to the passport office by creating a new thread and
  forking it. */
void AddNewSenator(int index) {
  Acquire(customer_count_lock_);
  CreateCustomer(customers_ + index, kSenator, 0);
  Fork(RunSenator);
  Release(customer_count_lock_);
/*  Thread* thread = new Thread("senator thread");
  thread->Fork(thread_runners::RunSenator, reinterpret_cast<int>(senator));
*/}
#endif

void DestroyCustomer(Customer* customer) {
}

void PrintCustomerIdentifierString(int customer_ssn) {
  if (customers_[customer_ssn].type_ == kCustomer) {
    Print("Customer [", 10);
  } else {
    Print("Senator [", 9);
  }
  PrintNum(customer_ssn);
  Print("]", 1);
}

void DoClerkWork(Customer* customer, Clerk* clerk) {
  Acquire(clerk->wakeup_lock_);
#ifdef NETWORK
  SetMonitor(clerk_customer_ssn_[clerk->type_], clerk->identifier_, customer->ssn_);
#endif
  clerk->customer_ssn_ = customer->ssn_;
  clerk->current_customer_ = customer;
  PrintCustomerIdentifierString(customer->ssn_);
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
#ifdef NETWORK
      SetMonitor(clerk_customer_input_[clerk->type_], clerk->identifier_, clerk->customer_input_);
#endif
      Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      break;
    case kCashier:
#ifdef NETWORK
      SetMonitor(clerk_customer_money_[clerk->type_], clerk->identifier_, PASSPORT_FEE);
#endif
      clerk->customer_money_ = PASSPORT_FEE;
      Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
#ifdef NETWORK
      SetMonitor(clerk_customer_input_[clerk->type_], clerk->identifier_, 1);
#endif
      clerk->customer_input_ = 1;
      Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      break;
    case kPassport:
#ifdef NETWORK
      SetMonitor(clerk_customer_input_[clerk->type_], clerk->identifier_, 1);
#endif
      clerk->customer_input_ = 1;
      Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      break;
    case NUM_CLERK_TYPES:
      break;
  }
}

void PrintLineJoin(Customer* customer, Clerk* clerk, int bribed) {
  PrintCustomerIdentifierString(customer->ssn_);
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
  int num_senators;

#ifndef NETWORK
  Release(customer_count_lock_);
#endif

  senator->running_ = 1;
  /* Increment the number of senators in the office so that others know 
     that a senator is there. */
  Acquire(num_senators_lock_);
#ifdef NETWORK
  SetMonitor(num_senators_, 0, GetMonitor(num_senators, 0) + 1);
#else
  ++num_senators_;
#endif
  Release(num_senators_lock_);

  Print("Starting Senator\n", 17);
  Acquire(senator_lock_);

  /* Wake up customers that are currently in line in the passport office so that
    they can join the outside line. */
#ifdef NETWORK
  while (GetMonitor(num_customers_being_served_, 0) > 0) {
#else
  while (num_customers_being_served_ > 0) {
#endif
    WakeWaitingCustomers();
    Yield();
  }

  /* Reset the line counts to 0 since there are no customers in the office
    at this point. */
  for (i = 0; i < NUM_CLERK_TYPES; ++i) {
    Acquire(line_locks_[i]);
    for (j = 0; j < num_clerks_[i]; ++j) {
#ifdef NETWORK
      SetMonitor(line_counts_[i], j, 0);
      SetMonitor(bribe_line_counts_[i], j, 0);
#else
      line_counts_[i][j] = 0;
      bribe_line_counts_[i][j] = 0;
#endif
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
#ifdef NETWORK
    SetMonitor(line_counts_[next_clerk], 0, GetMonitor(line_counts_[next_clerk], 0) + 1);
#else
    ++line_counts_[next_clerk][0];
#endif
    Release(line_locks_[next_clerk]);

    Signal(manager_.wakeup_condition_, manager_.wakeup_condition_lock_);

    PrintLineJoin(senator, clerk, 0);
    JoinLine(clerk, 0);

    DoClerkWork(senator, clerk);

    Acquire(line_locks_[next_clerk]);
#ifdef NETWORK
    SetMonitor(line_counts_[next_clerk], 0, GetMonitor(line_counts_[next_clerk], 0) - 1);
#else
    --line_counts_[next_clerk][0];
#endif
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

  Acquire(num_senators_lock_);
#ifdef NETWORK
  num_senators = GetMonitor(num_senators_, 0) - 1;
  SetMonitor(num_senators_, 0, num_senators);
#else
  num_senators = --num_senators_;
#endif
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

#ifndef NETWORK
  Release(customer_count_lock_);
#endif

  customer->running_ = 1;
  Print("Starting Customer\n", 18);
  while (customer->running_ &&
#ifdef NETWORK
      (!GetMonitor(customer_passport_verified_, customer->ssn_) ||
       !GetMonitor(customer_picture_taken_, customer->ssn_) ||
       !GetMonitor(customer_completed_application_, customer->ssn_) ||
       !GetMonitor(customer_certified_, customer->ssn_))) {
#else
      (!customer->passport_verified_ || !customer->picture_taken_ ||
      !customer->completed_application_ || !customer->certified_)) {
#endif
		Print("Customer Loop\n", 14);
    Acquire(num_senators_lock_);
#ifdef NETWORK
    if (GetMonitor(num_senators_, 0) > 0) {
#else
    if (num_senators_ > 0) {
#endif
      Release(num_senators_lock_);
      Acquire(outside_line_lock_);
      PrintCustomerIdentifierString(customer->ssn_);
      Print(" is going outside the Passport Office because there is a Senator present.\n", 74);
      Wait(outside_line_cv_, outside_line_lock_);
      Release(outside_line_lock_);
      continue;
    } else {
      Release(num_senators_lock_);
    }

    Acquire(customers_served_lock_);
#ifdef NETWORK
    SetMonitor(num_customers_being_served_, 0, GetMonitor(num_customers_being_served_, 0) + 1);
#else
    ++num_customers_being_served_;
#endif
    Release(customers_served_lock_);

    customer->bribed_ = 0;
#ifdef NETWORK
    if (!GetMonitor(customer_completed_application_, customer->ssn_) && !GetMonitor(customer_picture_taken_, customer->ssn_)) {
#else
    if (!customer->completed_application_ && !customer->picture_taken_ && NUM_APPLICATION_CLERKS > 0 && NUM_PICTURE_CLERKS > 0) {
#endif
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
#ifdef NETWORK
          GetMonitor(line_counts_[next_clerk], i) <
          GetMonitor(line_counts_[next_clerk], shortest)) {
#else
          line_counts_[next_clerk][i] < line_counts_[next_clerk][shortest]) {
#endif
        shortest = i;
      }
    }
#ifdef NETWORK
    if (GetMonitor(customer_money_, customer->ssn_) >= CLERK_BRIBE_AMOUNT + PASSPORT_FEE) {
#else
    if (customer->money_ >= CLERK_BRIBE_AMOUNT + PASSPORT_FEE) {
#endif
      bribe_shortest = -1;
      for (i = 0; i < num_clerks_[next_clerk]; ++i) {
        if (bribe_shortest == -1 ||
#ifdef NETWORK
            GetMonitor(bribe_line_counts_[next_clerk], i)
            < GetMonitor(bribe_line_counts_[next_clerk], bribe_shortest)) {
#else
          bribe_line_counts_[next_clerk][i]
            < bribe_line_counts_[next_clerk][bribe_shortest]) {
#endif
          bribe_shortest = i;
        }
      }
      if (shortest == -1) {
        customer->running_ = 0;
        Release(line_locks_[next_clerk]);
        continue;
      }
#ifdef NETWORK
      if (GetMonitor(bribe_line_counts_[next_clerk], bribe_shortest)
          < GetMonitor(line_counts_[next_clerk], shortest)) {
#else
      if (bribe_line_counts_[next_clerk][bribe_shortest]
          < line_counts_[next_clerk][shortest]) {
#endif
        clerk = clerks_[next_clerk] + bribe_shortest;
#ifdef NETWORK
        SetMonitor(customer_bribed_, customer->ssn_, 1);
#endif
        customer->bribed_ = 1;
      } else {
        clerk = clerks_[next_clerk] + shortest;
      }
    } else {
      if (shortest == -1) {
        Release(line_locks_[next_clerk]);
        customer->running_ = 0;
        continue;
      }
      clerk = clerks_[next_clerk] + shortest;
    }
    Release(line_locks_[next_clerk]);

    if (GetNumCustomersForClerkType(next_clerk) >
        CLERK_WAKEUP_THRESHOLD) {
      Signal(manager_.wakeup_condition_, manager_.wakeup_condition_lock_);
    }

    PrintLineJoin(customer, clerk, customer->bribed_);

    Acquire(num_customers_waiting_lock_);
#ifdef NETWORK
    SetMonitor(num_customers_waiting_, 0, GetMonitor(num_customers_waiting_, 0) + 1);
#else
    ++num_customers_waiting_;
#endif
    Release(num_customers_waiting_lock_);

    JoinLine(clerk, customer->bribed_);

    Acquire(num_customers_waiting_lock_);
#ifdef NETWORK
    SetMonitor(num_customers_waiting_, 0, GetMonitor(num_customers_waiting_, 0) - 1);
#else
    --num_customers_waiting_;
#endif
    Release(num_customers_waiting_lock_);

    Acquire(customers_served_lock_);
#ifdef NETWORK
    SetMonitor(num_customers_being_served_, 0, GetMonitor(num_customers_being_served_, 0) - 1);
#else
    --num_customers_being_served_;
#endif
    Release(customers_served_lock_);
    Acquire(num_senators_lock_);

#ifdef NETWORK
    if (GetMonitor(num_senators_, 0) > 0) {
#else
    if (num_senators_ > 0) {
#endif
      Release(num_senators_lock_);
      Acquire(customers_served_lock_);
#ifdef NETWORK
      if (GetMonitor(num_customers_being_served_, 0)  == 0) {
#else
      if (num_customers_being_served_ == 0) {
#endif
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
#ifdef NETWORK
      SetMonitor(clerk_customer_money_[clerk->type_], clerk->identifier_, CLERK_BRIBE_AMOUNT);
      SetMonitor(customer_money_, customer->ssn_, GetMonitor(customer_money_, customer->ssn_) - CLERK_BRIBE_AMOUNT);
#endif
      clerk->customer_money_ = CLERK_BRIBE_AMOUNT;
      customer->money_ -= CLERK_BRIBE_AMOUNT;
      Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
    }
    clerk->current_customer_ = NULL;
    Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
    Release(clerk->wakeup_lock_);
  }
#ifdef NETWORK
  if (GetMonitor(customer_passport_verified_, customer->ssn_)) {
#else
  if (customer->passport_verified_) {
#endif
    PrintCustomerIdentifierString(customer->ssn_);
    Print(" is leaving the Passport Office.\n", 33);
  } else {
    PrintCustomerIdentifierString(customer->ssn_);
    Print(" terminated early because it is impossible to get a passport.\n", 62);
  }
  Acquire(customers_served_lock_);
#ifdef NETWORK
  num_customers_being_served = GetMonitor(num_customers_being_served_, 0) - 1;
  SetMonitor(num_customers_being_served_, 0, num_customers_being_served);
#else
  num_customers_being_served = --num_customers_being_served_;
#endif
  if (num_customers_being_served == 0) {
    Broadcast(customers_served_cv_, customers_served_lock_);
  }
  Release(customers_served_lock_);

  Acquire(customer_count_lock_);
#ifdef NETWORK
  SetMonitor(customers_size_, 0, GetMonitor(customers_size_, 0) - 1);
#else
  --customers_size_;
#endif
  Release(customer_count_lock_);
}

/* ######## Clerk Functionality ######## */
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
  char nameBuf[128];
  int nameLen;

  clerk->customer_money_ = 0;
  clerk->customer_input_ = 0;
  clerk->state_ = kAvailable;
  clerk->collected_money_ = 0;
  clerk->identifier_ = identifier;
  clerk->running_ = 0;
  nameLen = Sprintf(nameBuf, "cll%d", 5, identifier * 10 + type);
  clerk->lines_lock_ = CreateLock(nameBuf, nameLen);
  nameLen = Sprintf(nameBuf, "cblcv%d", 7, identifier * 10 + type);
  clerk->bribe_line_lock_cv_ = CreateCondition(nameBuf, nameLen);
  nameLen = Sprintf(nameBuf, "cbll%d", 6, identifier * 10 + type);
  clerk->bribe_line_lock_ = CreateLock(nameBuf, nameLen);
  nameLen = Sprintf(nameBuf, "crlcv%d", 7, identifier * 10 + type);
  clerk->regular_line_lock_cv_ = CreateCondition(nameBuf, nameLen);
  nameLen = Sprintf(nameBuf, "crll%d", 6, identifier * 10 + type);
  clerk->regular_line_lock_ = CreateLock(nameBuf, nameLen);
  nameLen = Sprintf(nameBuf, "cwlcv%d", 7, identifier * 10 + type);
  clerk->wakeup_lock_cv_ = CreateCondition(nameBuf, nameLen);
  nameLen = Sprintf(nameBuf, "cwl%d", 5, identifier * 10 + type);
  clerk->wakeup_lock_ = CreateLock(nameBuf, nameLen);
  nameLen = Sprintf(nameBuf, "cml%d", 5, identifier * 10 + type);
  clerk->money_lock_ = CreateLock(nameBuf, nameLen);
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
#ifdef NETWORK
    SetMonitor(bribe_line_counts_[clerk->type_], clerk->identifier_,
        GetMonitor(bribe_line_counts_[clerk->type_], clerk->identifier_) + 1);
#else
    ++bribe_line_counts_[clerk->type_][clerk->identifier_];
#endif
    if (GetNumCustomersForClerkType(clerk->type_) > 
        CLERK_WAKEUP_THRESHOLD) {
      Signal(manager_.wakeup_condition_, manager_.wakeup_condition_lock_);
    }
    Wait(clerk->bribe_line_lock_cv_, clerk->bribe_line_lock_);
    Release(clerk->bribe_line_lock_);
  } else {
    Acquire(clerk->regular_line_lock_);
#ifdef NETWORK
    SetMonitor(line_counts_[clerk->type_], clerk->identifier_,
        GetMonitor(line_counts_[clerk->type_], clerk->identifier_) + 1);
#else
    ++line_counts_[clerk->type_][clerk->identifier_];
#endif
    if (GetNumCustomersForClerkType(clerk->type_) > 
        CLERK_WAKEUP_THRESHOLD) {
      Signal(manager_.wakeup_condition_, manager_.wakeup_condition_lock_);
    }
    Wait(clerk->regular_line_lock_cv_ , clerk->regular_line_lock_);
    Release(clerk->regular_line_lock_);
  }
}

int GetNumCustomersInLine(Clerk* clerk) {
#ifdef NETWORK
  return GetMonitor(line_counts_[clerk->type_], clerk->identifier_) +
      GetMonitor(bribe_line_counts_[clerk->type_], clerk->identifier_);
#else
  return line_counts_[clerk->type_][clerk->identifier_] +
      bribe_line_counts_[clerk->type_][clerk->identifier_];
#endif
}

void GetNextCustomer(Clerk* clerk) {
  int bribe_line_count;
  int regular_line_count;

  Acquire(line_locks_[clerk->type_]);
  Acquire(clerk->bribe_line_lock_);
#ifdef NETWORK
  bribe_line_count = GetMonitor(bribe_line_counts_[clerk->type_], clerk->identifier_);
#else
  bribe_line_count = bribe_line_counts_[clerk->type_][clerk->identifier_];
#endif
  Release(clerk->bribe_line_lock_);

  Acquire(clerk->regular_line_lock_);
#ifdef NETWORK
  regular_line_count = GetMonitor(line_counts_[clerk->type_], clerk->identifier_);
#else
  regular_line_count = line_counts_[clerk->type_][clerk->identifier_];
#endif
  Release(clerk->regular_line_lock_);

  if (bribe_line_count > 0) {
    PrintClerkIdentifierString(clerk);
    Print(" has signalled a Customer to come to their counter.\n", 52);

    Acquire(clerk->bribe_line_lock_);
    Acquire(clerk->wakeup_lock_);
    Signal(clerk->bribe_line_lock_cv_, clerk->bribe_line_lock_);
    Release(clerk->bribe_line_lock_);
    clerk->state_ = kBusy;
#ifdef NETWORK
    SetMonitor(bribe_line_counts_[clerk->type_], clerk->identifier_,
        GetMonitor(bribe_line_counts_[clerk->type_], clerk->identifier_) - 1);
#else
    bribe_line_counts_[clerk->type_][clerk->identifier_]--;
#endif
  } else if (regular_line_count > 0) {
    PrintClerkIdentifierString(clerk);
    Print(" has signalled a Customer to come to their counter.\n", 52);

    Acquire(clerk->regular_line_lock_);
    Acquire(clerk->wakeup_lock_);
    Signal(clerk->regular_line_lock_cv_, clerk->regular_line_lock_);
    Release(clerk->regular_line_lock_);
    clerk->state_ = kBusy;
#ifdef NETWORK
    SetMonitor(line_counts_[clerk->type_], clerk->identifier_,
        GetMonitor(line_counts_[clerk->type_], clerk->identifier_) - 1);
#else
    line_counts_[clerk->type_][clerk->identifier_]--;
#endif
  } else {
    clerk->state_ = kOnBreak;
  }
  Release(line_locks_[clerk->type_]);
}

void ApplicationClerkWork(Clerk* clerk) {
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  /* Wait for customer to put passport on counter. */
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
#ifdef NETWORK
  SetMonitor(customer_completed_application_,
      clerk->customer_ssn_, 1);
#else
  customers_[clerk->customer_ssn_].completed_application_ = 1;
#endif
  PrintClerkIdentifierString(clerk);
  Print(" has recorded a completed application for ", 42);
  PrintCustomerIdentifierString(clerk->customer_ssn_);
  Print("\n", 1);
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
}

void PictureClerkWork(Clerk* clerk) {
  int picture_accepted;
  /* Take Customer's picture and wait to hear if they like it. */
  Print(" [", 2);
  PrintNum(clerk->identifier_);
  Print(" has taken a picture of ", 24);
  PrintCustomerIdentifierString(clerk->customer_ssn_);
  Print("\n", 1);

  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
#ifdef NETWORK
  clerk->customer_input_ = GetMonitor(clerk_customer_input_[clerk->type_], clerk->identifier_);
#endif
  picture_accepted = clerk->customer_input_;

  /* If they don't like their picture don't set their picture to taken.  They go back in line. */
  if (!picture_accepted) {
    PrintClerkIdentifierString(clerk);
    Print(" has been told that ", 20);
    PrintCustomerIdentifierString(clerk->customer_ssn_);
    Print(" does not like their picture.\n", 30);
  } else {
    /* Set picture taken. */
#ifdef NETWORK
    SetMonitor(customer_picture_taken_, clerk->customer_ssn_, 1);
#else
    customers_[clerk->customer_ssn_].picture_taken_ = 1;
#endif
    PrintClerkIdentifierString(clerk); 
    Print(" has been told that ", 20);
    PrintCustomerIdentifierString(clerk->customer_ssn_);
    Print(" does like their picture.\n", 26);
  }
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
}

void PassportClerkWork(Clerk* clerk) {
  int picture_taken_and_passport_verified;
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  /* Wait for customer to show whether or not they got their picture taken and passport verified. */
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
#ifdef NETWORK
  clerk->customer_input_ = GetMonitor(clerk_customer_input_[clerk->type_], clerk->identifier_);
#endif
  picture_taken_and_passport_verified = clerk->customer_input_;

  /* Check to make sure their picture has been taken and passport verified. */
  if (!picture_taken_and_passport_verified) {
    PrintClerkIdentifierString(clerk);
    Print(" has determined that ", 21);
    PrintCustomerIdentifierString(clerk->customer_ssn_);
    Print(" does not have both their application and picture completed\n", 60);
  } else {
    PrintClerkIdentifierString(clerk);
    Print(" has determined that ", 21);
    PrintCustomerIdentifierString(clerk->customer_ssn_);
    Print(" does have both their application and picture completed\n", 56);
#ifdef NETWORK
    SetMonitor(customer_certified_, clerk->customer_ssn_, 1);
#else
    customers_[clerk->customer_ssn_].certified_ = 1;
#endif
    PrintClerkIdentifierString(clerk);
    Print(" has recorded ", 15);
    PrintCustomerIdentifierString(clerk->customer_ssn_);
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
  #ifdef NETWORK
  clerk->customer_money_ = GetMonitor(clerk_customer_money_[clerk->type_], clerk->identifier_);
  SetMonitor(clerk_customer_money_[clerk->type_], clerk->identifier_, 0);
  SetMonitor(clerk_collected_money_[clerk->type_], clerk->identifier_, GetMonitor(clerk_collected_money_[clerk->type_], clerk->identifier_) + clerk->customer_money_);
#endif
  clerk->collected_money_ += clerk->customer_money_;
  clerk->customer_money_ = 0;
  Release(clerk->money_lock_);

  /* Wait for the customer to show you that they are certified. */
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
#ifdef NETWORK
  clerk->customer_input_ = GetMonitor(clerk_customer_input_[clerk->type_], clerk->identifier_);
#endif
  certified = clerk->customer_input_;

  /* Check to make sure they have been certified. */
  if (!certified) {
    PrintClerkIdentifierString(clerk);
    Print(" has received the $100 from ", 28);
    PrintCustomerIdentifierString(clerk->customer_ssn_);
    Print(" before certification. They are to go to the back of my line.\n", 62);

    /* Give money back. */
    Acquire(clerk->money_lock_);
#ifdef NETWORK
    SetMonitor(clerk_collected_money_[clerk->type_], clerk->identifier_, GetMonitor(clerk_collected_money_[clerk->type_], clerk->identifier_) - 100);
#endif
    clerk->collected_money_ -= 100;
    Release(clerk->money_lock_);
#ifdef NETWORK
    SetMonitor(customer_money_, clerk->customer_ssn_, GetMonitor(customer_money_, clerk->customer_ssn_) + 100);
#else
    customers_[clerk->customer_ssn_].money_ += 100;
#endif
  } else {
    PrintClerkIdentifierString(clerk);
    Print(" has received the $100 from ", 28);
    PrintCustomerIdentifierString(clerk->customer_ssn_);
    Print(" after certification.\n", 23);

    /* Give customer passport. */
    PrintClerkIdentifierString(clerk);
    Print(" has provided ", 14);
    PrintCustomerIdentifierString(clerk->customer_ssn_);
    Print(" their completed passport.\n", 28);

    /* Record passport was given to customer. */
    PrintClerkIdentifierString(clerk);
    Print(" has recorded that ", 19);
    PrintCustomerIdentifierString(clerk->customer_ssn_);
    Print(" has been given their completed passport.\n", 43);

#ifdef NETWORK
    SetMonitor(customer_passport_verified_, clerk->customer_ssn_, 1);
#else
    customers_[clerk->customer_ssn_].passport_verified_ = 1;
#endif
  }
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
}

void ClerkRun(Clerk* clerk) {
  int i;
  int bribe;

#ifndef NETWORK
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
#endif

  clerk->running_ = 1;
  while (clerk->running_) {
    GetNextCustomer(clerk);
    if (clerk->state_ == kBusy || clerk->state_ == kAvailable) {
      /* Wait for customer to come to counter and give SSN. */
      Wait(clerk->wakeup_lock_cv_ , clerk->wakeup_lock_);
      /* Take Customer's SSN and verify passport. */
#ifdef NETWORK
      clerk->customer_ssn_ = GetMonitor(clerk_customer_ssn_[clerk->type_], clerk->identifier_);
#endif
      PrintClerkIdentifierString(clerk);
      Print(" has received SSN ", 18);
      PrintNum(clerk->customer_ssn_);
      Print(" from ", 6);
      PrintCustomerIdentifierString(clerk->customer_ssn_);
      Print("\n", 1);
      /* Do work specific to the type of clerk. */
      switch(clerk->type_) {
        case kApplication:
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
#ifdef NETWORK
      customers_[clerk->customer_ssn_].bribed_ =
        GetMonitor(customer_bribed_, clerk->customer_ssn_);
#endif
      if (customers_[clerk->customer_ssn_].bribed_) {
        Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
#ifdef NETWORK
        clerk->customer_money_  = GetMonitor(clerk_customer_money_[clerk->type_], clerk->identifier_);
        SetMonitor(clerk_customer_money_[clerk->type_], clerk->identifier_, 0);
#endif
        bribe = clerk->customer_money_;
        clerk->customer_money_ = 0;
        Acquire(clerk->money_lock_);
#ifdef NETWORK
        SetMonitor(clerk_collected_money_[clerk->type_], clerk->identifier_, GetMonitor(clerk_collected_money_[clerk->type_], clerk->identifier_) + bribe);
#endif
        clerk->collected_money_ += bribe;
        Release(clerk->money_lock_);
        PrintClerkIdentifierString(clerk);
        Print(" has received $", 15);
        PrintNum(bribe);
        Print(" from ", 6);
        PrintCustomerIdentifierString(clerk->customer_ssn_);
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
        manager->money_[clerks_[i][j].type_] += m;
      }
    }
    total = 0;
    for (i = 0; i < NUM_CLERK_TYPES; ++i) {
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

    for (i = 0; i < NUM_CLERK_TYPES; ++i) {
      Acquire(line_locks_[i]);
    }
    for (i = 0; i < NUM_CLERK_TYPES; ++i) {
      if (GetNumCustomersForClerkType((ClerkType)(i))
#ifdef NETWORK
          > CLERK_WAKEUP_THRESHOLD || GetMonitor(num_senators_, 0) > 0) {
#else
        > CLERK_WAKEUP_THRESHOLD || num_senators_ > 0) {
#endif
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
void RunManagerMoneyReport() {
  ManagerPrintMoneyReport(&manager_);
  Exit(0);
}

#ifndef NETWORK
void RunManager() {
  ManagerRun(&manager_);
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
#endif

void SetupPassportOffice() {
  int i;
  int nameLen;
  char nameBuf[128];

	application_clerk_init_lock_ = CreateLock("acil", 4);
  picture_clerk_init_lock_ = CreateLock("picil", 5);
  passport_clerk_init_lock_ = CreateLock("pacil", 5);
  cashier_clerk_init_lock_ = CreateLock("ccil", 4);

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

#ifdef NETWORK
  customers_size_ = CreateMonitor("cs", 2, 1);
  SetMonitor(customers_size_, 0, NUM_CUSTOMERS + NUM_SENATORS);
  num_customers_being_served_ = CreateMonitor("ncs", 3, 1);
  num_customers_waiting_ = CreateMonitor("ncw", 3, 1);
  num_senators_ = CreateMonitor("ns", 2, 1);
  customer_index_ = CreateMonitor("ci", 2, 1);
  customer_money_ = CreateMonitor("cm", 2, NUM_CUSTOMERS + NUM_SENATORS);
  customer_bribed_ = CreateMonitor("cb", 2, NUM_CUSTOMERS + NUM_SENATORS);
  customer_picture_taken_ = CreateMonitor("cpt", 3, NUM_CUSTOMERS + NUM_SENATORS);
  customer_certified_ = CreateMonitor("cc", 2, NUM_CUSTOMERS + NUM_SENATORS);
  customer_completed_application_ = CreateMonitor("ccp", 3, NUM_CUSTOMERS + NUM_SENATORS);
  customer_passport_verified_ = CreateMonitor("cpv", 3, NUM_CUSTOMERS + NUM_SENATORS);
#else
  customers_size_ = 0;
  num_customers_being_served_ = 0;
  num_customers_waiting_= 0;
  num_senators_ = 0;
#endif

  for (i = 0; i < NUM_CLERK_TYPES; ++i) {
    nameLen = Sprintf(nameBuf, "ll%d", 4, i);
    line_locks_[i] = CreateLock(nameBuf, nameLen);
#ifdef NETWORK
    nameLen = Sprintf(nameBuf, "lcmv%d", 6, i);
    line_counts_[i] = CreateMonitor(nameBuf, nameLen, num_clerks_[i]);
    nameLen = Sprintf(nameBuf, "blcmv%d", 6, i);
    bribe_line_counts_[i] = CreateMonitor(nameBuf, nameLen, num_clerks_[i]);
    nameLen = Sprintf(nameBuf, "cimv%d", 6, i);
    clerk_index_[i] = CreateMonitor(nameBuf, nameLen, num_clerks_[i]);
    nameLen = Sprintf(nameBuf, "ccssnmv%d", 9, i);
    clerk_customer_ssn_[i] = CreateMonitor(nameBuf, nameLen, num_clerks_[i]);
    nameLen = Sprintf(nameBuf, "ccmmv%d", 7, i);
    clerk_customer_money_[i] = CreateMonitor(nameBuf, nameLen, num_clerks_[i]);
    nameLen = Sprintf(nameBuf, "ccommv%d", 8, i);
    clerk_collected_money_[i] = CreateMonitor(nameBuf, nameLen, num_clerks_[i]);
    nameLen = Sprintf(nameBuf, "ccimv%d", 7, i);
    clerk_customer_input_[i] = CreateMonitor(nameBuf, nameLen, num_clerks_[i]);
#endif
  }

#ifdef NETWORK
	for (i = 0; i < NUM_CUSTOMERS + NUM_SENATORS; ++i) {
		CreateCustomer(customers_ + i, kCustomer, 0);
	}
#endif

  for (i = 0; i < NUM_APPLICATION_CLERKS; ++i) {
    CreateClerk(clerks_[kApplication] + i, i, kApplication);
#ifndef NETWORK
    line_counts_[kApplication][i] = 0;
    bribe_line_counts_[kApplication][i] = 0;
#endif
  }

	for (i = 0; i < NUM_PICTURE_CLERKS; ++i) {
    CreateClerk(clerks_[kPicture] + i, i, kPicture);
#ifndef NETWORK
    line_counts_[kPicture][i] = 0;
    bribe_line_counts_[kPicture][i] = 0;
#endif
  }

  for (i = 0; i < NUM_PASSPORT_CLERKS; ++i) {
    CreateClerk(clerks_[kPassport] + i, i, kPassport);
#ifndef NETWORK
    line_counts_[kPassport][i] = 0;
    bribe_line_counts_[kPassport][i] = 0;
#endif
  }

  for (i = 0; i < NUM_CASHIER_CLERKS; ++i) {
    CreateClerk(clerks_[kCashier] + i, i, kCashier);
#ifndef NETWORK
    line_counts_[kCashier][i] = 0;
    bribe_line_counts_[kCashier][i] = 0;
#endif
  }
  manager_ = CreateManager();
}

#ifndef NETWORK
void StartPassportOffice() {
  int i, j;

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
#endif

void WaitOnFinish() {
  int i, j, done;
    /* WaitOnFinish for the Passport Office */
  while (customers_size_ > 0) {
    for (i = 0; i < 400; ++i) { Yield(); }
#ifdef NETWORK
    if (GetMonitor(num_senators_, 0) > 0)
#else
    if (num_senators_ > 0) continue;
#endif
    Acquire(num_customers_waiting_lock_);
#ifdef NETWORK
    if (customers_size_ == GetMonitor(num_customers_waiting_, 0)) {
#else
      if (customers_size_ == num_customers_waiting_) {
#endif
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

void RunEntity(EntityType type, int entityId) {
  switch (type) {
    case CUSTOMER:
      CustomerRun(customers_ + entityId);
      break;
    case SENATOR:
      SenatorRun(customers_ + entityId);
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
      Fork(RunManagerMoneyReport);
      ManagerRun(&manager_);
      break;
    default:
      break;
  }
}

#endif
