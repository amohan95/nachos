#include "../userprog/syscall.h"

#define NUM_SYSTEM_CONDITIONS 10000

void test_result(int result);
void create_destroy_condition();
void wait_signal_broadcast();

int main() {
  /*create_destroy_condition();*/
  wait_signal_broadcast();
  Exit(0);
}

void test_result(int result) {
  if (result) {
    Write("=== SUCCESS ===\n", 17, ConsoleOutput);
  } else {
    Write("=== FAILURE ===\n", 17, ConsoleOutput);
  }
}

void create_destroy_condition() {
  int cvs[NUM_SYSTEM_CONDITIONS];
  int i;
  int cv;
  int result;

  Write("=== BEGIN CreateCondition Basic Test ===\n", 42, ConsoleOutput);
  cvs[0] = CreateCondition("Test Condition #1", 17);
  test_result(cvs[0] >= 0 && cvs[0] < NUM_SYSTEM_CONDITIONS);

  Write("=== BEGIN CreateCondition Too Many Test ===\n", 44, ConsoleOutput);
  for (i = 1; i < NUM_SYSTEM_CONDITIONS; ++i) {
    cvs[i] = CreateCondition("Another Test Condition", 22);
  }
  cv = CreateCondition("Another Test Condition", 22);
  test_result(cv == -1);

  Write("=== BEGIN DestroyCondition Basic Test ===\n", 42, ConsoleOutput);
  result = DestroyCondition(cvs[0]);
  test_result(result != -1);
  for (i = 1; i < NUM_SYSTEM_CONDITIONS; ++i) {
    DestroyCondition(cvs[i]);
  }

  Write("=== BEGIN DestroyCondition Invalid Argument Test ===\n", 53, ConsoleOutput);
  result = DestroyCondition(-1);
  test_result(result == -1);
  result = DestroyCondition(NUM_SYSTEM_CONDITIONS);
  test_result(result == -1);
  result = DestroyCondition(0);
  test_result(result == -1);
}


int forkCV;
int forkLock;
void wait_signal_basic() {
  Acquire(forkLock);
  Write("Forked thread signalling main thread\n", 37, ConsoleOutput);
  Signal(forkCV, forkLock);
  Release(forkLock);
  Exit(0);
}

int broadcastCV;
int broadcastLock;
int forkedThreads = 0;
void wait_broadcast_basic() {
  Acquire(forkLock);
  ++forkedThreads;
  Signal(forkCV, forkLock);
  Acquire(broadcastLock);
  Release(forkLock);
  Wait(broadcastCV, broadcastLock);
  Write("Forked thread signalled from main thread\n", 41, ConsoleOutput);
  Release(broadcastLock);
  Acquire(forkLock);
  if (--forkedThreads == 0) {
    Signal(forkCV, forkLock);
  }
  Release(forkLock);
  Exit(0);
}

void wait_signal_broadcast() {
  int result;
  int i;

  forkCV = CreateCondition("Wait Signal Broadcast Test Fork Condition", 41);
  broadcastCV = CreateCondition("Wait Signal Broadcast Test Condition", 36);
  forkLock = CreateLock("Wait Signal Broadcast Test Fork Lock", 36);
  broadcastLock = CreateLock("Wait Signal Broadcast Test Lock", 31);

  Write("=== BEGIN Wait/Signal Basic Test ===\n", 37, ConsoleOutput);
  Acquire(forkLock);
  Fork(wait_signal_basic);
  Write("Main thread waiting for forked thread to signal\n", 48, ConsoleOutput);
  Wait(forkCV, forkLock);
  Write("Main thread signalled from forked thread\n", 41, ConsoleOutput);

  Write("=== BEGIN Wait/Broadcast Basic Test ===\n", 40, ConsoleOutput);
  Write("Main thread forking 3 child threads\n", 36, ConsoleOutput);
  for (i = 0; i < 3; ++i) {
    Fork(wait_broadcast_basic);
    Wait(forkCV, forkLock);
  }
  Acquire(broadcastLock);
  Broadcast(broadcastCV, broadcastLock);
  Acquire(forkLock);
  Release(broadcastLock);
  Wait(forkCV, forkLock);
  Release(forkLock);

  DestroyLock(broadcastLock);
  DestroyCondition(broadcastCV);

  Write("=== BEGIN Wait Invalid Argument Test ===\n", 41, ConsoleOutput);
  result = Wait(-1, forkLock);
  test_result(result == -1);
  result = Wait(forkCV, -1);
  test_result(result == -1);
  result = Wait(NUM_SYSTEM_CONDITIONS, forkLock);
  test_result(result == -1);
  result = Wait(forkCV, NUM_SYSTEM_CONDITIONS);
  test_result(result == -1);

  Write("=== BEGIN Signal Invalid Argument Test ===\n", 42, ConsoleOutput);
  result = Signal(-1, forkLock);
  test_result(result == -1);
  result = Signal(forkCV, -1);
  test_result(result == -1);
  result = Signal(NUM_SYSTEM_CONDITIONS, forkLock);
  test_result(result == -1);
  result = Signal(forkCV, NUM_SYSTEM_CONDITIONS);
  test_result(result == -1);

  Write("=== BEGIN Broadcast Invalid Argument Test ===\n", 46, ConsoleOutput);
  result = Broadcast(-1, forkLock);
  test_result(result == -1);
  result = Broadcast(forkCV, -1);
  test_result(result == -1);
  result = Broadcast(NUM_SYSTEM_CONDITIONS, forkLock);
  test_result(result == -1);
  result = Broadcast(forkCV, NUM_SYSTEM_CONDITIONS);
  test_result(result == -1);

  DestroyLock(forkLock);
  DestroyCondition(forkCV);

  Write("=== BEGIN Wait/Signal/Broadcast Use After Destroyed Test ===\n", 61, ConsoleOutput);
  result = Wait(forkCV, forkLock);
  test_result(result == -1);
  result = Signal(forkCV, forkLock);
  test_result(result == -1);
  result = Broadcast(forkCV, forkLock);
  test_result(result == -1);
}
