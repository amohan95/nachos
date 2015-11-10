#include "syscall.h"

int main() {
  int mv;
  Print("###### Monitor Variable Test ######\n", 36);
  Print("Creating Monitor Variable. Index returned: ", 43);
  mv = CreateMonitor("MV", 2, 2);
  PrintNum(mv);
  Print("\n", 1);
  SetMonitor(mv, 0, 1);
  SetMonitor(mv, 1, 2);
  Print("Should be 1: ", 13);
  PrintNum(GetMonitor(mv, 0));
  Print("\n", 1);
  Print("Should be 2: ", 13);
  PrintNum(GetMonitor(mv, 1));
  Print("\n", 1);
  DestroyMonitor(mv);
  Exit(0);
}