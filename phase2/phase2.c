#include <usloss.h>
#include <usyscall.h>
#include "phase1.h"

void phase2_init(void) {}

// returns id of mailbox, or -1 if no more mailboxes, or -1 if invalid args
int MboxCreate(int slots, int slot_size) {
	return -1;
}

// returns 0 if successful, -1 if invalid arg
int MboxRelease(int mbox_id) {
	return -1;
}

// returns 0 if successful, -1 if invalid args
int MboxSend(int mbox_id, void *msg_ptr, int msg_size) {
	return -1;
}

// returns size of received msg if successful, -1 if invalid args
int MboxRecv(int mbox_id, void *msg_ptr, int msg_max_size) {
	return -1;
}

// returns 0 if successful, 1 if mailbox full, -1 if illegal args
int MboxCondSend(int mbox_id, void *msg_ptr, int msg_size) {
	return -1;
}

// returns 0 if successful, 1 if no msg available, -1 if illegal args
int MboxCondRecv(int mbox_id, void *msg_ptr, int msg_max_size) {
	return -1;
}

// type = interrupt device type, unit = # of device (when more than one),
// status = where interrupt handler puts device's status register.
void waitDevice(int type, int unit, int *status) {}
void wakeupByDevice(int type, int unit, int status) {}

// 
//void (*systemCallVec[])(USLOSS_Sysargs *args) {}

void phase2_start_service_processes(void) {}

int phase2_check_io(void) {
	return -1;
}

void phase2_clockHandler(void) {}
