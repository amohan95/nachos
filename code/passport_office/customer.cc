#include "customer.h"

#include <cstdlib>

static const uint32_t INITIAL_MONEY_AMOUNTS_DATA[] = {100, 600, 1100, 1600};
const uint32_t* Customer::INITIAL_MONEY_AMOUNTS = INITIAL_MONEY_AMOUNTS_DATA;
uint32_t Customer::CURRENT_UNUSED_SSN = 0;

Customer::Customer(PassportOffice* passport_office) :
  money_(INITIAL_MONEY_AMOUNTS[rand() % NUM_INITIAL_MONEY_AMOUNTS]),
  passport_office_(passport_office),
  bribed_(false),
  certified_(false),
  passport_verified_(false),
  picture_taken_(false),
  ssn_(CURRENT_UNUSED_SSN++),
  wakeup_condition_lock_("customer wakeup condition lock"),
  wakeup_condition_("customer wakeup condition") {
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
  while (!passport_verified() || !picture_taken() || !completed_application() || !certified()) {
    bribed_ = false;
    clerk_types::Type next_clerk;
    if (!completed_application() && !picture_taken()) {
      next_clerk = static_cast<clerk_types::Type>(rand() % 2); // either kApplication (0) or kPicture (1)
    } else if (!completed_application()) {
      next_clerk = clerk_types::kApplication;
    } else if (!picture_taken()) {
      next_clerk = clerk_types::kPicture;
    } else if (!certified()) {
      next_clerk = clerk_types::kCashier;
    } else {
      next_clerk = clerk_types::kPassport;
    }
    Clerk* clerk = NULL;
    passport_office_->line_locks_[next_clerk]->Acquire();
    uint32_t shortest = 0;
    for (uint32_t i = 1; i < passport_office_->line_counts_[next_clerk].size(); ++i) {
      if (passport_office_->line_counts_[next_clerk][i]
          < passport_office_->line_counts_[next_clerk][shortest]) {
        shortest = i;
      }
    }
    if (CanBribe()) {
      uint32_t bribe_shortest = 0;
      for (uint32_t i = 1; i < passport_office_->bribe_line_counts_[next_clerk].size(); ++i) {
        if (passport_office_->bribe_line_counts_[next_clerk][i]
            < passport_office_->bribe_line_counts_[next_clerk][bribe_shortest]) {
          bribe_shortest = i;
        }
      }
      if (passport_office_->bribe_line_counts_[next_clerk][bribe_shortest]
          < passport_office_->line_counts_[next_clerk][shortest]) {
        clerk = passport_office_->clerks_[next_clerk][bribe_shortest];
        bribed_ = true;
        clerk->lines_lock_.Acquire();
        ++passport_office_->bribe_line_counts_[next_clerk][bribe_shortest];
        clerk->lines_lock_.Release();
      } else {
        clerk = passport_office_->clerks_[next_clerk][shortest];
        clerk->lines_lock_.Acquire();
        ++passport_office_->line_counts_[next_clerk][shortest];
        clerk->lines_lock_.Release();
      }
    } else {
      clerk = passport_office_->clerks_[next_clerk][shortest];
      clerk->lines_lock_.Acquire();
      ++passport_office_->line_counts_[next_clerk][shortest];
      clerk->lines_lock_.Release();
    }
    passport_office_->line_locks_[next_clerk]->Release();
		PrintLineJoin(clerk, bribed_);
		clerk->JoinLine(bribed_);
		clerk->customer_ssn_ = ssn();
		std::cout << IdentifierString() << " has given SSN [" << ssn() << "] to "
							<< clerk->IdentifierString() << '.' << std::endl;
    clerk->wakeup_lock_.Acquire();
    clerk->wakeup_lock_cv_.Signal(&clerk->wakeup_lock_);
    clerk->wakeup_lock_cv_.Wait(&clerk->wakeup_lock_);
    switch (next_clerk) {
      case clerk_types::kApplication:
        break;
      case clerk_types::kPicture:
        clerk->customer_input_ = (rand() % 10) > 6;
        break;
      case clerk_types::kCashier:
        clerk->customer_money_ = PASSPORT_FEE;
        break;
      case clerk_types::kPassport:
        clerk->customer_input_ = true;
        break;
      case clerk_types::Size:
        break;
    }
    clerk->wakeup_lock_cv_.Signal(&clerk->wakeup_lock_);
    clerk->wakeup_lock_cv_.Wait(&clerk->wakeup_lock_);
    if (bribed_) {
      GiveBribe(clerk);
    }
    clerk->wakeup_lock_.Release();
  }
  std::cout << IdentifierString() << " is leaving the Passport Office." << std::endl;
}

void Customer::GiveBribe(Clerk* clerk) {
  clerk->customer_money_ = CLERK_BRIBE_AMOUNT;
  clerk->wakeup_lock_cv_.Signal(&clerk->wakeup_lock_);
  clerk->wakeup_lock_cv_.Wait(&clerk->wakeup_lock_);
}

void Customer::PrintLineJoin(Clerk* clerk, bool bribed) const {
  std::cout << IdentifierString() << " has gotten in " << (bribed ? "bribe" : "regular")
            << " line for " << clerk->clerk_type_ << "." << std::endl;
}
