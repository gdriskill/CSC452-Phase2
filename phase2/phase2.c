#include <stdlib.h>
#include <usloss.h>
#include <usyscall.h>
#include "phase1.h"
#include "phase2.h"

#define MAILBOX_ACTIVE 0
#define MAILBOX_INACTIVE -1
#define MAILBOX_RELEASED -2

#define MAILSLOT_ACTIVE 0
#define MAILSLOT_INACTIVE -1

#define PCB_INACTIVE -1
#define PCB_CONSUMER 1
#define PCB_PRODUCER 2

#define DEBUG 1
#define TRACE 1

typedef struct PCB {
	int id;
	int status;
	struct PCB* next_consumer;
	struct PCB* next_producer;
} PCB;

typedef struct MailSlot {
	int id;
	void* message;
	int msg_size;
        struct MailSlot* next_message;
        int status;
} MailSlot;

typedef struct MailBox {
	int id;
	int status;
	int slots_used;
	int max_slots;
	int slot_size;
	MailSlot* head;
	PCB* consumers;
	PCB* producers;
} MailBox;

static MailBox mailboxes[MAXMBOX];
static MailSlot mailSlots[MAXSLOTS];
PCB shadowTable[MAXPROC];

void (*systemCallVec[MAXSYSCALLS])(USLOSS_Sysargs *args);
int get_mode();
int disable_interrupts();
void restore_interrupts(int old_state);
void enable_interrupts();

/////////////////////////////////////////////////////////////
void  nullsys(USLOSS_Sysargs *args){
	USLOSS_Console("ERROR: Syscall\n");
	USLOSS_Halt(1);
}

int get_next_mailbox_id(){
	for(int i=0; i<MAXMBOX; i++){
		if(mailboxes[i].status == MAILBOX_INACTIVE){
			return i;
		}	
	}
	return -1;
}

int get_next_mailslot_id(){
        for(int i=0; i<MAXSLOTS; i++){
                if(mailSlots[i].status == MAILBOX_INACTIVE){
                        return i;
                }
        }
        return -1;
}

/* Similar to phase1_init. Used to initialize any data structures.
 * Do not fork1 from here
 * 
 * May Block: No
 * May Context Switch: No
 * Args: None
 * Return Value: None
 */
void phase2_init(void) {
	for(int i=0; i<MAXMBOX; i++){
		MailBox mb;
		mb.status = MAILBOX_INACTIVE;
		mailboxes[i] = mb;
	}
	for(int j=0; j<MAXSLOTS; j++){
		MailSlot ms;
		ms.status = MAILSLOT_INACTIVE;
		mailSlots[j] = ms;
	}
	for(int k=0; k<MAXPROC; k++){
		PCB pcb;
		pcb.status = PCB_INACTIVE;
		shadowTable[k] = pcb;
	}
	for(int l=0; l<MAXSYSCALLS; l++){
		systemCallVec[l] = nullsys;	
	}

}

/* Called by Phase1 init, once processes are running but before the testcase begins.
 * Can fork1 from here, but shouldn't be a need to
 * 
 * May Block: No
 * May Context Switch: No
 * Args: None
 * Return Value: None
 */
void phase2_start_service_processes(void) {}


/* Creates a new mailbox
 * 
 * May Block: No
 * May Context Switch: No
 * Args:
 *   numSlots - the maximum number of slots that may be used to queue up messages from this mailbox
 *   slotSize - the largest allowable message that can be sent through this mailbox
 * Return Value:
 *   -1: numSlots or slotSize is negative, or larger than allowed. Or there are no mailboxes available
 *   >= 0: ID of allocated mailbox
 */
int MboxCreate(int slots, int slot_size) {
	if(slots<0 || slot_size<0 || slots>MAXSLOTS || slot_size>MAX_MESSAGE){
		return -1;
	}
	int id = get_next_mailbox_id();

	if(id==-1){
		// Mailbox array is full
		return -1;
	}
	MailBox mb;
	mb.id = id;
	mb.status = MAILBOX_ACTIVE;
	mb.max_slots = slots;
	mb.slot_size = slot_size;
	mb.head = NULL;
	mb.consumers = NULL;
	mb.producers = NULL;
	mailboxes[id] = mb;
	return id;
}


/* Destroys a mailbox. All slots consumed by the mailbox will be freed.
 * 
 * May Block: Yes
 * May Context Switch: Yes
 * Args:
 *   mailboxID - the ID of the mailbox
 * Return Value:
 *   -1: The ID is not a mailbox that is currently in use
 *   0: Success
 */
int MboxRelease(int mbox_id) {
	int old_state = disable_interrupts();	
	if(mailboxes[mbox_id].status == MAILBOX_INACTIVE){
		restore_interrupts(old_state);
		return -1;	
	}
	MailBox mbox = mailboxes[mbox_id];
	mbox.status = MAILBOX_RELEASED;
	MailSlot* slot = mbox.head;
	while(slot!=NULL){
		slot->status = MAILSLOT_INACTIVE;
		slot = slot->next_message;
	}
	restore_interrupts(old_state);
	PCB* consumer = mbox.consumers;
	while(consumer!=NULL){
		unblockProc(consumer->id);
		consumer = consumer->next_consumer;
	}
	PCB* producer = mbox.producers;
        while(producer!=NULL){
                unblockProc(producer->id);
		producer = producer->next_producer;
        }
	mbox.status = MAILBOX_INACTIVE;
	return 0;
}


/* Sends a message through a mailbox. If delivered directly, do not block.
 * If no consumers queued, block until message can be delivered 
 * 
 * May Block: Yes
 * May Context Switch: Yes
 * Args:
 *   mailboxID - the ID of the mailbox
 *   message - pointer to a buffer. If messageSize is nonzero, then this must be non-NULL
 *   messageSize - the length of the message to send
 * Return Value:
 *   -3: the mailbox was released but is not completely invalid yet
 *   -2: The system has run out of global mailbox slots, so the message could not be queued
 *   -1: Illegal values given as arguments including invalid mailbox ID
 *   0: Success
 */
int MboxSend(int mbox_id, void *msg_ptr, int msg_size) {
	// Log process that is sending
	int pid = getpid();
	PCB* process_ptr = &shadowTable[pid%MAXPROC];;
	if (shadowTable[pid%MAXPROC].status == PCB_INACTIVE) {
		process_ptr->id = pid;
		process_ptr->status = PCB_PRODUCER;
		process_ptr->next_consumer = NULL;
		process_ptr->next_producer = NULL;
	}
 
	MailBox mbox = mailboxes[mbox_id];
	if(mbox.status == MAILBOX_RELEASED){
		return -3;	
	}
	if(mbox.status == MAILBOX_INACTIVE || msg_size>MAX_MESSAGE || msg_size<0){
		return -1;
	}
	MailSlot slot;
	int id = get_next_mailslot_id();
	if(id==-1){
		return  -2;
	}
	slot.id = id;
	slot.msg_size = msg_size;
	slot.message = msg_ptr;
	mailSlots[id] = slot;

	// Check if mailbox is full. If so, block process
	if (mbox.slots_used == mbox.max_slots) {
		if (mbox.producers == NULL) {
			mbox.producers = process_ptr;
		} else {
			PCB* cur = mbox.producers;
			while (cur->next_producer != NULL) {
				cur = cur->next_producer;
			}
			cur->next_producer = process_ptr;	
		}
		blockMe(11);	
	}

	if (mbox.head == NULL) {
		mbox.head = &mailSlots[id];
	} else {
		MailSlot* slot_ptr = mbox.head;
		while (slot_ptr->next_message != NULL) {
			slot_ptr = slot_ptr->next_message;
		}
		slot_ptr->next_message = &mailSlots[id];
	}
	
	// ONCE A MESSAGE HAS BEEN CLAIMED, NOTE IT IN A FUTURE FIELD ON MAILSLOT

	// Check if there is a consumer ready
	if (mbox.consumers != NULL) {
		PCB* consumer = mbox.consumers;
		mbox.consumers = consumer->next_consumer;
		unblockProc(consumer->id);
	}

	return 0;
}

/* Waits to receive a message through a mailbox.
 * If message, return it. If not, block until a message arrives
 * 
 * May Block: Yes
 * May Context Switch: Yes
 * Args:
 *   mailboxID - the ID of the mailbox
 *   message - pointer to a buffer. If messageSize is nonzero, then this must be non-NULL
 *   messageSize - the length of the message to send
 * Return Value:
 *   -3: the mailbox was released but is not completely invalid yet
 *   -1: Illegal values given as arguments including invalid mailbox ID
 *   >= 0: The size of the message received
 */
int MboxRecv(int mbox_id, void *msg_ptr, int msg_max_size) {
	return -1;
}

/* Sends a message through a mailbox. 
 * Does not block.
 * 
 * May Block: No
 * May Context Switch: Yes
 * Args:
 *   mailboxID - the ID of the mailbox
 *   message - pointer to a buffer. If messageSize is nonzero, then this must be non-NULL
 *   messageSize - the length of the message to send
 * Return Value:
 *   -3: the mailbox was released but is not completely invalid yet
 *   -2: The system has run out of global mailbox slots, so the message could not be queued
 *   -1: Illegal values given as arguments including invalid mailbox ID
 *   0: Success
 */
int MboxCondSend(int mbox_id, void *msg_ptr, int msg_size) {
	return -1;
}

/* Waits to receive a message through a mailbox.
 * Does not block.
 * 
 * May Block: No
 * May Context Switch: Yes
 * Args:
 *   mailboxID - the ID of the mailbox
 *   message - pointer to a buffer. If messageSize is nonzero, then this must be non-NULL
 *   messageSize - the length of the message to send
 * Return Value:
 *   -3: the mailbox was released but is not completely invalid yet
 *   -1: Illegal values given as arguments including invalid mailbox ID
 *   >= 0: The size of the message received
 */
int MboxCondRecv(int mbox_id, void *msg_ptr, int msg_max_size) {
	return -1;
}

/* Waits for an interrupt to fire on a given device.
 * Only devices allowed: clock (0), disk (0,1), and terminal (0,1,2,3)
 * If outside, throw error and halt
 * 
 * May Block: Yes
 * May Context Switch: Yes
 * Args:
 *   type - the device type: clock, disk, or terminal
 *   unit - which "unit" of a given type you are accessing. NULL pointer is ok
 *   status - an out parameter, which will be used to deliver the device status once an interrupt arrives
 * Return Value: None
 */
void waitDevice(int type, int unit, int *status) {}

/* Wakes up the device
 * 
 * May Block: ?
 * May Context Switch: ?
 * Args:
 *   type - the device type: clock, disk, or terminal
 *   unit - which "unit" of a given type you are accessing. NULL pointer is ok
 *   status - ?
 * Return Value: None
 */
void wakeupByDevice(int type, int unit, int status) {}

/* Called by Phase1 from sentinel, if it ever runs. 
 * 
 * May Block: No
 * May Context Switch: No
 * Args: None
 * Return Value: 
 *   0: No processes are blocked
 *   >= 0: Some processes are currently blocked
 */
int phase2_check_io(void) {
	return -1;
}

/* Clock interrupt handler from Phase1
 * 
 * May Block: No
 * May Context Switch: Yes
 * Args: None
 * Return Value: None 
 */
void phase2_clockHandler(void) {}

/**
Extracts the current mode from the PSR
Returns 1 if it is kernel mode, 0 if it is in user mode
*/
int get_mode(){
	return (USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet());
}

/**
 * Changes the PSR to disable interrupts.
 * 
 * Return: 1 if the interupts were previously enabled, 0 if they were
 * 		disbaled.
 */
int disable_interrupts(){
	int old_state = USLOSS_PSR_CURRENT_INT;
	int result = USLOSS_PsrSet(USLOSS_PsrGet() & ~USLOSS_PSR_CURRENT_INT);
	if(result!=USLOSS_DEV_OK){
                USLOSS_Console("ERROR: Could not set PSR to disable interrupts.\n");	
	}
	return old_state;
}

/**
 * Restores interrupts to the specified old_state. If old_state is 0, interrupts are
 * disabled. If old_state is grearter than 0, interrupts are enabled.
 */
void restore_interrupts(int old_state){
	int result;
	if(old_state>0){
		result = USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);
	}
	else{
		result = USLOSS_PsrSet(USLOSS_PsrGet() & ~USLOSS_PSR_CURRENT_INT);
	}
	if(result!=USLOSS_DEV_OK){
                USLOSS_Console("ERROR: Could not set PSR to restore interrupts.\n");
	}
}

/**
 * Changes the PSR to enable interrupts.
 */
void enable_interrupts(){
	int result = USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);
	if(result!=USLOSS_DEV_OK){
		USLOSS_Console("ERROR: Could not set PSR to enable interrupts.\n");
	}
}

