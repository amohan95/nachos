/* testfiles.c
 *	Simple program to test the file handling system calls
 */

#include "syscall.h"

int main() {
  /*OpenFileId fd;
  int bytesread;
  char buf[20];

    Create("testfile", 8);
    fd = Open("testfile", 8);

    Write("testing a write\n", 16, fd );
    Close(fd);


    fd = Open("testfile", 8);
    bytesread = Read( buf, 100, fd );
    Write( buf, bytesread, ConsoleOutput );
    Close(fd);*/
  int status;
  int j;
  int i = CreateLock("Lock", 4);
  PrintNum(i);
  Print("\n", 1);
  j = CreateCondition("CV", 2);
  status = Acquire(i);
  Print("Acquire: ", 9);
  PrintNum(status);
  Print("\n", 1);
  Signal(j, i);
  Print("Returned from signal", 18);
  Release(i);
  Exit(0);
}
