#include "clerks.h"
#include "customer.h"
#include "../userprog/syscall.h"

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

Clerk CreateClerk(
    PassportOffice* passport_office, int identifier, clerk_types::Type type) {
  Clerk clerk;
  clerk.customer_money_ = 0;
  clerk.customer_input_ = 0;
  clerk.state_ = clerk_states::kAvailable;
  clerk.passport_office_ = passport_office;
  clerk.collected_money_ = 0;
  clerk.identifier_ = identifier;
  clerk.running_ = 0;
  clerk.lines_lock_ = CreateLock("Clerk Lines Lock", 16);
  clerk.bribe_line_lock_cv_ = CreateCondition("Clerk Bribe Line Condition", 27);
  clerk.bribe_line_lock_ = CreateLock("Clerk Bribe Line Lock", 21);
  clerk.regular_line_lock_cv_ = CreateCondition("Clerk Regular Line Condition", 27);
  clerk.regular_line_lock_ = CreateLock("Clerk Regular Line Lock", 23);
  clerk.wakeup_lock_cv_ = CreateCondition("Clerk Wakeup Condition", 22);
  clerk.wakeup_lock_ = CreateLock("Clerk Wakeup Lock", 17);
  clerk.money_lock_ = CreateLock("Clerk Money Lock", 16);
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
    ++clerk->passport_office_->clerk->bribe_line_counts_[clerk->type_][clerk->identifier_];
    if (clerk->passport_office_->GetNumCustomersForClerkType(clerk->type_) > 
        CLERK_WAKEUP_THRESHOLD) {
      Signal(clerk->passport_office_->manager_->wakeup_condition_,
          clerk->passport_office_->manager_->wakeup_condition_lock_);
    }
		Wait(clerk->bribe_line_lock_cv_, clerk->bribe_line_lock_);
		Release(clerk->bribe_line_lock_);
	} else {
		Acquire(clerk->regular_line_lock_);
    ++clerk->passport_office_->clerk->line_counts_[clerk->type_][clerk->identifier_];
    if (clerk->passport_office_->GetNumCustomersForClerkType(clerk->type_) > 
        CLERK_WAKEUP_THRESHOLD) {
      Signal(clerk->passport_office_->manager_->wakeup_condition_,
          clerk->passport_office_->manager_->wakeup_condition_lock_);
    }
		Wait(clerk->regular_line_lock_cv_ , clerk->regular_line_lock_);
		Release(clerk->regular_line_lock_);
	}
}

int GetNumCustomersInLine(Clerk* clerk) const {
  return clerk->passport_office_->line_counts_[clerk->type_][clerk->identifier_] +
    clerk->passport_office_->bribe_line_counts_[clerk->type_][clerk->identifier_];
}

void GetNextCustomer(Clerk* clerk) {
//  lines_lock_.Acquire();
	Acquire(clerk->passport_office_->line_locks_[clerk->type_]);
  Acquire(clerk->bribe_line_lock_);
  int bribe_line_count = 
      clerk->passport_office_->bribe_line_counts_[clerk->type_][clerk->identifier_];
  Release(clerk->bribe_line_lock_);

  Acquire(clerk->regular_line_lock_);
  int regular_line_count = 
      clerk->passport_office_->line_counts_[clerk->type_][clerk->identifier_];
  Release(clerk->regular_line_lock_);

	if (bribe_line_count > 0) {
    std::cout << clerk->clerk_type_ << " [" << clerk->identifier_
      << "] has signalled a Customer to come to their counter." << std::endl;
		Acquire(clerk->bribe_line_lock_);
    Acquire(clerk->wakeup_lock_);
    Signal(clerk->bribe_line_lock_cv_, clerk->bribe_line_lock_);
		Release(clerk->bribe_line_lock_);
    clerk->state_ = clerk_states::kBusy;
    clerk->passport_office_->bribe_line_counts_[clerk->type_][clerk->identifier_]--;
  } else if (regular_line_count > 0) {
    std::cout << clerk->clerk_type_ << " [" << clerk->identifier_
      << "] has signalled a Customer to come to their counter." << std::endl;
    Acquire(clerk->regular_line_lock_);
		Acquire(clerk->wakeup_lock_);
    Signal(clerk->regular_line_lock_cv_, clerk->regular_line_lock_);
		Release(clerk->regular_line_lock_);
    clerk->state_ = clerk_states::kBusy;
    clerk->passport_office_->line_counts_[clerk->type_][clerk->identifier_]--;
  } else {
    clerk->state_ = clerk_states::kOnBreak;
  }
	// lines_lock_.Release();
	Release(clerk->passport_office_->line_locks_[clerk->type_]);
}

void ApplicationClerkWork(Clerk* clerk) {
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  // Wait for customer to put passport on counter.
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  clerk->current_customer_->completed_application_ = 1;
  std::cout << clerk->clerk_type_ << " [" << clerk->identifier_ 
      << "] has recorded a completed application for " 
      << clerk->current_customer_->IdentifierString() << std::endl;
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
}

void PictureClerkWork(Clerk* clerk) {
  // Take Customer's picture and wait to hear if they like it.
  std::cout << clerk->clerk_type_ << " [" << clerk->identifier_ << "] has taken a picture of "
            << clerk->current_customer_->IdentifierString() << std::endl;
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  int picture_accepted = customer_input_;

  // If they don't like their picture don't set their picture to taken.  They go back in line.
  if (!picture_accepted) {
    std::cout << clerk_type_ << " [" << clerk->identifier_ 
        << "] has been told that " << clerk->current_customer_->IdentifierString()
        << " does not like their picture" << std::endl;
  } else {
    // Set picture taken.
    clerk->current_customer_->picture_taken_ = 1;
    std::cout << clerk->clerk_type_ << " [" << clerk->identifier_ 
        << "] has been told that " << clerk->current_customer_->IdentifierString() 
        << " does like their picture" << std::endl;
  }
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
}

void PassportClerkWork(Clerk* clerk) {
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  // Wait for customer to show whether or not they got their picture taken and passport verified.
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  int picture_taken_and_passport_verified = clerk->customer_input_;

  // Check to make sure their picture has been taken and passport verified.
  if (!picture_taken_and_passport_verified) {
    std::cout << clerk->clerk_type_ << " [" << clerk->identifier_ 
        << "] has determined that " << clerk->current_customer_->IdentifierString() 
        << " does not have both their application and picture completed" 
        << std::endl;
  } else {
    std::cout << clerk->clerk_type_ << " [" << clerk->identifier_ 
        << "] has determined that " << clerk->current_customer_->IdentifierString() 
        << " does have both their application and picture completed" 
        << std::endl;
    clerk->current_customer_->certified_ = 1;
    std::cout << clerk->clerk_type_ << " [" << clerk->identifier_ 
        << "] has recorded " << clerk->current_customer_->IdentifierString() 
        << " passport documentation" << std::endl;
  }
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
}

void CashierClerkWork(Clerk* clerk) {
  // Collect application fee.
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  Acquire(clerk->money_lock_);
  clerk->collected_money_ += clerk->customer_money_;
  clerk->customer_money_ = 0;
  Release(clerk->money_lock_);

  // Wait for the customer to show you that they are certified.
  Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  Wait(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
  int certified = clerk->customer_input_;

  // Check to make sure they have been certified.
  if (!certified) {
    std::cout << clerk->clerk_type_ << " [" << clerk->identifier_ 
        << "] has received the $100 from "
        << clerk->current_customer_->IdentifierString() 
        << " before certification. They are to go to the back of my line." 
        << std::endl;

    // Give money back.
    Acquire(clerk->money_lock_);
    clerk->collected_money_ -= 100;
    Release(clerk->money_lock_);
    clerk->current_customer_->money_ += 100;
  } else {
    std::cout << clerk->clerk_type_ << " [" << clerk->identifier_ 
        << "] has received the $100 from "
        << clerk->current_customer_->IdentifierString() 
        << " after certification." << std::endl;

    // Give customer passport.
    std::cout << clerk->clerk_type_ << " [" << clerk->identifier_ 
        << "] has provided "
        << clerk->current_customer_->IdentifierString() 
        << " their completed passport." << std::endl;

    // Record passport was given to customer.
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
			// Wait for customer to come to counter and give SSN.
			Wait(clerk->wakeup_lock_cv_ , clerk->wakeup_lock_);
			// Take Customer's SSN and verify passport.
			std::cout << IdentifierString() << " has received SSN "
								<< clerk->customer_ssn_ << " from "
                << clerk->current_customer_->IdentifierString()
								<< std::endl;
			// Do work specific to the type of clerk.
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

			// Collect bribe money.
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
			// Random delay.
			int random_time = rand() % 80 + 20;
			for (int i = 0; i < random_time; ++i) {
				currentThread->Yield();
			}
			// Wakeup customer.
		} else if (clerk->state_ == clerk_states::kOnBreak) {
      Acquire(clerk->wakeup_lock_);
      // Wait until woken up.
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
