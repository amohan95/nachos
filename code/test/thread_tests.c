#include "../userprog/syscall.h"

int shared = 0;
int shared_lock;

void fork_function_0() {
  int a = 5;
  int b = 3;
  Write("Hello from a forked function #0!\n", 33, ConsoleOutput);
  Acquire(shared_lock);
  Print("Shared data value: ", 19);
  PrintNum(++shared);
  Print("\n", 1);
  Release(shared_lock);
  Exit(0);
}

void fork_function_1() {
  int c = 3;
  char f = 'f';
  Write("Hello from forked function #1!\n", 31, ConsoleOutput);
  Acquire(shared_lock);
  Print("Shared data value: ", 19);
  PrintNum(++shared);
  Print("\n", 1);
  Release(shared_lock);
  Exit(0);
}

int main() {
  char a = 'a';
  char b = 'b';
  int i;

  shared_lock = CreateLock("Shared Lock", 11);

  Write("Hello from the main thread!\n", 28, ConsoleOutput);
  Print("Shared data at the beginning: ", 30);
  PrintNum(shared);
  Print("\n", 1);

  Fork(fork_function_0);
  Fork(fork_function_1);

  Exit(0);
}
