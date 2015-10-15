#include "customer.h"
#include "../userprog/syscall.h"

static const int INITIAL_MONEY_AMOUNTS[] = {100, 600, 1100, 1600};
static int CURRENT_UNUSED_SSN = 0;
static int SENATOR_UNUSED_SSN = 0;

Customer CreateCustomer(PassportOffice* passport_office,
    customer_types::Type type = customer_types::kCustomer) {
  Customer customer;
  customer.passport_office_ = passport_office;
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
  customer.join_line_lock_ = CreateLock("jll", 3);
  customer.join_line_lock_cv_ = CreateCondition("jllcv", 5);
  return customer;
}

Customer CreateCustomer(PassportOffice* passport_office, int money__) {
  Customer customer;
  customer.money_ = money__;
  customer.ssn_ = CURRENT_UNUSED_SSN++; 
  customer.passport_office_ = passport_office;
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

std::string CustomerIdentifierString(Customer* customer) const {
  std::stringstream ss;
  ss << "Customer [" << customer->ssn_ << ']';
  return ss.str();
}

void DoClerkWork(Customer* customer, Clerk* clerk) {
  Acquire(clerk->wakeup_lock_);
  clerk->customer_ssn_ = customer->ssn_;
  clerk->current_customer_ = customer;
  std::cout << CustomerIdentifierString(customer) << " has given SSN [" 
      << customer->ssn_ << "] to " << ClerkIdentifierString(clerk) << '.' 
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
  std::cout << CustomerIdentifierString(customer) << " has gotten in " 
      << (bribed ? "bribe" : "regular") << " line for " 
      << ClerkIdentifierString(clerk)  << "." << std::endl;
}

std::string SenatorIdentifierString(Customer* customer) const {
  std::stringstream ss;
  ss << "Senator [" << customer->ssn_ << ']';
  return ss.str();
}

void SenatorRun(Customer* customer) {
  customer->running_ = 1;
  // Increment the number of senators in the office so that others know
  // that a senator is there.
  Acquire(customer->passport_office_->num_senators_lock_);
  ++customer->passport_office_->num_senators_;
  Release(customer->passport_office_->num_senators_lock_);

  Acquire(customer->passport_office_->senator_lock_);

  // Wake up customers that are currently in line in the passport office so that

  // they can join the outside line.
  while (customer->passport_office_->num_customers_being_served_ > 0) {
    customer->passport_office_->manager_->WakeWaitingCustomers();
    currentThread->Yield();
  }
  // Wait until all customers have left the building.
  //if (passport_office_->num_customers_being_served_ > 0) {
  //  passport_office_->customers_served_cv_.Wait(
  //      &passport_office_->customers_served_lock_);
  //}

  // Reset the line counts to 0 since there are no customers in the office
  // at this point.
  for (unsigned i = 0; i < customer->passport_office_->line_counts_.size(); ++i) {
    Acquire(customer->passport_office_->line_locks_[i]);
    for (unsigned j = 0; j < customer->passport_office_->line_counts_[i].size(); ++j) {
      customer->passport_office_->line_counts_[i][j] = 0;
      customer->passport_office_->bribe_line_counts_[i][j] = 0;      
    }
    Release(customer->passport_office_->line_locks_[i]);
  }
  
  // Wait for all clerks to get off of their breaks, if necessary.
  for (int i = 0; i < 500; ++i) { currentThread->Yield(); }

  while (customer->running_ &&
        (!customer->passport_verified_ || !customer->picture_taken_ ||
         !customer->completed_application_ || !customer->certified_) {
    clerk_types::Type next_clerk;
    if (!customer->completed_application_ && !customer->picture_taken_ && 
        customer->passport_office_->clerks_[clerk_types::kApplication].size() > 0 &&
        customer->passport_office_->clerks_[clerk_types::kPicture].size() > 0) {
      next_clerk = (clerk_types::Type)(Rand() % 2); // either kApplication (0) or kPicture (1)
    } else if (!customer->completed_application_ {
      next_clerk = clerk_types::kApplication;
    } else if (!customer->picture_taken_ {
      next_clerk = clerk_types::kPicture;
    } else if (!customer->certified_ {
      next_clerk = clerk_types::kPassport;
    } else {
      next_clerk = clerk_types::kCashier;
    }
    if (customer->passport_office_->clerks_[next_clerk].empty()) {
      break;
    }
    Clerk* clerk = customer->passport_office_->clerks_[next_clerk][0];
    Acquire(customer->passport_office_->line_locks_[next_clerk]);
    ++customer->passport_office_->line_counts_[next_clerk][0];
    Release(customer->passport_office_->line_locks_[next_clerk]);
    
    Signal(customer->passport_office_->manager_->wakeup_condition_,
        customer->passport_office_->manager_->wakeup_condition_lock_);

    PrintLineJoin(clerk, bribed_);
    JoinLine(clerk, bribed_);

    DoClerkWork(clerk);

    Acquire(customer->passport_office_->line_locks_[next_clerk]);
    --customer->passport_office_->line_counts_[next_clerk][0];
    Release(customer->passport_office_->line_locks_[next_clerk]);

    clerk->current_customer_ = NULL;
    Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
    Release(clerk->wakeup_lock_);
  }

  if (customer->passport_verified_ {
    std::cout << SenatorIdentifierString() << " is leaving the Passport Office." << std::endl;
  } else {
    std::cout << SenatorIdentifierString() << " terminated early because it is impossible to get a passport." << std::endl;
  }

  // Leaving the office - if there are no more senators left waiting, then
  // tell all the customers outside to come back in.
  --customer->passport_office_->num_senators_;
  if (customer->passport_office_->num_senators_ == 0) {
    Broadcast(passport_office_->outside_line_cv_,
        customer->passport_office_->outside_line_lock_);
  }
  Release(customer->passport_office_->senator_lock_);
}

void CustomerRun(Customer* customer) {
  customer->running_ = 1;
  while (customer->running_ &&
      (!customer->passport_verified_ || !customer->picture_taken_ ||
      !customer->completed_application_ || !customer->certified_) {
    Acquire(customer->passport_office_->num_senators_lock_);
    if (customer->passport_office_->num_senators_ > 0) {
      Release(customer->passport_office_->num_senators_lock_);
      Acquire(customer->passport_office_->outside_line_lock_);
      std::cout << customer->CustomerIdentifierString() << " is going outside "
          << "the Passport Office because there is a Senator present."
          << std::endl;
      Wait(customer->passport_office_->outside_line_cv_,
          customer->passport_office_->outside_line_lock_);
      Release(customer->passport_office_->outside_line_lock_);
      continue;
    } else {
      Release(customer->passport_office_->num_senators_lock_);
    }

    Acquire(customer->passport_office_->customers_served_lock_);
    ++customer->passport_office_->num_customers_being_served_;
    Release(customer->passport_office_->customers_served_lock_);

    customer->bribed_ = 0;
    clerk_types::Type next_clerk;
    if (!customer->completed_application_ && !customer->picture_taken_ && customer->passport_office_->clerks_[clerk_types::kApplication].size() > 0 && customer->passport_office_->clerks_[clerk_types::kPicture].size() > 0) {
      next_clerk = (clerk_types::Type>)(Rand() % 2); // either kApplication (0) or kPicture (1)
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
    Acquire(customer->passport_office_->line_locks_[next_clerk]);
    int32_t shortest = -1;
    for (int i = 0; i < customer->passport_office_->line_counts_[next_clerk].size(); ++i) {
      if (shortest == -1 ||
          customer->passport_office_->line_counts_[next_clerk][i]
          < customer->passport_office_->line_counts_[next_clerk][shortest]) {
        shortest = i;
      }
    }
    if (customer->money_ >= CLERK_BRIBE_AMOUNT + PASSPORT_FEE) {
      int32_t bribe_shortest = -1;
      for (int i = 0; i < customer->passport_office_->bribe_line_counts_[next_clerk].size(); ++i) {
        if (bribe_shortest == -1 ||
            customer->passport_office_->bribe_line_counts_[next_clerk][i]
            < customer->passport_office_->bribe_line_counts_[next_clerk][bribe_shortest]) {
          bribe_shortest = i;
        }
      }
      if (shortest == -1) {
        customer->running_ = 0;
        Release(customer->passport_office_->line_locks_[next_clerk]);
        continue;
      }
      if (customer->passport_office_->bribe_line_counts_[next_clerk][bribe_shortest]
          < customer->passport_office_->line_counts_[next_clerk][shortest]) {
        clerk = customer->passport_office_->clerks_[next_clerk][bribe_shortest];
        customer->bribed_ = 1;
//        clerk->lines_lock_.Acquire();
//        clerk->lines_lock_.Release();
      } else {
        clerk = customer->passport_office_->clerks_[next_clerk][shortest];
//        clerk->lines_lock_.Acquire();
//        clerk->lines_lock_.Release();
      }
    } else {
      if (shortest == -1) {
        Release(customer->passport_office_->line_locks_[next_clerk]);
        customer->running_ = 0;
        continue;
      }
      clerk = customer->passport_office_->clerks_[next_clerk][shortest];
//      clerk->lines_lock_.Acquire();
//      clerk->lines_lock_.Release();
    }
    Release(customer->passport_office_->line_locks_[next_clerk]);

    if (customer->passport_office_->GetNumCustomersForClerkType(next_clerk) > 
        CLERK_WAKEUP_THRESHOLD) {
      Signal(customer->passport_office_->manager_->wakeup_condition_,
          customer->passport_office_->manager_->wakeup_condition_lock_);
    }

		PrintLineJoin(clerk, customer->bribed_);
//    join_line_lock_.Acquire();
//    join_line_lock_cv_.Signal(&join_line_lock_);
//    join_line_lock_.Release();
    Acquire(customer->passport_office_->num_customers_waiting_lock_);
    ++customer->passport_office_->num_customers_waiting_;
    Release(customer->passport_office_->num_customers_waiting_lock_);

		JoinLine(clerk, customer->bribed_);

    Acquire(customer->passport_office_->num_customers_waiting_lock_);
    --customer->passport_office_->num_customers_waiting_;
    Release(customer->passport_office_->num_customers_waiting_lock_);

    Acquire(customer->passport_office_->customers_served_lock_);
    --customer->passport_office_->num_customers_being_served_;
    Release(customer->passport_office_->customers_served_lock_);
    Acquire(customer->passport_office_->num_senators_lock_);
    
    if (customer->passport_office_->num_senators_ > 0) {
      Release(customer->passport_office_->num_senators_lock_);
      Acquire(customer->passport_office_->customers_served_lock_);
      if (customer->passport_office_->num_customers_being_served_ == 0) {
        Broadcast(customer->passport_office_->customers_served_cv_,
            customer->passport_office_->customers_served_lock_);
      }
      Release(customer->passport_office_->customers_served_lock_);
      continue;
    }
    Release(customer->passport_office_->num_senators_lock_);

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
    std::cout << customer->CustomerIdentifierString() << " is leaving the Passport Office." << std::endl;
  } else {
    std::cout << customer->CustomerIdentifierString() << " terminated early because it is impossible to get a passport." << std::endl;
  }
  Acquire(customer->passport_office_->customers_served_lock_);
  --customer->passport_office_->num_customers_being_served_;
  if (customer->passport_office_->num_customers_being_served_ == 0) {
    Broadcast(customer->passport_office_->customers_served_cv_,
        customer->passport_office_->customers_served_lock_);
  }
  Release(customer->passport_office_->customers_served_lock_);
  
  Acquire(customer->passport_office_->customer_count_lock_);
  customer->passport_office_->customers_.erase(this);
  Release(customer->passport_office_->customer_count_lock_);
}
