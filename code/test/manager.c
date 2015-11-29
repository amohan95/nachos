#include "../userprog/syscall.h"

#define NETWORK

#include "simulation.c"

int main() {
  RunEntity(MANAGER, 0);
  Exit(0);
}
