#include "passport_office.h"

#include <string>

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

}

PassportOffice::PassportOffice(
    int num_application_clerks, int num_picture_clerks,
    int num_passport_clerks, int num_cashier_clerks)
    : manager_thread_("manager thread"),
      clerks_(clerk_types::Size, std::vector<Clerk*>()),
      breaking_clerks_lock_("breaking clerks lock") {
  for (int i = 0; i < clerk_types::Size; ++i) {
    line_locks_.push_back(new Lock(std::to_string(i).c_str()));
  }
  
  for (int i = 0; i < num_application_clerks; ++i) {
    clerks_[clerk_types::kApplication].push_back(new ApplicationClerk(this));
  }

  for (int i = 0; i < num_picture_clerks; ++i) {
    clerks_[clerk_types::kPicture].push_back(new PictureClerk(this));
  }

  for (int i = 0; i < num_passport_clerks; ++i) {
    clerks_[clerk_types::kPassport].push_back(new PassportClerk(this));
  }

  for (int i = 0; i < num_cashier_clerks; ++i) {
    clerks_[clerk_types::kCashier].push_back(new CashierClerk(this));
  }

  manager_ = new Manager(this);
}

void PassportOffice::Start() {
  manager_thread_.Fork(thread_runners::RunManager, manager_);
  
  for (const auto& clerk_list : clerks_) {
    for (Clerk* clerk : clerk_list) {
      Thread* thread = new Thread("clerk thread");
      thread_list_.push_back(thread);
      thread->Fork(thread_runners::RunClerk, clerk);
    }
  }
}

void PassportOffice::AddNewCustomer(Customer* customer) {
  Thread* thread = new Thread("customer thread");
  thread->Fork(thread_runners::RunCustomer, customer);
  thread_list_.push_back(thread);
}

void PassportOffice::AddNewSenator(Senator* senator) {
  Thread* thread = new Thread("senator thread");
  thread->Fork(thread_runners::RunCustomer, senator);
  thread_list_.push_back(thread);
}
