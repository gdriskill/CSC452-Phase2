/*
 * These are the definitions for phase 2 of the project
 */

#ifndef _PHASE2_H
#define _PHASE2_H

#include <usyscall.h>

// Maximum line length. Used by terminal read and write.
#define MAXLINE         80

#define MAXMBOX         2000
#define MAXSLOTS        2500
#define MAX_MESSAGE     150  // largest possible message in a single slot



extern void phase2_init(void);

// returns id of mailbox, or -1 if no more mailboxes, or -1 if invalid args
extern int MboxCreate(int slots, int slot_size);

// returns 0 if successful, -1 if invalid arg
extern int MboxRelease(int mbox_id);

// returns 0 if successful, -1 if invalid args
extern int MboxSend(int mbox_id, void *msg_ptr, int msg_size);

// returns size of received msg if successful, -1 if invalid args
extern int MboxRecv(int mbox_id, void *msg_ptr, int msg_max_size);

// returns 0 if successful, 1 if mailbox full, -1 if illegal args
extern int MboxCondSend(int mbox_id, void *msg_ptr, int msg_size);

// returns 0 if successful, 1 if no msg available, -1 if illegal args
extern int MboxCondRecv(int mbox_id, void *msg_ptr, int msg_max_size);

// type = interrupt device type, unit = # of device (when more than one),
// status = where interrupt handler puts device's status register.
extern void     waitDevice(int type, int unit, int *status);
extern void wakeupByDevice(int type, int unit, int status);

// 
extern void (*systemCallVec[])(USLOSS_Sysargs *args);

#endif
