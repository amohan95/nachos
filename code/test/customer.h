#ifndef PASSPORT_OFFICE_CUSTOMER_H
#define PASSPORT_OFFICE_CUSTOMER_H

#include <stdint.h>

#include "utilities.h"

#define CLERK_BRIBE_AMOUNT 500
#define PASSPORT_FEE 100

struct PassportOffice;
struct Clerk;
struct CashierClerk;

typedef struct {
  int money_;
  const int ssn_;
  int join_line_lock_;
  int join_line_lock_cv_;
  PassportOffice* passport_office_;
  int bribed_;
  int certified_;
  int completed_application_;
  int passport_verified_;
  int picture_taken_;
  int running_;
  customer_types::Type type_;
} Customer;

#endif
