#include <stdlib.h>
#include <stdbool.h>
#include <usloss.h>
#include <usyscall.h>
#include <string.h>
#include "phase1.h"
#include "phase2.h"

#define MAILBOX_ACTIVE    0
#define MAILBOX_INACTIVE -1
#define MAILBOX_RELEASED -2

#define MAILSLOT_INACTIVE     -1
#define MAILSLOT_ACTIVE        0
#define MAILSLOT_MSG_UNCLAIMED 1
#define MAILSLOT_MSG_CLAIMED   2


#define PCB_INACTIVE -1
#define PCB_CONSUMER  1
#define PCB_PRODUCER  2

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
	PCB* claimant;
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
	if (TRACE)
		USLOSS_Console("TRACE: In MboxCreate\n");
	if (DEBUG)
		USLOSS_Console("DEBUG: Slots: %d, Slot size: %d\n", slots, slot_size);

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

	if (DEBUG)
		USLOSS_Console("DEBUG: Mailbox created for id:%d\n", id);

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
	if (TRACE)
		USLOSS_Console("TRACE: In MboxRelease (id:%d)\n", mbox_id);

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

	while (consumer!=NULL) {
		if (DEBUG)
			USLOSS_Console("DEBUG: Unblocking consumer (%d)\n", consumer->id);
		unblockProc(consumer->id);
		consumer = consumer->next_consumer;
	}
	
	PCB* producer = mbox.producers;
	while (producer!=NULL) {
		if (DEBUG)
			USLOSS_Console("DEBUG: Unblocking producer (%d)\n", producer->id);
		unblockProc(producer->id);
		producer = producer->next_producer;
	}

	mbox.status = MAILBOX_INACTIVE;
	return 0;
}


/* Sends a message through a mailbox. If delivered directly, do not block.
 * If no consumers queued AND no space for message, block until message can be delivered 
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
	if (TRACE)
		USLOSS_Console("TRACE: In MboxSend (id:%d) \n", mbox_id);

	// Log process that is sending
	int pid = getpid();
	PCB* process_ptr = &shadowTable[pid%MAXPROC];;
	if (shadowTable[pid%MAXPROC].status == PCB_INACTIVE) {
		process_ptr->id = pid;
		process_ptr->status = PCB_PRODUCER;
		process_ptr->next_consumer = NULL;
		process_ptr->next_producer = NULL;
	}
 
	MailBox* mbox_ptr = &mailboxes[mbox_id];
	if(mbox_ptr->status == MAILBOX_RELEASED){
		return -3;	
	}
	if(mbox_ptr->status == MAILBOX_INACTIVE || msg_size>MAX_MESSAGE || msg_size<0){
		return -1;
	}

	// Initializing the slot
	MailSlot slot;
	int id = get_next_mailslot_id();
	if(id==-1){
		return -2;
	}
	slot.id = id;
	slot.msg_size = msg_size;
	slot.message = msg_ptr;
	slot.claimant = NULL;
	mailSlots[id%MAXSLOTS] = slot;

	// Check if mailbox is full. If so, place into the producer queue and block process
	// If not full, producer is done and take off of shadow table
	if (mbox_ptr->slots_used == mbox_ptr->max_slots) {
		if (mbox_ptr->producers == NULL) {
			mbox_ptr->producers = process_ptr;
		} else {
			PCB* cur = mbox_ptr->producers;
			while (cur->next_producer != NULL) {
				cur = cur->next_producer;
			}
			cur->next_producer = process_ptr;	
		}
		blockMe(11);	
	} else {
		shadowTable[pid%MAXPROC].status = PCB_INACTIVE;
	} 

	if (mbox_ptr->head == NULL) {
		mbox_ptr->head = &mailSlots[id];
	} else {
		MailSlot* slot_ptr = mbox_ptr->head;
		while (slot_ptr->next_message != NULL) {
			slot_ptr = slot_ptr->next_message;
		}
		slot_ptr->next_message = &mailSlots[id];
	}
	
	// Check if there is a consumer ready
	if (mbox_ptr->consumers != NULL) {
		mailSlots[id].status = MAILSLOT_MSG_CLAIMED; 
		mailSlots[id].claimant = mbox_ptr->consumers;
	} else {
		mailSlots[id].status = MAILSLOT_MSG_UNCLAIMED;
	}

	return 0;
}

/* Receives the message. 
 * If a message has been claimed and the pid matches, success. 
 * If a message has not been claimed and a process comes in, success. Otherwise, block
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
	if (TRACE)
		USLOSS_Console("TRACE: In MboxRecv (id:%d)\n", mbox_id);

	MailBox* mbox_ptr = &mailboxes[mbox_id%MAXMBOX];
	if (mbox_ptr->status == MAILBOX_RELEASED) {
		USLOSS_Console("ERROR: Mailbox (%d) has already been released\n", mbox_id);
		return -3;
	}
	if (mbox_ptr->status == MAILBOX_INACTIVE) {
		USLOSS_Console("ERROR: Mailbox (%d) is inactive\n", mbox_id);
		return -1;
	}
	if (msg_ptr == NULL || msg_max_size < 0) {
		USLOSS_Console("ERROR: Invalid parameters\n");
		return -1;
	}

	MailSlot* slot_ptr = mbox_ptr->head;
	// Check for receiver
	// If unmarked, then first consumer gets it
	// If reservered, then check for the consumer in the queue
	// If no messages for the calling receiver, block
	bool msg_received = false;
	PCB* consumer = mbox_ptr->consumers;
	while (!msg_received) {
		if (slot_ptr->status == MAILSLOT_MSG_UNCLAIMED) {
			slot_ptr->claimant = consumer;
			slot_ptr->status = MAILSLOT_MSG_CLAIMED;
			msg_received = true;
		} else {
			while (consumer != NULL && !msg_received) {
				if (slot_ptr->claimant == consumer) {
					slot_ptr->status = MAILSLOT_MSG_CLAIMED;
					msg_received = true;
				} else {
					consumer = consumer->next_consumer;
				}
			}
		} 

		// No consumer was found for the claimed message (should this happen?)
		if (!msg_received) {
			blockMe(12);
		}
		consumer = mbox_ptr->consumers;
	}

	// Consume the message by copying it onto the provided pointer
	int msg_size = msg_max_size < slot_ptr->msg_size ? 
		msg_max_size : slot_ptr->msg_size;
	memcpy(msg_ptr, slot_ptr->message, msg_size);
	mbox_ptr->head = slot_ptr->next_message;

	// Take consumer off the queue, if it's on one
	if (mbox_ptr->consumers != NULL) {
		if (consumer == mbox_ptr->consumers) {
			mbox_ptr->consumers = consumer->next_consumer;
		} else {
			PCB* prev_consumer = mbox_ptr->consumers;
			while (consumer != prev_consumer->next_consumer) {
				prev_consumer = prev_consumer->next_consumer;
			}
			prev_consumer->next_consumer = consumer->next_consumer;
		}
	}

	return msg_size;
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

