#ifndef PASSPORT_OFFICE_MANAGER_H
#define PASSPORT_OFFICE_MANAGER_H

#include "synch.h"
#include "utilities.h"

#include <stdint.h>
#include <vector>


#define CLERK_WAKEUP_THRESHOLD 3
#define MONEY_REPORT_INTERVAL 5000

class PassportOffice;

void RunPrintMoney(int manager);

class Manager {
 void PrintMoneyReport(int manager);
 public:
  Manager(PassportOffice* passport_office);
  ~Manager();

  inline void set_running(bool running__) { running_ = running__; }

  void PrintMoneyReport();
  void Run();
  void WakeUp();

  void WakeWaitingCustomers();
  void WakeClerksForSenator();

  Condition wakeup_condition_;
  Lock wakeup_condition_lock_;
 private:
  uint32_t elapsed_;
  std::vector<uint32_t> money_;
  PassportOffice* passport_office_;
  bool running_;
};
#endif
