#include "Customer.h"

#include <cstdlib>

static const uint32_t INITIAL_MONEY_AMOUNTS_DATA = {100, 600, 1100, 1600};
uint32_t* Customer::INITIAL_MONEY_AMOUNTS = INITIALI_MONEY_AMOUNTS_DATA;
uint32_t Customer::CURRENT_UNUSED_SSN = 0;

Customer::Customer() :
  certified_(false),
  done_(false),
  money_(INITIAL_MONEY_AMOUNTS[rand() % NUM_INITIAL_MONEY_AMOUNTS]),
  passport_verified_(false),
  picture_taken_(false),
  ssn_(CURRENT_UNUSED_SSN++) {
}

Customer::~Customer() {
}

bool Customer::CanBribe() const {
  return money() >= CLERK_BRIBE_AMOUNT + PASSPORT_FEE;
}

void Customer::Run() {
  while (!done_) {
    clerk_types::Type next_clerk;
    if (!passport_verified() && !picture_taken()) {
      next_clerk = rand() % 2; // either kApplication (0) or kPicture (1)
    } else if (!passport_verified()) {
      next_clerk = clerk_types::kApplication;
    } else if (!picture_taken()) {
      next_clerk = clerk_types::kPicture;
    } else if (!certified()) {
      next_clerk = clerk_types::kCashier;
    } else {
      done_ = true;
    }
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
            < passport_office_->bribe_line_counts[next_clerk][bribe_shortest]) {
          bribe_shortest = i;
        }
      }
      if (passport_office_->bribe_line_counts[next_clerk][bribe_shortest]
          < passport_office_->line_counts_[next_clerk][shortest]) {
        money -= CLERK_BRIBE_AMOUNT;
        ++passport_office_->bribe_line_counts_[next_clerk][bribe_shortest];
      } else {
        ++passport_office_->line_counts_[next_clerk][shortest];
      }
    } else {
      ++passport_office_->line_counts_[next_clerk][shortest];
    }
    passport_office_->line_locks_[next_clerk]->Release();
    wakeup_condition_->Wait(wakeup_lock_);
  }
}

