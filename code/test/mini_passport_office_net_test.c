#include "../userprog/syscall.h"

int main() {
  Exec("../test/application_clerk", 25);
  Exec("../test/picture_clerk", 21);
  Exec("../test/passport_clerk", 22);
  Exec("../test/cashier", 15);
  Exec("../test/manager", 15);
  Exec("../test/customer", 16);
  Exit(0);
}
