#ifndef PASSPORT_OFFICE_CLERKS_H
#define PASSPORT_OFFICE_CLERKS_H

#include "utilities.h"

struct PassportOffice;
struct Customer;

typedef struct {
  int lines_lock_;
  int bribe_line_lock_cv_;
  int bribe_line_lock_;
  int regular_line_lock_cv_;
  int regular_line_lock_;
  int wakeup_lock_cv_;
  int wakeup_lock_;
  int money_lock_;
  int customer_ssn_;
  Customer* current_customer_;
  int customer_money_;
  int customer_input_;
  char * clerk_type_;
  clerk_types::Type type_;
  clerk_states::State state_;
  PassportOffice* passport_office_;
  int collected_money_;
  int identifier_;
  int running_;
} Clerk;

#endif // PASSPORT_OFFICE_CLERK_H
