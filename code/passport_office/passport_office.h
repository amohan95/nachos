#ifndef PASSPORT_OFFICE_PASSPORT_OFFICE_H_
#define PASSPORT_OFFICE_PASSPORT_OFFICE_H_

#include "clerks.h"
#include "cashier.h"
#include "customer.h"
#include "manager.h"
#include "senator.h"
#include "../threads/synch.h"
#include "../threads/thread.h"

class PassportOffice {
 public:
  PassportOffice();
  ~PassportOffice() {}
  
  void Start();

  void CreateNewCustomer();
  void CreateNewSenator();

  void AddCustomerToApplicationLine(Customer* customer);
  void AddCustomerToPictureLine(Customer* customer);
  void AddCustomerToPassportLine(Customer* customer);
  void AddCustomerToCashierLine(Customer* customer);

  // Accessors - returns the first customer from the appropriate line and
  // returns it.
  Customer* RemoveCustomerFromApplicationLine();
  Customer* RemoveCustomerFromPictureLine();
  Customer* RemoveCustomerFromPassportLine();
  Customer* RemoveCustomerFromCashierLine();

  const Deque& application_line() const {
    return application_line_;
  }
  const Deque& picture_line() const { return picture_line_; }
  const Deque& passport_line() const { return passport_line_; }
  const Deque& cashier_line() const { return cashier_line_; }
 private:
  void AddCustomerToLine(Customer* customer, Lock* lock);

  Deque application_line_;
  Deque picture_line_;
  Deque passport_line_;
  Deque cashier_line_;

  Lock application_line_lock_;
  Lock picture_line_lock_;
  Lock passport_line_lock_;
  Lock cashier_line_lock_;

  Thread application_clerk_thread_;
  Thread picture_clerk_thread_;
  Thread passport_clerk_thread_;
  Thread cashier_clerk_thread_;
  Thread manager_thread_;

  Manager* manager_;
  ApplicationClerk* application_clerk_;
  PictureClerk* picture_clerk_;
  PassportClerk* passport_clerk_;
  CashierClerk* cashier_clerk_;
};

#endif  // PASSPORT_OFFICE_PASSPORT_OFFICE_H_
