#include "syscall.h"

void fork_function() {
  int a = 5;
  int b = 3;
  Write("Hello from a forked thread!\n", 28, ConsoleOutput);
  Exit(0);
}

int main() {
  // char c = 'c';
  // char d = 'd';
  // Write("Hello from the main thread!\n", 28, ConsoleOutput);
  // Fork(&fork_function);

  char * c = 'hi';
  Print(c);

  Exit(0);
}
