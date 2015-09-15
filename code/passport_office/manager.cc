#include "Manager.h"

#include "../machine/timer.h"

#include <iostream>

Manager::Manager(PassportOffice* passport_office) :
  money_(clerk_types::Size, 0),
  passport_office_(passport_office),
  running_(false) {
}

Manager::~Manager() {
}

void Manager::PrintMoneyReport(int32_t dummy) const {
  for (uint32_t i = 0; i < passport_office_->clerks_.size(); ++i) {
    uint32_t m = passport_office_->clerks_[i]->collected_money();
    money_[passport_office_->clerks[i]->type()] += m;
    passport_office_->clerks[i]->CollectMoney(m);
  }
  std::cout << "$$$ Money Report $$$" << std::endl;
  uint32_t total = 0;
  for (uint32_t i = 0; i < clerk_types::Size; ++i) {
    total += money_[i];
    std::cout << Clerk::NameForClerkType(i) << ": $"
              << money_[i] << std:endl;
  }
  std::cout << "Total: $" << total << std::endl;
}

void Manager::Run() {
  running_ = true;
  Timer report_timer(std::bind(&PrintMoneyReport, this), 0);
  while(running_) {
    wakeup_condition_.Wait(wakeup_condition_lock_);
    passport_office_->breaking_clerks_lock_->Acquire();
    uint32_t n = passport_office_->breaking_clerks_.size();
    for (uint32_t i = 0; i < n; ++i) {
      Clerk* clerk = passport_office_->breaking_clerks_[i];
      passport_office_->line_locks_[clerk->type()]->Acquire();
      if (clerk->line_.size() > CLERK_WAKEUP_THRESHOLD) {
        clerk->WakeUp();
        passport_office_->breaking_clerks_->remove(i);
        --n;
      }
      passport_office_->line_locks_[clerk->type()]->Release();
    }
    passport_office_->breaking_clerks_lock_->Release();
  }
}
