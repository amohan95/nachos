#include "syscall.h"

int main() {
  char a = 'a';
  char b = 'b';
  Write("Testing Exec syscall by calling thread_tests.\n", 46, ConsoleOutput);
  Exec("../test/thread_tests", 20);
  Exec("../test/thread_tests", 20);  
  Exit(0);
}
