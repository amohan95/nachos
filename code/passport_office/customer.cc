#include "customer.h"

#include <cstdlib>
#include <iostream>
#include <sstream>

static const uint32_t INITIAL_MONEY_AMOUNTS_DATA[] = {100, 600, 1100, 1600};
uint32_t* Customer::INITIAL_MONEY_AMOUNTS = INITIAL_MONEY_AMOUNTS_DATA;
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

void Customer::GivePassportFee(CashierClerk* clerk) {
  clerk->CollectApplicationFee(PASSPORT_FEE);
  money_ -= PASSPORT_FEE;
  std::cout << IdentifierString() << " has given " << clerk->IdentifierString()
            << "$" << PASSPORT_FEE << '.' << std::endl;
}

std::string Customer::IdentifierString() const {
  std::stringstream ss;
  ss << "Customer [" << ssn() << ']';
  return ss.str();
}

void Customer::Run() {
  while (!passport_verified() || !picture_taken() || !completed_application_() || !certified()) {
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
            < passport_office_->bribe_line_counts[next_clerk][bribe_shortest]) {
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
		clerk->JoinLine(bribed);
		clerk->customer_ssn_ = ssn();
		std::cout << IdentifierString() << " has given SSN [" ssn() "] to "
							<< clerk->IdentifierString() << '.' << std::endl;
    clerk->wakeup_lock_.Acquire();
    clerk->wakeup_lock_cv_.Signal(clerk->wakeup_lock_);
    clerk->wakeup_lock_cv_.Wait(clerk->wakeup_lock_);
    switch (next_clerk) {
      case kApplication:
        break;
      case kPicture:
        clerk->customer_input_ = (rand() % 10) > 6;
        break;
      case kCashier:
        clerk->customer_money_ = PASSPORT_FEE;
        break;
      case kPassport:
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
            << " line for " << clerk->IdentifierString() << "." << std::endl;
}
