#include "syscall.h"

int main() {
  /* Print("hello\n", 6);
  // Print("hello\n", 5);
  // Print("1", 1);
  // Print("\n", 1);
  // PrintNum(3);
  // Print("\n", 1);
  // PrintNum(1000);
  // Print("\n", 1);
  // Exit(0);*/

  /* int j, status;
  // int i = CreateLock("Lock", 4);
  // PrintNum(i);
  // Print("\n", 1);
  // status = Acquire(i);
  // Print("Acquire: ", 9);
  // PrintNum(status);
  // Print("\n", 1);
  // j = CreateCondition("CV", 2);
  // Wait(j, i);
  // Print("Returned from wait\n", 19);
  // Release(i);
  // Exit(0);*/

  int mv;
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
