/*
 * These are the definitions for phase1 of the project (the kernel).
 */

#ifndef _PHASE1_H
#define _PHASE1_H

#include <usloss.h>

/*
 * Maximum number of processes. 
 */

#define MAXPROC      50

/*
 * Maximum length of a process name
 */

#define MAXNAME      50

/*
 * Maximum length of string argument passed to a newly created process
 */

#define MAXARG       100

/*
 * Maximum number of syscalls.
 */

#define MAXSYSCALLS  50


/* 
 * These functions must be provided by Phase 1.
 */

extern void phase1_init(void);
extern int  fork1(char *name, int(*func)(char *), char *arg,
                  int stacksize, int priority);
extern int  join(int *status);
extern void quit(int status) __attribute__((__noreturn__));
extern void zap(int pid);
extern int  isZapped(void);
extern int  getpid(void);
extern void dumpProcesses(void);
extern void blockMe(int block_status);
extern int  unblockProc(int pid);
extern int  readCurStartTime(void);
extern void timeSlice(void);
extern int  readtime(void);
extern int  currentTime(void);
extern void startProcesses(void);    // never returns!


/*
 * These functions are called *BY* Phase 1 code, and are implemented in
 * Phase 5.  If we are testing code before Phase 5 is written, then the
 * testcase must provide a NOP implementation of each.
 */

extern void mmu_init_proc(int pid);
extern void mmu_quit(int pid);
extern void mmu_flush(void);

/* these functions are also called by the phase 1 code, from inside
 * init_main().  They are called first; after they return, init()
 * enters an infinite loop, just join()ing with children forever.
 *
 * In early phases, these are provided (as NOPs) by the testcase.
 */
extern void phase2_start_service_processes(void);
extern void phase3_start_service_processes(void);
extern void phase4_start_service_processes(void);
extern void phase5_start_service_processes(void);

/* this function is called by the init process, after the service
 * processes are running, to start whatever processes the testcase
 * wants to run.  This may call fork() or spawn() many times, and
 * block as long as you want.  When it returns, Halt() will be
 * called by the Phase 1 code (nonzero means error).
 */
extern int testcase_main(void);

/* this is called by the clock handler (which is in Phase 1), so that Phase 2
 * can add new clock features
 */
extern void phase2_clockHandler(void);

/* this is called by sentinel to ask if there are any ongoing I/O operations */
extern int phase2_check_io(void);



#endif /* _PHASE1_H */
