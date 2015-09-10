#include "passport_office.h"

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

PassportOffice::PassportOffice()
    : application_line_lock_("application line lock"),
      picture_line_lock_("picture line lock"),
      passport_line_lock_("passport line lock"),
      cashier_line_lock_("cashier line lock"), 
      application_clerk_thread_("application clerk thread"),
      picture_clerk_thread_("picture clerk thread"),
      passport_clerk_thread_("passport clerk thread"),
      cashier_clerk_thread_("cashier clerk thread"),
      manager_thread_("manager thread") {
  application_clerk_ = new ApplicationClerk(this);
  picture_clerk_ = new PictureClerk(this);
  passport_clerk_ = new PassportClerk(this);
  cashier_clerk_ = new CashierClerk(this);
  manager_ = new Manager(this, application_clerk_, picture_clerk_,
                         passport_clerk_, cashier_clerk_);
}

void PassportOffice::Start() {
  application_clerk_thread_.Fork(thread_runners::RunClerk, application_clerk_);
  picture_clerk_thread_.Fork(thread_runners::RunClerk, picture_clerk_);
  passport_clerk_thread_.Fork(thread_runners::RunClerk, passport_clerk_);
  cashier_clerk_thread_.Fork(thread_runners::RunClerk, cashier_clerk_);
  manager_thread_.Fork(thread_runners::RunManager, manager_);
}

void PassportOffice::AddNewCustomer(Customer* customer) {
  Thread* customer_thread = new Thread("customer thread");
  customer_thread->Fork(thread_runners::RunCustomer, customer);
}

void PassportOffice::AddNewSenator(Senator* senator) {
  Thread* senator_thread = new Thread("senator thread");
  senator_thread->Fork(thread_runners::RunCustomer, senator);
}

void PassportOffice::AddCustomerToApplicationLine(Customer* customer) {
  AddCustomerToLine(
      customer, &application_line_, &application_line_lock_);
}

void PassportOffice::AddCustomerToPictureLine(Customer* customer) {
  AddCustomerToLine(customer, &picture_line_, &picture_line_lock_);
}

void PassportOffice::AddCustomerToPassportLine(Customer* customer) {
  AddCustomerToLine(customer, &passport_line_, &passport_line_lock_);
}

void PassportOffice::AddCustomerToCashierLine(Customer* customer) {
  AddCustomerToLine(customer, &cashier_line_, &cashier_line_lock_);
}

void PassportOffice::AddCustomerToLine(
    Customer* customer, Deque* line, Lock* lock) {
  MutexLock l(lock);
  line->push_back(customer);
}

bool PassportOffice::MoveAheadInApplicationLine(Customer* customer) {
  return MoveAheadInLine(
      customer, &application_line_, &application_line_lock_);
}

bool PassportOffice::MoveAheadInPictureLine(Customer* customer) {
  return MoveAheadInLine(customer, &picture_line_, &picture_line_lock_);
}

bool PassportOffice::MoveAheadInPassportLine(Customer* customer) {
  return MoveAheadInLine(customer, &passport_line_, &passport_line_lock_);
}

bool PassportOffice::MoveAheadInCashierLine(Customer* customer) {
  return MoveAheadInLine(customer, &cashier_line_, &cashier_line_lock_);
}

bool PassportOffice::MoveAheadInLine(
    Customer* customer, Deque* line, Lock* lock) {
  MutexLock l(lock);
  int customer_index = -1;
  for (int i = 0; i < line.size(); ++i) {
    if (line[i] == customer) {
      customer_index = i;
      break;
    }
  }
  if (customer_index == -1 || customer_index == 0) return false;
  
  int new_index = -1;
  for (int i = 0; i < customer_index - 1; ++i) {
    if (!line[i]->has_bribed()) new_index = i;
  }
  if (new_index == -1) return false;

  line.erase(line.begin() + customer_index);
  line.insert(line.begin() + new_index, customer);
  return true;
}

Customer* PassportOffice::RemoveCustomerFromApplicationLine() {
  return RemoveCustomerFromLine(&application_line_, &application_line_lock_);
}

Customer* PassportOffice::RemoveCustomerFromPictureLine() {
  return RemoveCustomerFromLine(&picture_line_, &picture_line_lock_);
}

Customer* PassportOffice::RemoveCustomerFromPassportLine() {
  return RemoveCustomerFromLine(&passport_line_, &passport_line_lock_);
}

Customer* PassportOffice::RemoveCustomerFromCashierLine() {
  return RemoveCustomerFromLine(&cashier_line_, &cashier_line_lock_);
}

Customer* PassportOffice::RemoveCustomerFromLine(Deque* line, Lock* lock) {
  MutexLock l(lock);
  if (line->size()) {
    return line->pop_front();
  }
  return NULL;
}

