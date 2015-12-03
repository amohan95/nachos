#include "syscall.h"

int main() {
  int cv, lock, status;
  Print("###### Condition Variable Test2 ######\n", 39);
  Print("Creating Lock. Index returned: ", 31);
  lock = CreateLock("Lock", 4);
  PrintNum(lock);
  Print("\n", 1);
  Print("Creating Condition Variable. Index returned: ", 45);
  cv = CreateCondition("CV", 2);
  Print("Acquiring Lock\n", 15);
  status = Acquire(lock);
  if (status) {
    Print("Successfully Acquired\n", 22);
  } else {
    Print("FAIL: Something went wrong Acquire\n", 35);
  }
  Print("Calling Signal.\n", 16);
  Signal(cv, lock);
  Print("Returned from signal and releasing lock\n", 40);
  Release(lock);
  Exit(0);
}
