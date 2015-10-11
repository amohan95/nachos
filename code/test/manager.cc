#include "manager.h"

#include "../machine/timer.h"
#include "../machine/stats.h"
#include "../userprog/syscall.h"

#include <iostream>

void RunPrintMoney(int manager) {
  Manager* man = reinterpret_cast<Manager*>(manager);
  man->PrintMoneyReport();
}

Manager::Manager(PassportOffice* passport_office) :
    elapsed_(0),
    money_(clerk_types::Size, 0),
    passport_office_(passport_office),
    running_(false) {
  wakeup_condition_ = CreateCondition("Manager Wakeup Lock Condition");
  wakeup_condition_lock_ = CreateLock("Manager Wakeup Lock");
}

Manager::~Manager() {
  DestroyCondition(wakeup_condition_);
  DestroyLock(wakeup_condition_lock_);
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
    Acquire(wakeup_condition_lock_);
    Wait(wakeup_condition_, wakeup_condition_lock_);
		if (!running_) {
			break;
		}
    for (uint32_t i = 0; i < clerk_types::Size; ++i) {
      Acquire(passport_office_->line_locks_[i]);
    }
    for (uint32_t i = 0; i < clerk_types::Size; ++i) {
      if (passport_office_->GetNumCustomersForClerkType(
          static_cast<clerk_types::Type>(i)) > CLERK_WAKEUP_THRESHOLD ||
          passport_office_->num_senators_ > 0) {
        for (uint32_t j = 0; j < passport_office_->clerks_[i].size(); ++j) {
          Clerk* clerk = passport_office_->clerks_[i][j];
          if (clerk->state_ == clerk_states::kOnBreak) {
            Acquire(clerk->wakeup_lock_);
            Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
            clerk->state_ = clerk_states::kAvailable;
            Release(clerk->wakeup_lock_);
            std::cout << "Manager has woken up a"
              << (clerk->type_ == clerk_types::kApplication ? "n " : " ")
              << Clerk::NameForClerkType(clerk->type_) << std::endl;
          }
        }
      }
    }
    for (uint32_t i = 0; i < clerk_types::Size; ++i) {
      Release(passport_office_->line_locks_[i]);
    }
    Release(wakeup_condition_lock_);
  }
}

void Manager::WakeWaitingCustomers() {
  for (unsigned int i = 0; i < passport_office_->clerks_.size(); ++i) {
    for (unsigned int j = 0; j < passport_office_->clerks_[i].size(); ++j) {
      Clerk* clerk = passport_office_->clerks_[i][j];
      Broadcast(clerk->bribe_line_lock_cv_, clerk->bribe_line_lock_);
      Broadcast(clerk->regular_line_lock_cv_, clerk->regular_line_lock_);
    }
  }
}

void Manager::WakeClerksForSenator() {
  for (unsigned int i = 0; i < passport_office_->clerks_.size(); ++i) {
    if (!passport_office_->clerks_[i].empty()) {
      Acquire(passport_office_->line_locks_[i]);
      Clerk* clerk = passport_office_->clerks_[i][0];
      if (clerk->state_ == clerk_states::kOnBreak) {
        clerk->state_ = clerk_states::kAvailable;
        Signal(clerk->wakeup_lock_cv_, clerk->wakeup_lock_);
      }
      Release(passport_office_->line_locks_[i]);
    }
  }
}
