#include "syscall.h"

void fork_function() {
  int a, b;
  Write("Hello from a forked thread!\n", 28, ConsoleOutput);
  Exit(0);
}

int main() {
  char c, d;
  Write("Hello from the main thread!\n", 28, ConsoleOutput);
  Fork(&fork_function);
  Exit(0);
}
