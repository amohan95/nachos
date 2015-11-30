#include "../userprog/syscall.h"

#define NETWORK
#include "simulation.h"
int main() {
	SetupPassportOffice();
  RunEntity(MANAGER, 0);
  Exit(0);
}
#include "simulation.c"
