#include "../userprog/syscall.h"

int main() {
  Exec("../test/create_entity 2 0", 25);
  Exec("../test/create_entity 3 0", 25);
  Exec("../test/create_entity 4 0", 25);
  Exec("../test/create_entity 5 0", 25);
  Exec("../test/create_entity 6 0", 25);
  Exec("../test/create_entity 0 0", 25);
  Exec("../test/create_entity 0 1", 25);
  Exec("../test/create_entity 0 2", 25);
  Exec("../test/create_entity 0 3", 25);
  Exit(0);
}
