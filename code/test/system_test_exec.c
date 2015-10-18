#include "../userprog/syscall.h"

int main() {
  Print("Starting to exec system test.\n", 30);

  Exec("../test/system_test", 19);
  Exec("../test/system_test", 19);
  Exit(0);
}
