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
      manager_thread_("manager thread") {
  // std::stringstream ss;
  for (int i = 0; i < clerk_types::Size; ++i) {
    char* name = new char[80];
    sprintf(name, "Line Lock %d", i); 
    printf("Assigning lock %d - %s\n", i, name);
    Lock* fucking_lock = new Lock(name);
    printf("Fucking lock: %s - %d\n", fucking_lock->getName(), fucking_lock);
    line_locks_.push_back(fucking_lock);
    printf("Line fucking lock[0]: %d\n", line_locks_[i]);
  }
   
  // Lock* fucking_lock = new Lock("0");
  // line_locks_.push_back(fucking_lock);

  // Lock* fucking_lock1 = new Lock("1");
  // line_locks_.push_back(fucking_lock1);

  // Lock* fucking_lock2 = new Lock("2");
  // line_locks_.push_back(fucking_lock2);

  // Lock* fucking_lock3 = new Lock("3");
  // line_locks_.push_back(fucking_lock3);

  for (int i = 0; i < clerk_types::Size; ++i) {
    printf("Line lock: %d - %s\n", line_locks_[i], line_locks_[i]->getName());
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
      Thread* thread = new Thread("clerk thread");
      thread_list_.push_back(thread);
      thread->Fork(thread_runners::RunClerk, (int) clerks_[i][j]);
    }
  }
}

void PassportOffice::Stop() {
 for (unsigned int i = 0 ; i < thread_list_.size(); ++i) {
  thread_list_[i]->Finish();
 }
}

void PassportOffice::AddNewCustomer(Customer* customer) {
  Thread* thread = new Thread("customer thread");
  thread->Fork(thread_runners::RunCustomer, (int) customer);
  thread_list_.push_back(thread);
}

void PassportOffice::AddNewSenator(Senator* senator) {
  Thread* thread = new Thread("senator thread");
  thread->Fork(thread_runners::RunSenator, (int) senator);
  thread_list_.push_back(thread);
}
