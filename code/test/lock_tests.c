#include "../userprog/syscall.h"

#define NUM_SYSTEM_LOCKS 100

void test_result(int result);
void create_destroy_lock();
void acquire_release();

int main() {
  create_destroy_lock();
  acquire_release();
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
  locks[0] = CreateLock("Test Lock #1", 12);
  test_result(locks[0] >= 0 && locks[0] < NUM_SYSTEM_LOCKS);

  Write("=== BEGIN CreateLock Too Many Test ===\n", 39, ConsoleOutput);
  for (i = 1; i < NUM_SYSTEM_LOCKS; ++i) {
    locks[i] = CreateLock("Another Test Lock", 17);
  }
  lock = CreateLock("Another Test Lock", 17);
  test_result(lock == -1);

  Write("=== BEGIN DestroyLock Basic Test ===\n", 37, ConsoleOutput);
  result = DestroyLock(locks[0]);
  test_result(result != -1);
  for (i = 1; i < NUM_SYSTEM_LOCKS; ++i) {
    DestroyLock(locks[i]);
  }

  Write("=== BEGIN DestroyLock Invalid Argument Test ===\n", 48, ConsoleOutput);
  result = DestroyLock(-1);
  test_result(result == -1);
  result = DestroyLock(NUM_SYSTEM_LOCKS);
  test_result(result == -1);
  result = DestroyLock(0);
  test_result(result == -1);
}

int lock;
int cv;
int acquireReleaseBasicVariable = 1;
void acquire_release_lock_basic() {
  int result;
  int i;

  result = Acquire(lock);
  test_result(result != -1);

  for (i = 0; i < 10; ++i) {
    acquireReleaseBasicVariable =
        acquireReleaseBasicVariable + acquireReleaseBasicVariable;
  }

  result = Release(lock);
  test_result(result != -1);

  test_result(acquireReleaseBasicVariable == 1 << 20);

  Signal(cv, lock);
  Exit(0);
}

void acquire_release() {
  int result;
  int i;

  lock = CreateLock("Acquire Release Test Lock", 25);
  cv = CreateCondition("Acquire Release Test Condition", 30);

  Write("=== BEGIN Acquire/Release Basic Test ===\n", 41, ConsoleOutput);
  result = Acquire(lock);
  test_result(result != -1);

  Fork(acquire_release_lock_basic);

  for (i = 0; i < 10; ++i) {
    acquireReleaseBasicVariable =
        acquireReleaseBasicVariable + acquireReleaseBasicVariable;
  }

  Wait(cv, lock);
  result = Release(lock);
  test_result(result != -1);

  Write("=== BEGIN Acquire Invalid Argument Test ===\n", 44, ConsoleOutput);
  result = Acquire(-1);
  test_result(result == -1);
  result = Acquire(NUM_SYSTEM_LOCKS);
  test_result(result == -1);
  result = Acquire(1);
  test_result(result == -1);

  Write("=== BEGIN Release Invalid Argument Test ===\n", 44, ConsoleOutput);
  result = Release(-1);
  test_result(result == -1);
  result = Release(NUM_SYSTEM_LOCKS);
  test_result(result == -1);
  result = Release(1);
  test_result(result == -1);

  DestroyLock(lock);
  DestroyCondition(cv);
}

