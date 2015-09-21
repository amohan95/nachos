#include "customer.h"

#include <cstdlib>

static const uint32_t INITIAL_MONEY_AMOUNTS[] = {100, 600, 1100, 1600};
uint32_t Customer::CURRENT_UNUSED_SSN = 0;

Customer::Customer(PassportOffice* passport_office) :
  passport_office_(passport_office),
  certified_(false),
  money_(INITIAL_MONEY_AMOUNTS[rand() % NUM_INITIAL_MONEY_AMOUNTS]),
  passport_verified_(false),
  picture_taken_(false),
  ssn_(CURRENT_UNUSED_SSN++),
  wakeup_condition_("customer wakeup condition"),
  wakeup_condition_lock_("customer wakeup lock") {
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
    bool bribed = false;
    if (CanBribe()) {
      uint32_t bribe_shortest = 0;
      for (uint32_t i = 1; i < passport_office_->bribe_line_counts_[next_clerk].size(); ++i) {
        if (passport_office_->bribe_line_counts_[next_clerk][i]
            < passport_office_->bribe_line_counts_[next_clerk][bribe_shortest]) {
          bribe_shortest = i;
        }
      }
      if (passport_office_->bribe_line_counts[next_clerk][bribe_shortest]
          < passport_office_->line_counts_[next_clerk][shortest]) {
        clerk = passport_office_->clerks_[next_clerk][bribe_shortest];
        bribed = true;
        ++passport_office_->bribe_line_counts_[next_clerk][bribe_shortest];
      } else {
        clerk = passport_office_->clerks_[next_clerk][shortest];
        ++passport_office_->line_counts_[next_clerk][shortest];
      }
    } else {
      clerk = passport_office_->clerks_[next_clerk][shortest];
      ++passport_office_->line_counts_[next_clerk][shortest];
    }
    passport_office_->line_locks_[next_clerk]->Release();
		PrintLineJoin(clerk, bribed);
		if (bribed) {
      clerk->bribe_line_lock_.Acquire();
      clerk->bribe_line_lock_cv_.Wait(&clerk->bribe_line_lock_);
      clerk->bribe_line_lock_.Release();
    } else {
      clerk->regular_line_lock_.Acquire();
      clerk->regular_line_lock_cv_.Wait(&clerk->regular_line_lock_);
      clerk->regular_line_lock_.Release();
    }
		clerk->customer_ssn_ = ssn_;
		std::cout << IdentifierString() << " has given SSN [" ssn_ "] to "
							<< clerk->IdentifierString() << '.' << std::endl;
    clerk->wakeup_lock_.Acquire();
    clerk->wakeup_lock_cv_.Signal(clerk->wakeup_lock_);
    clerk->wakeup_lock_cv_.Wait(clerk->wakeup_lock_);
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
    }
    clerk->wakeup_lock_cv_.Signal(clerk->wakeup_lock_);
    clerk->wakeup_lock_cv_.Wait(clerk_->wakeup_lock_);
    if (bribed) {
      GiveBribe(clerk);
    }
    clerk->wakeup_lock_.Release();
  }
  std::cout << IdentifierString() << " is leaving the Passport Office." << std::endl;
}

void Customer::GiveBribe(Clerk* clerk) {
  clerk->customer_money_ = CLERK_BRIBE_AMOUNT;
  clerk->wakeup_lock_cv_.Signal(clerk->wakeup_lock_);
  clerk->wakeup_lock_cv_.Wait(clerk->wakeup_lock_);
}

void Customer::PrintLineJoin(Clerk* clerk, bool bribed) const {
  std::cout << IdentifierString() << " has gotten in " << (bribed ? "bribe" : "regular")
            << " line for " << clerk->clerk_type_ << "." << std::endl;
}
