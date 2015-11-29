#include "../userprog/syscall.h"

int main() {
	char buf[64];
	int len = Sprintf(buf, "$%d$\n", 5, 12345);
	Print(buf, len);
	Exit(0);
}
