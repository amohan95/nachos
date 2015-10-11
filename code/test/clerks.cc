#include "clerks.h"
#include "customer.h"
#include "../userprog/syscall.h"

#include <stdlib.h>

const char* Clerk::NameForClerkType(clerk_types::Type type) {
  static const char* NAMES[] = {
    "ApplicationClerk",
    "PictureClerk",
    "PassportClerk",
    "Cashier"
  };
  return NAMES[type];
}

Clerk::Clerk(PassportOffice* passport_office, int identifier) :
    customer_money_(0),
    customer_input_(false),
    state_(clerk_states::kAvailable),
    passport_office_(passport_office),
    collected_money_(0),
    identifier_(identifier),
    running_(false) {
  lines_lock_ = CreateLock("Clerk Lines Lock");
  bribe_line_lock_cv_ = CreateCondition("Clerk Bribe Line Condition"); 
  bribe_line_lock_ = CreateLock("Clerk Bribe Line Lock");
  regular_line_lock_cv_ = CreateCondition("Clerk Regular Line Condition"); 
  regular_line_lock_ = CreateLock("Clerk Regular Line Lock");
  wakeup_lock_cv_ = CreateCondition("Clerk Wakeup Condition");
  wakeup_lock_ = CreateLock("Clerk Wakeup Lock");
  money_lock_ = CreateLock("Clerk Money Lock");
}

Clerk::~Clerk() {
  DestroyLock(lines_lock_);
  DestroyCondition(bribe_line_lock_cv_); 
  DestroyLock(bribe_line_lock_);
  DestroyCondition(regular_line_lock_cv_); 
  DestroyLock(regular_line_lock_);
  DestroyCondition(wakeup_lock_cv_);
  DestroyLock(wakeup_lock_);
  DestroyLock(money_lock_);
}

std::string Clerk::IdentifierString() const {
	std::stringstream ss;
	ss << NameForClerkType(type_) << " [" << identifier_ << ']';
	return ss.str();
}

int Clerk::CollectMoney() {
  Acquire(money_lock_);
  int money = collected_money_;
  collected_money_ = 0;
  Release(money_lock_);
  return money;
}

void Clerk::JoinLine(bool bribe) {
	if (bribe) {
		Acquire(bribe_line_lock_);
    ++passport_office_->bribe_line_counts_[type_][identifier_];
    if (passport_office_->GetNumCustomersForClerkType(type_) > 
        CLERK_WAKEUP_THRESHOLD) {
      Signal(passport_office_->manager_->wakeup_condition_,
          passport_office_->manager_->wakeup_condition_lock_);
    }
		Wait(bribe_line_lock_cv_, bribe_line_lock_);
		Release(bribe_line_lock_);
	} else {
		Acquire(regular_line_lock_);
    ++passport_office_->line_counts_[type_][identifier_];
    if (passport_office_->GetNumCustomersForClerkType(type_) > 
        CLERK_WAKEUP_THRESHOLD) {
      Signal(passport_office_->manager_->wakeup_condition_,
          passport_office_->manager_->wakeup_condition_lock_);
    }
		Wait(regular_line_lock_cv_ ,regular_line_lock_);
		Release(regular_line_lock_);
	}
}

int Clerk::GetNumCustomersInLine() const {
  return passport_office_->line_counts_[type_][identifier_] +
    passport_office_->bribe_line_counts_[type_][identifier_];
}

void Clerk::GetNextCustomer() {
//  lines_lock_.Acquire();
	Acquire(passport_office_->line_locks_[type_]);
  Acquire(bribe_line_lock_);
  int bribe_line_count = passport_office_->bribe_line_counts_[type_][identifier_];
  Release(bribe_line_lock_);

  Acquire(regular_line_lock_);
  int regular_line_count = passport_office_->line_counts_[type_][identifier_];
  Release(regular_line_lock_);

	if (bribe_line_count > 0) {
    std::cout << clerk_type_ << " [" << identifier_
      << "] has signalled a Customer to come to their counter." << std::endl;
		Acquire(bribe_line_lock_);
    Acquire(wakeup_lock_);
    Signal(bribe_line_lock_cv_, bribe_line_lock_);
		Release(bribe_line_lock_);
    state_ = clerk_states::kBusy;
    passport_office_->bribe_line_counts_[type_][identifier_]--;
  } else if (regular_line_count > 0) {
    std::cout << clerk_type_ << " [" << identifier_
      << "] has signalled a Customer to come to their counter." << std::endl;
    Acquire(regular_line_lock_);
		Acquire(wakeup_lock_);
    Signal(regular_line_lock_cv_, regular_line_lock_);
		Release(regular_line_lock_);
    state_ = clerk_states::kBusy;
    passport_office_->line_counts_[type_][identifier_]--;
  } else {
    state_ = clerk_states::kOnBreak;
  }
	// lines_lock_.Release();
	Release(passport_office_->line_locks_[type_]);
}

void Clerk::Run() {
  running_ = true;
  while (running_) {
    GetNextCustomer();
		if (state_ == clerk_states::kBusy || state_ == clerk_states::kAvailable) {
			// Wait for customer to come to counter and give SSN.
			Wait(wakeup_lock_cv_ , wakeup_lock_);
			// Take Customer's SSN and verify passport.
			std::cout << IdentifierString() << " has received SSN "
								<< customer_ssn_ << " from "
                << current_customer_->IdentifierString()
								<< std::endl;
			// Do work specific to the type of clerk.
			ClerkWork();

			// Collect bribe money.
			if (current_customer_->has_bribed()) {
				Wait(wakeup_lock_cv_, wakeup_lock_);
				int bribe = customer_money_;
				customer_money_ = 0;
				Acquire(money_lock_);
				collected_money_ += bribe;
				Release(money_lock_);
				std::cout << IdentifierString() << " has received $"
									<< bribe << " from Customer " << customer_ssn_
									<< std::endl;
        Signal(wakeup_lock_cv_, wakeup_lock_);
			}
      Wait(wakeup_lock_cv_ ,wakeup_lock_);
			// Random delay.
			int random_time = rand() % 80 + 20;
			for (int i = 0; i < random_time; ++i) {
				currentThread->Yield();
			}
			// Wakeup customer.
		} else if (state_ == clerk_states::kOnBreak) {
      Acquire(wakeup_lock_);
      // Wait until woken up.
      std::cout << IdentifierString() << " is going on break" << std::endl;
      Signal(wakeup_lock_cv_, wakeup_lock_);
      Wait(wakeup_lock_cv_, wakeup_lock_);
			state_ = clerk_states::kAvailable;
      if (!running_) {
        break;
      }
			std::cout << IdentifierString() << " is coming off break" << std::endl;
      for (int i = 0; i < 25; ++i) { currentThread->Yield(); }
		}
		Release(wakeup_lock_);
  }
}

ApplicationClerk::ApplicationClerk(
    PassportOffice* passport_office, int identifier) 
    : Clerk(passport_office, identifier) {
    clerk_type_ = "ApplicationClerk";
    type_ = clerk_types::kApplication;
}

void ApplicationClerk::ClerkWork() {
  Signal(wakeup_lock_cv_, wakeup_lock_);
  // Wait for customer to put passport on counter.
  Wait(wakeup_lock_cv_, wakeup_lock_);
  current_customer_->set_completed_application();
  std::cout << clerk_type_ << " [" << identifier_ 
      << "] has recorded a completed application for " 
      << current_customer_->IdentifierString() << std::endl;
  Signal(wakeup_lock_cv_, wakeup_lock_);
}

PictureClerk::PictureClerk(PassportOffice* passport_office, int identifier) 
    : Clerk(passport_office, identifier) {
    clerk_type_ = "PictureClerk";
    type_ = clerk_types::kPicture;
}

void PictureClerk::ClerkWork() {
  // Take Customer's picture and wait to hear if they like it.
  std::cout << clerk_type_ << " [" << identifier_ << "] has taken a picture of "
            << current_customer_->IdentifierString() << std::endl;
  Signal(wakeup_lock_cv_, wakeup_lock_);
  Wait(wakeup_lock_cv_, wakeup_lock_);
  bool picture_accepted = customer_input_;

  // If they don't like their picture don't set their picture to taken.  They go back in line.
  if (!picture_accepted) {
    std::cout << clerk_type_ << " [" << identifier_ 
        << "] has been told that " << current_customer_->IdentifierString()
        << " does not like their picture" << std::endl;
  } else {
    // Set picture taken.
    current_customer_->set_picture_taken();
    std::cout << clerk_type_ << " [" << identifier_ 
        << "] has been told that " << current_customer_->IdentifierString() 
        << " does like their picture" << std::endl;
  }
  Signal(wakeup_lock_cv_, wakeup_lock_);
}

PassportClerk::PassportClerk(PassportOffice* passport_office, int identifier) 
    : Clerk(passport_office, identifier) {
    clerk_type_ = "PassportClerk";
    type_ = clerk_types::kPassport;
}

void PassportClerk::ClerkWork() {
  Signal(wakeup_lock_cv_, wakeup_lock_);
  // Wait for customer to show whether or not they got their picture taken and passport verified.
  Wait(wakeup_lock_cv_, wakeup_lock_);
  bool picture_taken_and_passport_verified = customer_input_;

  // Check to make sure their picture has been taken and passport verified.
  if (!picture_taken_and_passport_verified) {
    std::cout << clerk_type_ << " [" << identifier_ 
        << "] has determined that " << current_customer_->IdentifierString() 
        << " does not have both their application and picture completed" 
        << std::endl;
  } else {
    std::cout << clerk_type_ << " [" << identifier_ 
        << "] has determined that " << current_customer_->IdentifierString() 
        << " does have both their application and picture completed" 
        << std::endl;
    current_customer_->set_certified();
    std::cout << clerk_type_ << " [" << identifier_ 
        << "] has recorded " << current_customer_->IdentifierString() 
        << " passport documentation" << std::endl;
  }
  Signal(wakeup_lock_cv_, wakeup_lock_);
}

CashierClerk::CashierClerk(PassportOffice* passport_office, int identifier) 
    : Clerk(passport_office, identifier) {
    clerk_type_ = "Cashier";
    type_ = clerk_types::kCashier;
}

void CashierClerk::ClerkWork() {
  // Collect application fee.
  Signal(wakeup_lock_cv_, wakeup_lock_);
  Wait(wakeup_lock_cv_, wakeup_lock_);
  Acquire(money_lock_);
  collected_money_ += customer_money_;
  customer_money_ = 0;
  Release(money_lock_);

  // Wait for the customer to show you that they are certified.
  Signal(wakeup_lock_cv_, wakeup_lock_);
  Wait(wakeup_lock_cv_, akeup_lock_);
  bool certified = customer_input_;

  // Check to make sure they have been certified.
  if (!certified) {
    std::cout << clerk_type_ << " [" << identifier_ 
        << "] has received the $100 from "
        << current_customer_->IdentifierString() 
        << " before certification. They are to go to the back of my line." 
        << std::endl;

    // Give money back.
    Acquire(money_lock_);
    collected_money_ -= 100;
    Release(money_lock_);
    current_customer_->money_ += 100;
  } else {
    std::cout << clerk_type_ << " [" << identifier_ 
        << "] has received the $100 from "
        << current_customer_->IdentifierString() 
        << " after certification." << std::endl;

    // Give customer passport.
    std::cout << clerk_type_ << " [" << identifier_ 
        << "] has provided "
        << current_customer_->IdentifierString() 
        << " their completed passport." << std::endl;

    // Record passport was given to customer.
    std::cout << clerk_type_ << " [" << identifier_ 
        << "] has recorded that "
        << current_customer_->IdentifierString() 
        << " has been given their completed passport." << std::endl;

    current_customer_->set_passport_verified();
  }
  Signal(wakeup_lock_cv_, wakeup_lock_);

}
