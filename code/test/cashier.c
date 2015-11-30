#include "../userprog/syscall.h"

#define NETWORK
#include "simulation.h"
int main() {
  int entityId;
  SetupPassportOffice();
  entityId = GetMonitor(clerk_index_[kCashier], 0);
  SetMonitor(clerk_index_[kCashier], 0, entityId + 1);
  RunEntity(CASHIER_CLERK, entityId);
  Exit(0);
}
#include "simulation.c"
