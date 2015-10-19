#include "../userprog/syscall.h"

#define NUM_CUSTOMERS 15
#define NUM_SENATORS 3
#define NUM_APPLICATION_CLERKS 3
#define NUM_PICTURE_CLERKS 3
#define NUM_PASSPORT_CLERKS 3
#define NUM_CASHIER_CLERKS 3
#define NUM_TOTAL_CLERKS NUM_APPLICATION_CLERKS + NUM_PICTURE_CLERKS + NUM_PASSPORT_CLERKS + NUM_CASHIER_CLERKS

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
      AddNewSenator(CreateCustomer(kSenator, 0), index);
      --num_senators;
    } else {
      AddNewCustomer(CreateCustomer(kCustomer, 0), index);
      --num_customers;
    }
    ++index;
  }

  WaitOnFinish();
}
