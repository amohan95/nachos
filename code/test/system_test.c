#include "../userprog/syscall.h"

#include "simulation.c"

int main() {
  int index = 0;
  int num_customers = NUM_CUSTOMERS;
  int num_senators = NUM_SENATORS;

  Print("Starting Simulation System Test.\n", 33);

  SetupPassportOffice();
  StartPassportOffice();
  
  while (num_customers + num_senators > 0) {
    if (Rand() % (num_customers + num_senators) >= num_customers) {
      AddNewSenator(index);
      --num_senators;
    } else {
      AddNewCustomer(index);
      --num_customers;
    }
    ++index;
  }

  WaitOnFinish();
}
