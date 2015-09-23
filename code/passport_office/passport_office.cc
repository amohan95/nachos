#include "passport_office.h"

#include <string>
#include <cstdlib>

namespace thread_runners {

void RunManager(int arg) {
  Manager* manager = (Manager*) arg;
  manager->Run();
}

void RunClerk(int arg) {
  Clerk* clerk = (Clerk*) arg;
  clerk->Run();
}

void RunCustomer(int arg) {
  Customer* customer = (Customer*) arg;
  customer->Run();
}

void RunSenator(int arg) {
  Senator* senator = (Senator*) arg;
  senator->Run();
}

}

PassportOffice::PassportOffice(
    int num_application_clerks, int num_picture_clerks,
    int num_passport_clerks, int num_cashier_clerks) :
      clerks_(clerk_types::Size, std::vector<Clerk*>()),
      line_counts_(clerk_types::Size, std::vector<int>()),
      bribe_line_counts_(clerk_types::Size, std::vector<int>()),
      breaking_clerks_lock_(new Lock("breaking clerks lock")),
      senator_lock_(new Lock("senator lock")),
      senator_condition_(new Condition("senator condition")),
      customer_count_lock_("customer count lock"),
      num_customers_waiting_lock_("customer waiting counter"),
      num_customers_waiting_(0),
      manager_thread_("manager thread") {
  for (int i = 0; i < clerk_types::Size; ++i) {
    char* name = new char[80];
    sprintf(name, "Line Lock %d", i);
    line_locks_.push_back(new Lock(name));
  }

  for (int i = 0; i < num_application_clerks; ++i) {
    clerks_[clerk_types::kApplication].push_back(new ApplicationClerk(this, i));
    line_counts_[clerk_types::kApplication].push_back(0);
    bribe_line_counts_[clerk_types::kApplication].push_back(0);
  }

  for (int i = 0; i < num_picture_clerks; ++i) {
    clerks_[clerk_types::kPicture].push_back(new PictureClerk(this, i));
    line_counts_[clerk_types::kPicture].push_back(0);
    bribe_line_counts_[clerk_types::kPicture].push_back(0);
  }

  for (int i = 0; i < num_passport_clerks; ++i) {
    clerks_[clerk_types::kPassport].push_back(new PassportClerk(this, i));
    line_counts_[clerk_types::kPassport].push_back(0);
    bribe_line_counts_[clerk_types::kPassport].push_back(0);
  }

  for (int i = 0; i < num_cashier_clerks; ++i) {
    clerks_[clerk_types::kCashier].push_back(new CashierClerk(this, i));
    line_counts_[clerk_types::kCashier].push_back(0);
    bribe_line_counts_[clerk_types::kCashier].push_back(0);  
  }

  manager_ = new Manager(this);
}

void PassportOffice::Start() {
  manager_thread_.Fork(thread_runners::RunManager, (int) manager_);
  for (unsigned i = 0; i < clerks_.size(); ++i) {
    for (unsigned j = 0; j < clerks_[i].size(); ++j) {
      std::string id = clerks_[i][j]->IdentifierString();
      char* id_str = new char[id.length()];
      std::strcpy(id_str, id.c_str());
      Thread* thread = new Thread(id_str);
      thread_list_.push_back(thread);
      thread->Fork(thread_runners::RunClerk, (int) clerks_[i][j]);
    }
  }
}

void PassportOffice::WaitOnFinish() {
  while (customers_.size() > 0) {
    for (int i = 0; i < 400; ++i) {
      currentThread->Yield();
    }
    num_customers_waiting_lock_.Acquire();
    if (customers_.size() == num_customers_waiting_) {
      num_customers_waiting_lock_.Release();
      bool done = true;
      breaking_clerks_lock_->Acquire();
      for (unsigned int i = 0; i < clerks_.size(); ++i) {
        line_locks_[i]->Acquire();
        for (unsigned int j = 0; j < clerks_[i].size(); ++j) {
          if (clerks_[i][j]->state_ != clerk_states::kOnBreak ||
              GetNumCustomersForClerkType(static_cast<clerk_types::Type>(i)) > 
              CLERK_WAKEUP_THRESHOLD) {
            done = false;
            break;
          }
        }
        line_locks_[i]->Release();
        if (done) break;
      }
      breaking_clerks_lock_->Release();
      if (done) return;
    } else {
      num_customers_waiting_lock_.Release();
    }
  }
}

void PassportOffice::Stop() {
  printf("Attempting to stop passport office\n");
  while (customers_.size() > 0) {
    bool done = true;
    breaking_clerks_lock_->Acquire();
    for (unsigned int i = 0; i < clerks_.size(); ++i) {
      for (unsigned int j = 0; j < clerks_[i].size(); ++j) {
        if (clerks_[i][j]->state_ != clerk_states::kOnBreak) {
          done = false;
        }
      }
    }
    breaking_clerks_lock_->Release();
    if (done) {
      break;
    } else {
      for (int i = 0; i < 100; ++i) {
        currentThread->Yield();
      }
    }
  }
  for (std::set<Customer*>::iterator itr = customers_.begin(); itr != customers_.end(); ++itr) {
    (*itr)->set_running(false);
  }
  for (unsigned int i = 0; i < clerks_.size(); ++i) {
    line_locks_[i]->Acquire();
    for (unsigned int j = 0; j < clerks_[i].size(); ++j) {
      clerks_[i][j]->regular_line_lock_.Acquire();
      clerks_[i][j]->regular_line_lock_cv_.Broadcast(&clerks_[i][j]->regular_line_lock_);
      clerks_[i][j]->regular_line_lock_.Release();
      clerks_[i][j]->bribe_line_lock_.Acquire();
      clerks_[i][j]->bribe_line_lock_cv_.Broadcast(&clerks_[i][j]->bribe_line_lock_);
      clerks_[i][j]->bribe_line_lock_.Release();
      clerks_[i][j]->set_running(false);
      clerks_[i][j]->wakeup_lock_.Acquire();
      clerks_[i][j]->wakeup_lock_cv_.Signal(&clerks_[i][j]->wakeup_lock_);
      clerks_[i][j]->wakeup_lock_.Release();
    }
    line_locks_[i]->Release();
  }
  manager_->set_running(false);
  std::cout << "Set manager running false" << std::endl;
  manager_->wakeup_condition_lock_.Acquire();
  manager_->wakeup_condition_.Signal(&manager_->wakeup_condition_lock_);
  manager_->wakeup_condition_lock_.Release();
  for (int i = 0; i < 100000; ++i) {
    currentThread->Yield();
  }
  printf("Passport office stopped\n");
  /*for (unsigned int i = 0 ; i < thread_list_.size(); ++i) {
    thread_list_[i]->Finish();
  }
  manager_thread_.Finish();*/
}
int PassportOffice::GetNumCustomersForClerkType(clerk_types::Type type) const {
  int total = 0;
  for (unsigned int j = 0; j < line_counts_[type].size(); ++j) {
    total += line_counts_[type][j];
    total += bribe_line_counts_[type][j];
  }
  return total;
}

void PassportOffice::AddNewCustomer(Customer* customer) {
  std::string id = customer->IdentifierString();
  char* id_str = new char[id.length()];
  std::strcpy(id_str, id.c_str());
  Thread* thread = new Thread(id_str);
  thread->Fork(thread_runners::RunCustomer, (int) customer);
  thread_list_.push_back(thread);
  customer_count_lock_.Acquire();
  customers_.insert(customer);
  customer_count_lock_.Release();
}

void PassportOffice::AddNewSenator(Senator* senator) {
  Thread* thread = new Thread("senator thread");
  thread->Fork(thread_runners::RunSenator, (int) senator);
  thread_list_.push_back(thread);
  customer_count_lock_.Acquire();
  customers_.insert(senator);
  customer_count_lock_.Release();
}
