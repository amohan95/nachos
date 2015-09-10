#ifndef PASSPORT_OFFICE_PASSPORT_OFFICE_H_
#define PASSPORT_OFFICE_PASSPORT_OFFICE_H_

#include "clerks.h"
#include "customer.h"
#include "manager.h"
#include "senator.h"
#include "../threads/synch.h"
#include "../threads/thread.h"

namespace line_types {

typedef enum {
  kApplication,
  kPicture,
  kPassport,
  kCashier,
  Size,
} Type;

}  // namespace line_types

class PassportOffice {
 public:
  PassportOffice(int num_application_clerks, int num_picture_clerks,
                 int num_passport_clerks, int num_cashier_clerks);
  ~PassportOffice() {}
  
  void Start();

  void CreateNewCustomer();
  void CreateNewSenator();

  // Adds a customer to a specific line.
  void AddCustomerToApplicationLine(Customer* customer);
  void AddCustomerToPictureLine(Customer* customer);
  void AddCustomerToPassportLine(Customer* customer);
  void AddCustomerToCashierLine(Customer* customer);

  // Tries to move customer ahead in its line. Assumes that the customer has
  // enough money to do so. Returns true if the customer can move ahead in line,
  // false otherwise.
  bool MoveAheadInApplicationLine(Customer* customer);
  bool MoveAheadInPictureLine(Customer* customer);
  bool MoveAheadInPassportLine(Customer* customer);
  bool MoveAheadInCashierLine(Customer* customer);

  // Returns the first customer from the appropriate line, returns NULL if the
  // line is empty.
  Customer* RemoveCustomerFromApplicationLine();
  Customer* RemoveCustomerFromPictureLine();
  Customer* RemoveCustomerFromPassportLine();
  Customer* RemoveCustomerFromCashierLine();

  const Deque& application_line() const { return application_line_; }
  const Deque& picture_line() const { return picture_line_; }
  const Deque& passport_line() const { return passport_line_; }
  const Deque& cashier_line() const { return cashier_line_; }
 private:
  void AddCustomerToLine(Customer* customer, Lock* lock);
  bool MoveAheadInLine(Customer* customer, Deque* line, Lock* lock);
  Customer* RemoveCustomerFromLine(Deque* line, Lock* lock);

  Deque application_line_;
  Deque picture_line_;
  Deque passport_line_;
  Deque cashier_line_;

  int num_application_clerks_;
  int num_picture_clerks_;
  int num_passport_clerks_;
  int num_cashier_clerks_;

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
