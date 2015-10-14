#include "syscall.h"

void fork_function() {
  Write("Hello from a forked thread!\n", 28, ConsoleOutput);
  Exit(0);
}

int main() {
  Write("Hello from the main thread!\n", 28, ConsoleOutput);
  Fork(fork_function);
  Exit(0);
}
