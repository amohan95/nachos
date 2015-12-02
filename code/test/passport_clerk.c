#include "../userprog/syscall.h"

#define NETWORK
#include "simulation.h"
int main() {
  int entityId;
  SetupPassportOffice();
  Acquire(passport_clerk_init_lock_);
  entityId = GetMonitor(clerk_index_[kPassport], 0);
  SetMonitor(clerk_index_[kPassport], 0, entityId + 1);
  Release(passport_clerk_init_lock_);
  RunEntity(PASSPORT_CLERK, entityId);
  Exit(0);
}
#include "simulation.c"
