#include "customer.h"

#include <cstdlib>

static const uint32_t INITIAL_MONEY_AMOUNTS_DATA[] = {100, 600, 1100, 1600};
const uint32_t* Customer::INITIAL_MONEY_AMOUNTS = INITIAL_MONEY_AMOUNTS_DATA;
uint32_t Customer::CURRENT_UNUSED_SSN = 0;

Customer::Customer(PassportOffice* passport_office) :
  money_(INITIAL_MONEY_AMOUNTS[rand() % NUM_INITIAL_MONEY_AMOUNTS]),
  join_line_lock_("jll"),
  join_line_lock_cv_("jllcv"),
  passport_office_(passport_office),
  bribed_(false),
  certified_(false),
  completed_application_(false),
  passport_verified_(false),
  picture_taken_(false),
  running_(false),
  ssn_(CURRENT_UNUSED_SSN++) {
}

Customer::Customer(PassportOffice* passport_office, uint32_t money__) :
  money_(money__),
  join_line_lock_("jll"),
  join_line_lock_cv_("jllcv"),
  passport_office_(passport_office),
  bribed_(false),
  certified_(false),
  completed_application_(false),
  passport_verified_(false),
  picture_taken_(false),
  running_(false),
  ssn_(CURRENT_UNUSED_SSN++){
}

Customer::~Customer() {
}

bool Customer::CanBribe() const {
  return money() >= CLERK_BRIBE_AMOUNT + PASSPORT_FEE;
}

std::string Customer::IdentifierString() const {
  std::stringstream ss;
  ss << "Customer [" << ssn_ << ']';
  return ss.str();
}

void Customer::Run() {
  running_ = true;
  while (running_ && (!passport_verified() || !picture_taken() || !completed_application() || !certified())) {
    bribed_ = false;
    clerk_types::Type next_clerk;
    if (!completed_application() && !picture_taken() && passport_office_->clerks_[clerk_types::kApplication].size() > 0 && passport_office_->clerks_[clerk_types::kPicture].size() > 0) {
      next_clerk = static_cast<clerk_types::Type>(rand() % 2); // either kApplication (0) or kPicture (1)
    } else if (!completed_application()) {
      next_clerk = clerk_types::kApplication;
    } else if (!picture_taken()) {
      next_clerk = clerk_types::kPicture;
    } else if (!certified()) {
      next_clerk = clerk_types::kPassport;
    } else {
      next_clerk = clerk_types::kCashier;
    }
    Clerk* clerk = NULL;
    passport_office_->line_locks_[next_clerk]->Acquire();
    int32_t shortest = -1;
    for (uint32_t i = 0; i < passport_office_->line_counts_[next_clerk].size(); ++i) {
      if (shortest == -1 ||
          passport_office_->line_counts_[next_clerk][i]
          < passport_office_->line_counts_[next_clerk][shortest]) {
        shortest = i;
      }
    }
    if (CanBribe()) {
      int32_t bribe_shortest = -1;
      for (uint32_t i = 0; i < passport_office_->bribe_line_counts_[next_clerk].size(); ++i) {
        if (bribe_shortest == -1 ||
            passport_office_->bribe_line_counts_[next_clerk][i]
            < passport_office_->bribe_line_counts_[next_clerk][bribe_shortest]) {
          bribe_shortest = i;
        }
      }
      if (shortest == -1) {
        set_running(false);
        passport_office_->line_locks_[next_clerk]->Release();
        continue;
      }
      if (passport_office_->bribe_line_counts_[next_clerk][bribe_shortest]
          < passport_office_->line_counts_[next_clerk][shortest]) {
        clerk = passport_office_->clerks_[next_clerk][bribe_shortest];
        bribed_ = true;
//        clerk->lines_lock_.Acquire();
        ++passport_office_->bribe_line_counts_[next_clerk][bribe_shortest];
//        clerk->lines_lock_.Release();
      } else {
        clerk = passport_office_->clerks_[next_clerk][shortest];
//        clerk->lines_lock_.Acquire();
        ++passport_office_->line_counts_[next_clerk][shortest];
//        clerk->lines_lock_.Release();
      }
    } else {
      if (shortest == -1) {
        passport_office_->line_locks_[next_clerk]->Release();
        set_running(false);
        continue;
      }
      clerk = passport_office_->clerks_[next_clerk][shortest];
//      clerk->lines_lock_.Acquire();
      ++passport_office_->line_counts_[next_clerk][shortest];
//      clerk->lines_lock_.Release();
    }

    passport_office_->line_locks_[next_clerk]->Release();
    if (passport_office_->GetNumCustomersForClerkType(next_clerk) > 
        CLERK_WAKEUP_THRESHOLD) {
      passport_office_->manager_->wakeup_condition_.Signal(
          &passport_office_->manager_->wakeup_condition_lock_);
    }
		PrintLineJoin(clerk, bribed_);
//    join_line_lock_.Acquire();
//    join_line_lock_cv_.Signal(&join_line_lock_);
//    join_line_lock_.Release();
    passport_office_->num_customers_waiting_lock_.Acquire();
    ++passport_office_->num_customers_waiting_;
    passport_office_->num_customers_waiting_lock_.Release();

		clerk->JoinLine(bribed_);
    passport_office_->num_customers_waiting_lock_.Acquire();
    --passport_office_->num_customers_waiting_;
    passport_office_->num_customers_waiting_lock_.Release();

    if (!running_) {
      break;
    }
    
    clerk->wakeup_lock_.Acquire();
    clerk->customer_ssn_ = ssn();
    clerk->current_customer_ = this;
    std::cout << IdentifierString() << " has given SSN [" << ssn() << "] to "
              << clerk->IdentifierString() << '.' << std::endl;
    clerk->wakeup_lock_cv_.Signal(&clerk->wakeup_lock_);
    clerk->wakeup_lock_cv_.Wait(&clerk->wakeup_lock_);
    switch (next_clerk) {
      case clerk_types::kApplication:
        clerk->wakeup_lock_cv_.Signal(&clerk->wakeup_lock_);
        clerk->wakeup_lock_cv_.Wait(&clerk->wakeup_lock_);
        break;
      case clerk_types::kPicture:
        clerk->customer_input_ = (rand() % 10) > 0;
        clerk->wakeup_lock_cv_.Signal(&clerk->wakeup_lock_);
        clerk->wakeup_lock_cv_.Wait(&clerk->wakeup_lock_);
        break;
      case clerk_types::kCashier:
        clerk->customer_money_ = PASSPORT_FEE;
        clerk->wakeup_lock_cv_.Signal(&clerk->wakeup_lock_);
        clerk->wakeup_lock_cv_.Wait(&clerk->wakeup_lock_);
        clerk->customer_input_ = true;
        clerk->wakeup_lock_cv_.Signal(&clerk->wakeup_lock_);
        clerk->wakeup_lock_cv_.Wait(&clerk->wakeup_lock_);
        break;
      case clerk_types::kPassport:
        clerk->customer_input_ = true;
        clerk->wakeup_lock_cv_.Signal(&clerk->wakeup_lock_);
        clerk->wakeup_lock_cv_.Wait(&clerk->wakeup_lock_);
        break;
      case clerk_types::Size:
        break;
    }
    if (bribed_) {
      clerk->customer_money_ = CLERK_BRIBE_AMOUNT;
      money_ -= CLERK_BRIBE_AMOUNT;
      clerk->wakeup_lock_cv_.Signal(&clerk->wakeup_lock_);
      clerk->wakeup_lock_cv_.Wait(&clerk->wakeup_lock_);
    }
    clerk->current_customer_ = NULL;
    clerk->wakeup_lock_cv_.Signal(&clerk->wakeup_lock_);
    clerk->wakeup_lock_.Release();
  }
  if (passport_verified()) {
    std::cout << IdentifierString() << " is leaving the Passport Office." << std::endl;
  } else {
    std::cout << IdentifierString() << " terminated early because it is impossible to get a passport." << std::endl;
  }
  passport_office_->customer_count_lock_.Acquire();
  passport_office_->customers_.erase(this);
  passport_office_->customer_count_lock_.Release();
}

void Customer::PrintLineJoin(Clerk* clerk, bool bribed) const {
  std::cout << IdentifierString() << " has gotten in " << (bribed ? "bribe" : "regular")
            << " line for " << clerk->IdentifierString()  << "." << std::endl;
}
