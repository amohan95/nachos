#include "../userprog/syscall.h"

#define NETWORK

#include "simulation.c"

/*int main() {
  int entityId;
  Exit(1);
  Print("A\n", 2);
  entityId = GetMonitor(customer_index_, 0);
  Print("B\n", 2);
  SetMonitor(customer_index_, 0, entityId + 1);
  Print("C\n", 2);
  RunEntity(CUSTOMER, entityId);
  Exit(0);
}*/
int main() {
  Print("Hello World!\n", 13);
  Exit(0);
}
