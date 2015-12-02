#include "../userprog/syscall.h"

#define NETWORK
#include "simulation.h"
int main() {
  int entityId;
  SetupPassportOffice();
  Acquire(cashier_clerk_init_lock_);
  entityId = GetMonitor(clerk_index_[kCashier], 0);
  SetMonitor(clerk_index_[kCashier], 0, entityId + 1);
  Release(cashier_clerk_init_lock_);
  RunEntity(CASHIER_CLERK, entityId);
  Exit(0);
}
#include "simulation.c"
