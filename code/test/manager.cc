#include "manager.h"

#include "../userprog/syscall.h"

Manager CreateManager(PassportOffice* passport_office) {
  Manager manager;
  manager.passport_office_ = passport_office;
  manager.wakeup_condition_ = CreateCondition("Manager Wakeup Lock Condition");
  manager.wakeup_condition_lock_ = CreateLock("Manager Wakeup Lock");
  running_ = true;
  elapsed_ = 0;
  return manager;
}

void ManagerPrintMoneyReport() {
  while (running_) {
    for (int j = 0; j < clerk_types::Size; ++j) {
      for (int i = 0; i < passport_office_->clerks_[j].size(); ++i) {
        int m = passport_office_->clerks_[j][i]->CollectMoney();
        money_[passport_office_->clerks_[j][i]->type_] += m;
      }
    }
    int total = 0;
    for (int i = 0; i < clerk_types::Size; ++i) {
      total += money_[i];
      std::cout << "Manager has counted a total of $" << money_[i] << " for "
                << Clerk::NameForClerkType(static_cast<clerk_types::Type>(i)) << 's' << std::endl;
    }
    std::cout << "Manager has counted a total of $" << total
              <<  " for the passport office" << std::endl;
    for(int i = 0; i < 200; ++i) {
      if (!running_) return;
      Yield();
    }
  }
}

void ManagerRun() {
  running_ = true;
  Thread* report_timer_thread = new Thread("Report timer thread");
  report_timer_thread->Fork(&RunPrintMoney, reinterpret_cast<int>(this));
  while(running_) {
    Acquire(wakeup_condition_lock_);
    Wait(wakeup_condition_, wakeup_condition_lock_);
		if (!running_) {
			break;
		}
    for (int i = 0; i < clerk_types::Size; ++i) {
      Acquire(passport_office_->line_locks_[i]);
    }
    for (int i = 0; i < clerk_types::Size; ++i) {
      if (passport_office_->GetNumCustomersForClerkType(
          static_cast<clerk_types::Type>(i)) > CLERK_WAKEUP_THRESHOLD ||
          passport_office_->num_senators_ > 0) {
        for (int j = 0; j < passport_office_->clerks_[i].size(); ++j) {
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
    for (int i = 0; i < clerk_types::Size; ++i) {
      Release(passport_office_->line_locks_[i]);
    }
    Release(wakeup_condition_lock_);
  }
}

void WakeWaitingCustomers() {
  for (int i = 0; i < passport_office_->clerks_.size(); ++i) {
    for (int j = 0; j < passport_office_->clerks_[i].size(); ++j) {
      Clerk* clerk = passport_office_->clerks_[i][j];
      Broadcast(clerk->bribe_line_lock_cv_, clerk->bribe_line_lock_);
      Broadcast(clerk->regular_line_lock_cv_, clerk->regular_line_lock_);
    }
  }
}

void WakeClerksForSenator() {
  for (int i = 0; i < passport_office_->clerks_.size(); ++i) {
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
