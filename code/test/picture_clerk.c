#include "../userprog/syscall.h"

#define NETWORK
#include "simulation.h"
int main() {
  int entityId;
  SetupPassportOffice();
  Acquire(picture_clerk_init_lock_);
  entityId = GetMonitor(clerk_index_[kPicture], 0);
  SetMonitor(clerk_index_[kPicture], 0, entityId + 1);
  Release(picture_clerk_init_lock_);
  RunEntity(PICTURE_CLERK, entityId);
  Exit(0);
}
#include "simulation.c"
