#include "../userprog/syscall.h"

void fork_function_0() {
  int a = 5;
  int b = 3;
  Write("Hello from a forked thread #0!\n", 32, ConsoleOutput);
  Exit(0);
}

void fork_function_1() {
  int c = 3;
  char f = 'f';
  Write("Hello from forked thread #1!\n", 30, ConsoleOutput);
  Exit(0);
}

int main() {
  char * c = 'hi';
  Print(c);

  /*char a = 'a';
  char b = 'b';
  int i;

  Write("Hello from the main thread!\n", 28, ConsoleOutput);
  
  for (i = 0; i < 5; ++i) {
    if (Rand() % 2) {
      Fork(fork_function_1);
    } else {
      Fork(fork_function_0);
    }
  }*/

  Exit(0);
}
