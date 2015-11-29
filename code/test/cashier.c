#include "../userprog/syscall.h"

#define NETWORK

#include "simulation.c"

int main() {
  int entityId;
  entityId = GetMonitor(clerk_index_[kCashier], 0);
  SetMonitor(clerk_index_[kCashier], 0, entityId + 1);
  RunEntity(CASHIER_CLERK, entityId);
  Exit(0);
}
