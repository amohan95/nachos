#include "manager.h"

#include "../machine/timer.h"
#include "../machine/stats.h"

#include <iostream>

void RunPrintMoney(int manager) {
  Manager* man = reinterpret_cast<Manager*>(manager);
  man->PrintMoneyReport();
}

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

void Manager::PrintMoneyReport() {
  while (running_) {
    for (uint32_t j = 0; j < clerk_types::Size; ++j) {
      for (uint32_t i = 0; i < passport_office_->clerks_[j].size(); ++i) {
        uint32_t m = passport_office_->clerks_[j][i]->CollectMoney();
        money_[passport_office_->clerks_[j][i]->type_] += m;
      }
    }
    uint32_t total = 0;
    for (uint32_t i = 0; i < clerk_types::Size; ++i) {
      total += money_[i];
      std::cout << "Manager has counted a total of $" << money_[i] << " for "
                << Clerk::NameForClerkType(static_cast<clerk_types::Type>(i)) << 's' << std::endl;
    }
    std::cout << "Manager has counted a total of $" << total
              <<  " for the passport office" << std::endl;
    for(int i = 0; i < 200; ++i) {
      if (!running_) return;
      currentThread->Yield();
    }
  }
}

void Manager::Run() {
  running_ = true;
  Thread* report_timer_thread = new Thread("Report timer thread");
  report_timer_thread->Fork(&RunPrintMoney, reinterpret_cast<int>(this));
  while(running_) {
    wakeup_condition_lock_.Acquire();
    wakeup_condition_.Wait(&wakeup_condition_lock_);
		if (!running_) {
			break;
		}
    for (uint32_t i = 0; i < clerk_types::Size; ++i) {
      passport_office_->line_locks_[i]->Acquire();
    }
    for (uint32_t i = 0; i < clerk_types::Size; ++i) {
      if (passport_office_->GetNumCustomersForClerkType(
          static_cast<clerk_types::Type>(i)) > CLERK_WAKEUP_THRESHOLD ||
          passport_office_->num_senators_ > 0) {
        for (uint32_t j = 0; j < passport_office_->clerks_[i].size(); ++j) {
          Clerk* clerk = passport_office_->clerks_[i][j];
          if (clerk->state_ == clerk_states::kOnBreak) {
            clerk->wakeup_lock_.Acquire();
            clerk->wakeup_lock_cv_.Signal(&clerk->wakeup_lock_);
            clerk->state_ = clerk_states::kAvailable;
            clerk->wakeup_lock_.Release();
            std::cout << "Manager has woken up a"
              << (clerk->type_ == clerk_types::kApplication ? "n " : " ")
              << Clerk::NameForClerkType(clerk->type_) << std::endl;
          }
        }
      }
    }
    for (uint32_t i = 0; i < clerk_types::Size; ++i) {
      passport_office_->line_locks_[i]->Release();
    }
    wakeup_condition_lock_.Release();
  }
}

void Manager::WakeWaitingCustomers() {
  for (unsigned int i = 0; i < passport_office_->clerks_.size(); ++i) {
    for (unsigned int j = 0; j < passport_office_->clerks_[i].size(); ++j) {
      Clerk* clerk = passport_office_->clerks_[i][j];
      clerk->bribe_line_lock_cv_.Broadcast(&clerk->bribe_line_lock_);
      clerk->regular_line_lock_cv_.Broadcast(&clerk->regular_line_lock_);
    }
  }
}

void Manager::WakeClerksForSenator() {
  for (unsigned int i = 0; i < passport_office_->clerks_.size(); ++i) {
    if (!passport_office_->clerks_[i].empty()) {
      passport_office_->line_locks_[i]->Acquire();
      Clerk* clerk = passport_office_->clerks_[i][0];
      if (clerk->state_ == clerk_states::kOnBreak) {
        clerk->state_ = clerk_states::kAvailable;
        clerk->wakeup_lock_cv_.Signal(&clerk->wakeup_lock_);
      }
      passport_office_->line_locks_[i]->Release();
    }
  }
}
