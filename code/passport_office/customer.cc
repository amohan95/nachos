#include "Customer.h"

#include <cstdlib>

uint32_t Customer::CURRENT_UNUSED_SSN = 0;

Customer::Customer() :
  certified_(false),
  done_(false),
  money_(INITIAL_MONEY_AMOUNTS[rand() % INITIAL_MONEY_AMOUNTS.size()]),
  passport_verified_(false),
  picture_taken_(false),
  ssn_(std::to_string(CURRENT_UNUSED_SSN++)) {
}

Customer::~Customer() {
}

void Customer::Run() {
  while (!done_) {
    ClerkType next_clerk;
    if (!passport_verified() && !picture_taken()) {
      next_clerk = rand() % 2; // either APPLICATION (0) or PICTURE (1)
    } else if (!passport_verified()) {
      next_clerk = APPLICATION;
    } else if (!picture_taken()) {
      next_clerk = PICTURE;
    } else if (!certified()) {
      next_clerk = CASHIER;
    } else {
      done_ = true;
    }
    passport_office_->line_locks_[next_clerk]->Acquire();
    uint32_t shortest = 0;
    for (uint32_t i = 0; i < passport_office_->lines_.size(); ++i) {
      if (passport_office_->lines_[i].size() < passport_office_->lines_[shortest].size()) {
        shortest = i;
      }
    }
    passport_office_->line_locks_[next_clerk]->Release();

    passport_office->lines_[shortest].push_back(this);
    wakeup_condition_->Wait(wakeup_lock_);
  }
}

