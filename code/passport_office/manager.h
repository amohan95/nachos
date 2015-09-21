#ifndef PASSPORT_OFFICE_MANAGER_H
#define PASSPORT_OFFICE_MANAGER_H

#include "synch.h"
#include "utilities.h"

#include <stdint.h>
#include <vector>


#define CLERK_WAKEUP_THRESHOLD 3
#define MONEY_REPORT_INTERVAL 5000

class PassportOffice;

void PrintMoneyReport(int manager);

class Manager {
 friend void PrintMoneyReport(int manager);
 public:
  Manager(PassportOffice* passport_office);
  ~Manager();

  void Run();
  void WakeUp();

  Condition wakeup_condition_;
  Lock wakeup_condition_lock_;
 private:
  std::vector<uint32_t> money_;
  PassportOffice* passport_office_;
  bool running_;
};
#endif
