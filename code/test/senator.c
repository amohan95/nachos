#include "../userprog/syscall.h"

#define NETWORK
#include "simulation.h"
int main() {
  int entityId;
  entityId = GetMonitor(customer_index_, 0);
  SetMonitor(customer_index_, 0, entityId + 1);
  RunEntity(SENATOR, entityId);
  Exit(0);
}
#include "simulation.c"
