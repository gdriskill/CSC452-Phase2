PREFIX = ../..

CC = gcc

CSRCS = $(wildcard *.c)
COBJS = $(CSRCS:.c=.o)

LIBS = -lusloss4.7 -lphase1

LIB_DIR     = ${PREFIX}/lib
INCLUDE_DIR = ${PREFIX}/include

CFLAGS = -Wall -g -I${INCLUDE_DIR} -I.
LDFLAGS = -Wl,--start-group -L${LIB_DIR} -L. ${LIBS} -Wl,--end-group



VPATH = testcases
TESTS = test00 test01 test02 test03 test04 test05 test06 test07 test08 test09 \
        test10 test11 test12 test13 test14 test15 test16 test17 test18 test19 \
        test20 test21 test22 test23 test24 test25 test26 test27 test28 test29 \
        test30 test31 test32 test33 test34 test35 test36 test37 test38 test39 \
        test40 test41 test42 test43 test44 test45 test46 test47



all: ${TESTS}

${TESTS}: phase2_common_testcase_code.o $(COBJS) libphase1.a

ARCH=$(shell uname | tr '[:upper:]' '[:lower:]')-$(shell uname -p | sed -e "s/aarch/arm/g")

phase2_messages_no_debug_symbols-${ARCH}.o: phase2_messages.c
	gcc -I${INCLUDE_DIR} -I. -c phase2_messages.c -o phase2_messages_no_debug_symbols-${ARCH}.o

phase2_devices_no_debug_symbols-${ARCH}.o: phase2_devices.c
	gcc -I${INCLUDE_DIR} -I. -c phase2_devices.c -o phase2_devices_no_debug_symbols-${ARCH}.o

phase2_syscall_no_debug_symbols-${ARCH}.o: phase2_syscall.c
	gcc -I${INCLUDE_DIR} -I. -c phase2_syscall.c -o phase2_syscall_no_debug_symbols-${ARCH}.o

libphase2-${ARCH}.a: phase2_messages_no_debug_symbols-${ARCH}.o phase2_devices_no_debug_symbols-${ARCH}.o phase2_syscall_no_debug_symbols-${ARCH}.o
	-rm $@
	ar -r $@ $^

clean:
	-rm *.o ${TESTS} term[0-3].out

