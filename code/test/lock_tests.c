#include "../userprog/syscall.h"

#define NUM_SYSTEM_LOCKS 100
#define NULL 0

void test_result(int result);
void create_destroy_lock();
void acquire_release_lock();

int main() {
  create_destroy_lock();
  acquire_release_lock();
  Exit(0);
}

void test_result(int result) {
  if (result) {
    Write("=== SUCCESS ===\n", 17, ConsoleOutput);
  } else {
    Write("=== FAILURE ===\n", 17, ConsoleOutput);
  }
}

void create_destroy_lock() {
  int locks[NUM_SYSTEM_LOCKS];
  int i;
  int lock;
  int result;

  Write("=== BEGIN CreateLock Basic Test ===\n", 37, ConsoleOutput);
  locks[0] = CreateLock("Test Lock #1");
  test_result(locks[0] >= 0 && locks[0] < NUM_SYSTEM_LOCKS);

  Write("=== BEGIN CreateLock Too Many Test ===\n", 39, ConsoleOutput);
  for (i = 1; i < NUM_SYSTEM_LOCKS; ++i) {
    locks[i] = CreateLock("Another Test Lock");
  }
  lock = CreateLock("Another Test Lock");
  test_result(lock == -1);

  Write("=== BEGIN DestroyLock Basic Test ===\n", 37, ConsoleOutput);
  result = DestroyLock(locks[0]);
  test_result(result != -1);
  for (i = 1; i < NUM_SYSTEM_LOCKS; ++i) {
    DestroyLock(locks[i]);
  }

  Write("=== BEGIN DestroyLock Invalid Argument Test ===\n", 48, ConsoleOutput);
  result = DestroyLock(0);
  test_result(result == -1);
}

void acquire_release_lock() {
}
