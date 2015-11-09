#include "syscall.h"

int main() {
	char a = 1;
	char b = 2;
  Exec("../test/matmult", 15);
  Exec("../test/matmult", 15);
  Exit(0);
}
