#include "../userprog/syscall.h"

int main() {
	int i;
	Print("Starting to exec system test.\n", 30);

	for (i = 0; i < 3; ++i) {
		Exec("../test/system_test", 19);
	}
 
  Exit(0);
}
