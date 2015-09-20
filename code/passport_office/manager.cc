#include "manager.h"

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
  uint32_t total = 0;
  for (uint32_t i = 0; i < clerk_types::Size; ++i) {
    total += money_[i];
    std::cout << "Manager has counted a total of $[" << money_[i] << "] for "
              << Clerk::NameForClerkType(i) << 's' << std::endl;
  }
  std::cout << "Manager has counted a total of $[" << total
            <<  "] for the passport office" << std::endl;
}

void Manager::Run() {
  running_ = true;
  Timer report_timer(std::bind(&PrintMoneyReport, this), 0);
  while(running_) {
    wakeup_condition_lock_.Acquire();
    wakeup_condition_.Wait(wakeup_condition_lock_);
    passport_office_->breaking_clerks_lock_->Acquire();
    uint32_t n = passport_office_->breaking_clerks_.size();
    for (uint32_t i = 0; i < n; ++i) {
      Clerk* clerk = passport_office_->breaking_clerks_[i];
      passport_office_->line_locks_[clerk->type()]->Acquire();
      if (clerk->GetNumCustomersInLine() > CLERK_WAKEUP_THRESHOLD) {
        clerk->WakeUp();
        std::cout << "Manager has woken up a"
                  << (clerk->type_ == clerk_types::kApplication ? "n " : " ")
                  << Clerk::NameForClerkType(clerk->type_) << std::endl;
        passport_office_->breaking_clerks_->remove(i);
        --n;
      }
      passport_office_->line_locks_[clerk->type()]->Release();
    }
    passport_office_->breaking_clerks_lock_->Release();
  }
}
