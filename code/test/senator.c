#include "../userprog/syscall.h"

#define NETWORK
#include "simulation.h"
int main() {
  int entityId;
	SetupPassportOffice();
  Acquire(customer_count_lock_);
  entityId = GetMonitor(customer_index_, 0);
  SetMonitor(customer_index_, 0, entityId + 1);
  Release(customer_count_lock_);
	customers_[entityId].type_ = kSenator;
  RunEntity(SENATOR, entityId);
  Exit(0);
}
#include "simulation.c"
