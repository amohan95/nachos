#include "passport_office.h"
#include "../userprog/syscall.h"

#include <string>
#include <cstdlib>

// Thread runners: Takes an int argument and casts to correct pointer,
// and runs this specific object.
namespace thread_runners {

void RunManager(int arg) {
  Manager* manager = reinterpret_cast<Manager*>(arg);
  manager->Run();
}

void RunClerk(int arg) {
  Clerk* clerk = reinterpret_cast<Clerk*>(arg);
  clerk->Run();
}

void RunCustomer(int arg) {
  Customer* customer = reinterpret_cast<Customer*>(arg);
  customer->Run();
}

void RunSenator(int arg) {
  Senator* senator = reinterpret_cast<Senator*>(arg);
  senator->Run();
}

}

// Initialize passport office with all locks and the correct number of clerks
// of each type.
PassportOffice::PassportOffice(
    int num_application_clerks, int num_picture_clerks,
    int num_passport_clerks, int num_cashier_clerks) :
      clerks_(clerk_types::Size, std::vector<Clerk*>()),
      breaking_clerks_lock_(CreateLock("breaking clerks lock")),
      line_counts_(clerk_types::Size, std::vector<int>()),
      bribe_line_counts_(clerk_types::Size, std::vector<int>()),
      senator_lock_(CreateLock("senator lock")),
      senator_condition_(CreateCondition("senator condition")),
      customer_count_lock_(CreateLock("customer count lock")),
      customers_served_lock_(CreateLock("num customers being served lock")),
      customers_served_cv_(CreateCondition("num customers being served cv")),
      num_customers_being_served_(0),
      num_customers_waiting_lock_(CreateLock("customer waiting counter")),
      num_customers_waiting_(0),
      num_senators_lock_(CreateLock("num senators lock")),
      num_senators_(0),
      outside_line_lock_(CreateLock("outside line lock")),
      outside_line_cv_(CreateCondition("outside line condition")),
      manager_thread_("manager thread") {
  for (int i = 0; i < clerk_types::Size; ++i) {
    char* name = new char[80];
    sprintf(name, "Line Lock %d", i);
    line_locks_.push_back(CreateLock(name));
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

// Creates the threads and forks each one for all the necessary components of 
// the passport office. 
void PassportOffice::Start() {
  manager_thread_.Fork(
      thread_runners::RunManager, reinterpret_cast<int>(manager_));
  for (unsigned i = 0; i < clerks_.size(); ++i) {
    for (unsigned j = 0; j < clerks_[i].size(); ++j) {
      std::string id = clerks_[i][j]->IdentifierString();
      char* id_str = new char[id.length()];
      std::strcpy(id_str, id.c_str());
      Thread* thread = new Thread(id_str);
      thread_list_.push_back(thread);
      thread->Fork(
          thread_runners::RunClerk, reinterpret_cast<int>(clerks_[i][j]));
    }
  }
}

// Waits for the passport office to finish or be in a state where it is no
// longer possible to complete. This means that all customers are waiting in
// a line to be served, and al clerks are on break. Additionally, the number
// of customers in each particular type of line is less than the threshold for
// the manager to wake up the clerk.
void PassportOffice::WaitOnFinish() {
  while (customers_.size() > 0) {
    for (int i = 0; i < 400; ++i) {
      currentThread->Yield();
    }
    if (num_senators_ > 0) continue;
    Acquire(num_customers_waiting_lock_);
    if (customers_.size() == num_customers_waiting_) {
      Release(num_customers_waiting_lock_);
      bool done = true;
      Acquire(breaking_clerks_lock_);
      for (unsigned int i = 0; i < clerks_.size(); ++i) {
        Acquire(line_locks_[i]);
        for (unsigned int j = 0; j < clerks_[i].size(); ++j) {
          if (clerks_[i][j]->state_ != clerk_states::kOnBreak ||
              GetNumCustomersForClerkType(static_cast<clerk_types::Type>(i)) > 
              CLERK_WAKEUP_THRESHOLD) {
            done = false;
            break;
          }
        }
        Release(line_locks_[i]);
        if (done) break;
      }
      Release(breaking_clerks_lock_);
      if (done) break;
    } else {
      Release(num_customers_waiting_lock_);
    }
  }
  for (int i = 0; i < 1000; ++i) {
    currentThread->Yield();
  }
}

// Attempts to stop the passport office. This means setting running_ to false
// on all the currently running passport office members, as well as signaling
// conditions to wake up any perpetually sleeping thread.
void PassportOffice::Stop() {
  printf("Attempting to stop passport office\n");
  while (customers_.size() > 0) {
    bool done = true;
    Acquire(breaking_clerks_lock_);
    for (unsigned int i = 0; i < clerks_.size(); ++i) {
      for (unsigned int j = 0; j < clerks_[i].size(); ++j) {
        if (clerks_[i][j]->state_ != clerk_states::kOnBreak) {
          done = false;
        }
      }
    }
    Release(breaking_clerks_lock_);
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
    Acquire(line_locks_[i]);
    for (unsigned int j = 0; j < clerks_[i].size(); ++j) {
      Acquire(clerks_[i][j]->regular_line_lock_);
      Broadcast(clerks_[i][j]->regular_line_lock_cv_, clerks_[i][j]->regular_line_lock_);
      Release(clerks_[i][j]->regular_line_lock_);
      Acquire(clerks_[i][j]->bribe_line_lock_);
      Broadcast(clerks_[i][j]->bribe_line_lock_cv_, clerks_[i][j]->bribe_line_lock_);
      Release(clerks_[i][j]->bribe_line_lock_);
      clerks_[i][j]->set_running(false);
      Acquire(clerks_[i][j]->wakeup_lock_);
      Broadcast(clerks_[i][j]->wakeup_lock_cv_, clerks_[i][j]->wakeup_lock_);
      Release(clerks_[i][j]->wakeup_lock_);
    }
    Release(line_locks_[i]);
  }
  manager_->set_running(false);
  std::cout << "Set manager running false" << std::endl;
  Acquire(manager_->wakeup_condition_lock_);
  Signal(manager_->wakeup_condition_, manager_->wakeup_condition_lock_);
  Release(manager_->wakeup_condition_lock_);
  for (int i = 0; i < 1000; ++i) {
    currentThread->Yield();
  }
  printf("Passport office stopped\n");
  /*for (unsigned int i = 0 ; i < thread_list_.size(); ++i) {
    thread_list_[i]->Finish();
  }
  manager_thread_.Finish();*/
}

// Gets the total number of customers waiting in line for a clerk type. 
int PassportOffice::GetNumCustomersForClerkType(clerk_types::Type type) const {
  int total = 0;
  for (unsigned int j = 0; j < line_counts_[type].size(); ++j) {
    total += line_counts_[type][j];
    total += bribe_line_counts_[type][j];
  }
  return total;
}

// Adds a new customer to the passport office by creating a new thread and
// forking it. Additionally, this will add the customer to the customers_ set.
void PassportOffice::AddNewCustomer(Customer* customer) {
  std::string id = customer->IdentifierString();
  char* id_str = new char[id.length()];
  std::strcpy(id_str, id.c_str());
  Thread* thread = new Thread(id_str);
  thread->Fork(thread_runners::RunCustomer, reinterpret_cast<int>(customer));
  thread_list_.push_back(thread);
  Acquire(customer_count_lock_);
  customers_.insert(customer);
  Release(customer_count_lock_);
}

// Adds a new senator to the passport office by creating a new thread and
// forking it.
void PassportOffice::AddNewSenator(Senator* senator) {
  Thread* thread = new Thread("senator thread");
  thread->Fork(thread_runners::RunSenator, reinterpret_cast<int>(senator));
  thread_list_.push_back(thread);
}
