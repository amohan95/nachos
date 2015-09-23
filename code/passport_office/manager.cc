#include "manager.h"

#include "../machine/timer.h"
#include "../machine/stats.h"

#include <iostream>

Manager::Manager(PassportOffice* passport_office) :
	wakeup_condition_("Manager Wakeup Lock Condition"),
  wakeup_condition_lock_("Manager Wakeup Lock"),
	elapsed_(0),
  money_(clerk_types::Size, 0),
  passport_office_(passport_office),
  running_(false) {
}

Manager::~Manager() {
}

void PrintMoneyReport(int manager) {
	Manager* man = reinterpret_cast<Manager*>(manager);
  while (man->running_) {
    for (uint32_t j = 0; j < clerk_types::Size; ++j) {
      for (uint32_t i = 0; i < man->passport_office_->clerks_[j].size(); ++i) {
        uint32_t m = man->passport_office_->clerks_[j][i]->CollectMoney();
        man->money_[man->passport_office_->clerks_[j][i]->type_] += m;
      }
    }
    uint32_t total = 0;
    for (uint32_t i = 0; i < clerk_types::Size; ++i) {
      total += man->money_[i];
      std::cout << "Manager has counted a total of $" << man->money_[i] << " for "
                << Clerk::NameForClerkType(static_cast<clerk_types::Type>(i)) << 's' << std::endl;
    }
    std::cout << "Manager has counted a total of $" << total
              <<  " for the passport office" << std::endl;
    std::cout << "Passport Office has " << man->passport_office_->customers_.size() << " customers remaining." << std::endl;

    for(int i = 0; i < 200; ++i) {
      if (!man->running_) return;
      currentThread->Yield();
    }
  }
  currentThread->Finish();
}

void Manager::Run() {
  running_ = true;
  Thread* report_timer_thread = new Thread("Report timer thread");
  report_timer_thread->Fork(&PrintMoneyReport, reinterpret_cast<int>(this));
  while(running_) {
    wakeup_condition_lock_.Acquire();
    wakeup_condition_.Wait(&wakeup_condition_lock_);
    wakeup_condition_lock_.Release();
		if (!running_) {
			break;
		}
    passport_office_->breaking_clerks_lock_->Acquire();
    uint32_t n = passport_office_->breaking_clerks_.size();
    for (uint32_t i = 0; i < n; ++i) {
      Clerk* clerk = passport_office_->breaking_clerks_[i];
      passport_office_->line_locks_[clerk->type_]->Acquire();
      if (clerk->GetNumCustomersInLine() > CLERK_WAKEUP_THRESHOLD) {
        clerk->state_ = clerk_states::kAvailable;
				clerk->wakeup_lock_.Acquire();
				clerk->wakeup_lock_cv_.Signal(&clerk->wakeup_lock_);
				clerk->wakeup_lock_.Release();
        std::cout << "Manager has woken up a"
                  << (clerk->type_ == clerk_types::kApplication ? "n " : " ")
                  << Clerk::NameForClerkType(clerk->type_) << std::endl;
        passport_office_->breaking_clerks_.erase(passport_office_->breaking_clerks_.begin() + i);
        --n;
      }
      passport_office_->line_locks_[clerk->type_]->Release();
    }
    passport_office_->breaking_clerks_lock_->Release();
  }
  delete report_timer_thread;
}
