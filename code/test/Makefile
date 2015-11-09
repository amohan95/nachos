# use normal make for this Makefile
#
# Makefile for building user programs to run on top of Nachos
#
# Several things to be aware of:
#
#    Nachos assumes that the location of the program startup routine (the
# 	location the kernel jumps to when the program initially starts up)
#       is at location 0.  This means: start.o must be the first .o passed 
# 	to ld, in order for the routine "Start" to be loaded at location 0
#

# If you are running on SunOS 5.x machines, uncomment the next line
GCCDIR = ../gnu/
LDFLAGS = -T newscript -N
ASFLAGS = -mips1
CPPFLAGS = $(INCDIR)

# if you aren't cross-compiling:
# GCCDIR =
# LDFLAGS = -N -T 0
# ASFLAGS =
# CPPFLAGS = -P $(INCDIR)


CC = $(GCCDIR)gcc
AS = $(GCCDIR)as
LD = $(GCCDIR)ld
CPP = $(GCCDIR)cpp

INCDIR =-I../userprog -I../threads
CFLAGS = -G 0 -c $(INCDIR) 

all: halt matmult sort testfiles thread_tests exec_test lock_tests \
		 condition_tests system_test system_test_exec

start.o: start.s ../userprog/syscall.h
	$(CPP) $(CPPFLAGS) start.s > strt.s
	$(AS) $(ASFLAGS) -o start.o strt.s
	rm strt.s

halt.o: halt.c
	$(CC) $(CFLAGS) -c halt.c
halt: halt.o start.o
	$(LD) $(LDFLAGS) start.o halt.o -o halt.coff
	../bin/coff2noff halt.coff halt

sort.o: sort.c
	$(CC) $(CFLAGS) -c sort.c
sort: sort.o start.o
	$(LD) $(LDFLAGS) start.o sort.o -o sort.coff
	../bin/coff2noff sort.coff sort

matmult.o: matmult.c
	$(CC) $(CFLAGS) -c matmult.c
matmult: matmult.o start.o
	$(LD) $(LDFLAGS) start.o matmult.o -o matmult.coff
	../bin/coff2noff matmult.coff matmult

testfiles.o: testfiles.c
	$(CC) $(CFLAGS) -c testfiles.c
testfiles: testfiles.o start.o
	$(LD) $(LDFLAGS) start.o testfiles.o -o testfiles.coff
	../bin/coff2noff testfiles.coff testfiles

thread_tests.o: thread_tests.c
	$(CC) $(CFLAGS) -c thread_tests.c
thread_tests: thread_tests.o start.o
	$(LD) $(LDFLAGS) start.o thread_tests.o -o thread_tests.coff
	../bin/coff2noff thread_tests.coff thread_tests

exec_test.o: exec_test.c
	$(CC) $(CFLAGS) -c exec_test.c
exec_test: exec_test.o start.o
	$(LD) $(LDFLAGS) start.o exec_test.o -o exec_test.coff
	../bin/coff2noff exec_test.coff exec_test

lock_tests.o: lock_tests.c
	$(CC) $(CFLAGS) -c $^
lock_tests.coff: lock_tests.o start.o
	$(LD) $(LDFLAGS) $^ -o $@
lock_tests: lock_tests.coff
	../bin/coff2noff $< $@

condition_tests.o: condition_tests.c
	$(CC) $(CFLAGS) -c $^
condition_tests.coff: condition_tests.o start.o
	$(LD) $(LDFLAGS) $^ -o $@
condition_tests: condition_tests.coff
	../bin/coff2noff $< $@

system_test.o: system_test.c simulation.c
	$(CC) $(CFLAGS) -c system_test.c
system_test: system_test.o start.o
	$(LD) $(LDFLAGS) start.o system_test.o -o system_test.coff
	../bin/coff2noff system_test.coff system_test

system_test_exec.o: system_test_exec.c simulation.c
	$(CC) $(CFLAGS) -c system_test_exec.c
system_test_exec: system_test_exec.o start.o
	$(LD) $(LDFLAGS) start.o system_test_exec.o -o system_test_exec.coff
	../bin/coff2noff system_test_exec.coff system_test_exec

exec_test.o: print_tests.c
	$(CC) $(CFLAGS) -c print_tests.c
exec_test: print_tests.o start.o
	$(LD) $(LDFLAGS) start.o print_tests.o -o print_tests.coff
	../bin/coff2noff print_tests.coff print_tests

clean:
	rm -f *.o *.coff
