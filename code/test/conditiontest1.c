#include "syscall.h"

int main() {
  int cv, lock, status;
  Print("###### Condition Variable Test1 ######\n", 39);
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
  Print("Calling Wait and waiting to be signalled by other program.\n", 60);
  Wait(cv, lock);
  Print("Returned from wait and releasing lock\n", 38);
  status = Release(lock);
  if (status) {
    Print("Successfully Released\n", 22);
  } else {
    Print("FAIL: Something went wrong Release\n", 35);
  }
  Print("Destroying Lock\n", 16);
  DestroyLock(lock);
  Print("Destroying Condition Variable\n", 30);
  DestroyCondition(cv);
  Exit(0);
}
