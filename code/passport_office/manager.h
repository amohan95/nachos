#ifndef PASSPORT_OFFICE_MANAGER_H
#define PASSPORT_OFFICE_MANAGER_H

#include "synch.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "passport_office.h"

#define CLERK_WAKEUP_THRESHOLD 3
#define MONEY_REPORT_INTERVAL 5000

public:
  Manager(PassportOffice* passport_office);
  ~Manager();

  void PrintMoneyReport(int32_t dummy) const;
  void Run();
  void WakeUp();
 private:
  std::vector<uint32_t> money_;
  PassportOffice* passport_office_;
  bool running_;
  Condition wakeup_condition_;
  Lock wakeup_condition_lock_;
};
#endif
