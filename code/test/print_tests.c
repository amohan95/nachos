#include "syscall.h"

int main() {
  Print("hello\n", 6);
  Print("hello\n", 5);
  Print("1", 1);
  Print("\n", 1);
  PrintNum(3);
  Print("\n", 1);
  PrintNum(1000);
  Print("\n", 1);
  Exit(0);
}
