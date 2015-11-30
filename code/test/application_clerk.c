#include "../userprog/syscall.h"

#define NETWORK
#include "simulation.h"
int main() {
  int entityId;
  SetupPassportOffice();
  entityId = GetMonitor(clerk_index_[kApplication], 0);
  SetMonitor(clerk_index_[kApplication], 0, entityId + 1);
  RunEntity(APPLICATION_CLERK, entityId);
  Exit(0);
}
#include "simulation.c"
