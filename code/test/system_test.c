#include "../userprog/syscall.h"

#define NUM_CUSTOMERS 15
#define NUM_SENATORS 0
#define NUM_APPLICATION_CLERKS 3
#define NUM_PICTURE_CLERKS 3
#define NUM_PASSPORT_CLERKS 3
#define NUM_CASHIER_CLERKS 3
#define NUM_TOTAL_CLERKS NUM_APPLICATION_CLERKS + NUM_PICTURE_CLERKS + NUM_PASSPORT_CLERKS + NUM_CASHIER_CLERKS

#include "simulation.c"

int main() {
  SetupPassportOffice();
  StartPassportOffice();
}
