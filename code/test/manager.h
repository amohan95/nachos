#ifndef PASSPORT_OFFICE_MANAGER_H
#define PASSPORT_OFFICE_MANAGER_H

#include "utilities.h"

#define CLERK_WAKEUP_THRESHOLD 3
#define MONEY_REPORT_INTERVAL 5000

struct PassportOffice;

typedef struct {
  int wakeup_condition_;
  int wakeup_condition_lock_;
  int elapsed_;
  int money_[clerk_types::Size];
  PassportOffice* passport_office_;
  int running_;
} Manager;

#endif
