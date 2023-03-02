#include <stdlib.h>
#include <stdbool.h>
#include <usloss.h>
#include <usyscall.h>
#include <string.h>
#include <assert.h>
#include "phase1.h"
#include "phase2.h"

#define MAILBOX_ACTIVE_ZERO  0
#define MAILBOX_ACTIVE       1
#define MAILBOX_INACTIVE    -1
#define MAILBOX_RELEASED    -2

#define MAILBOX_CLOCK     0
#define MAILBOX_DISK1     1
#define MAILBOX_DISK2     2
#define MAILBOX_TERM1     3
#define MAILBOX_TERM2     4
#define MAILBOX_TERM3     5
#define MAILBOX_TERM4     6

#define MAILSLOT_INACTIVE     -1
#define MAILSLOT_ACTIVE        0
#define MAILSLOT_MSG_UNCLAIMED 1
#define MAILSLOT_MSG_CLAIMED   2

#define PCB_INACTIVE             -1
#define PCB_CONSUMER              1
#define PCB_CONSUMER_HAS_MESSAGE  2
#define PCB_PRODUCER              3

#define UNBLOCKED                   1
#define BLOCK_WAITING_ON_CONSUMER  14
#define BLOCK_WAITING_ON_PRODUCER  15

#define DEBUG 0
#define TRACE 0 

typedef struct PCB {
	int id;
	int status;
	int result;
	char message [MAX_MESSAGE];
	int msg_size;
	struct PCB* next_consumer;
	struct PCB* next_producer;
} PCB;

typedef struct MailSlot {
	int id;
	char message [MAX_MESSAGE];
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
int last_clock_send;
int blocked_waitDevice;

void join_consumer_queue(int mbox_id, PCB* consumer);
void leave_consumer_queue(int mbox_id, PCB* consumer);
void join_producer_queue(int mbox_id, PCB* producer);
void leave_producer_queue(int mbox_id, PCB* producer);

void (*systemCallVec[MAXSYSCALLS])(USLOSS_Sysargs *args);
int get_mode();
int disable_interrupts();
void restore_interrupts(int old_state);
void enable_interrupts();
static void disk_handler(int type, void* unit);
static void term_handler(int type, void* unit);
static void syscall_handler(int type, void* unit);

int Send(int mbox_id, void *msg_ptr, int msg_size, bool block_on_fail);
int Recv(int mbox_id, void *msg_ptr, int msg_size, bool block_on_fail);
int put_in_slot(int mbox_id, void* msg_ptr, int  msg_size);

// Helper Funcs
/////////////////////////////////////////////////////////////
void nullsys(USLOSS_Sysargs *args){
	USLOSS_Console("nullsys(): Program called an unimplemented syscall.  syscall no: %d   PSR: %#04x\n",
		args->number, USLOSS_PsrGet());
	USLOSS_Halt(1);
}

int get_next_mailbox_id(){
	for(int i=0; i<MAXMBOX; i++){
		int status = mailboxes[i].status;
		if(status == MAILBOX_INACTIVE || status == MAILBOX_RELEASED){
			return i;
		}	
	}
	return -1;
}

int get_next_mailslot_id(){
	for(int i=0; i<MAXSLOTS; i++){
		if(mailSlots[i].status == MAILSLOT_INACTIVE){
			return i;
		}
	}
	return -1;
}

void join_consumer_queue(int mbox_id, PCB* consumer) {
	if (TRACE)
		USLOSS_Console("TRACE: In join consumer queue\n");
	if (DEBUG)
		USLOSS_Console("DEBUG: Joining consumer queue for Mbox id: %d, pid: %d\n", mbox_id, consumer->id);

	MailBox* mbox_ptr = &mailboxes[mbox_id];
	if (mbox_ptr->consumers == NULL) {
		mbox_ptr->consumers = consumer;
	} else {
		PCB* last_consumer = mbox_ptr->consumers;
		while (last_consumer->next_consumer != NULL) {
			last_consumer = last_consumer->next_consumer;
		}
		last_consumer->next_consumer = consumer;
	}
}

void leave_consumer_queue(int mbox_id, PCB* consumer) {
	if (TRACE)
		USLOSS_Console("TRACE: In leave consumer queue\n");
	if (DEBUG)
		USLOSS_Console("DEBUG: Leaving consumer queue for Mbox id: %d, pid: %d\n", mbox_id, consumer->id);

	MailBox* mbox_ptr = &mailboxes[mbox_id];
	if (mbox_ptr->consumers == consumer) {
		mbox_ptr->consumers = consumer->next_consumer;
	} else {
		PCB* prev_consumer = mbox_ptr->consumers;
		while (prev_consumer->next_consumer != consumer) {
			if (prev_consumer == NULL) {
				USLOSS_Console("ERROR: consumer not found in queue. Returning \n");
				return;
			}
			prev_consumer = prev_consumer->next_consumer;
		}
		prev_consumer->next_consumer = consumer->next_consumer;
	}
}

void join_producer_queue(int mbox_id, PCB* producer) {
	if (TRACE)
		USLOSS_Console("TRACE: In join producer queue\n");
	if (DEBUG)
		USLOSS_Console("DEBUG: Joining producer queue for Mbox id: %d, pid: %d\n", mbox_id, producer->id);
	
	MailBox* mbox_ptr = &mailboxes[mbox_id];
	if (mbox_ptr->producers == NULL) {
		mbox_ptr->producers = producer;
	} else {
		PCB* last_producer = mbox_ptr->producers;
		while (last_producer->next_producer != NULL) {
			last_producer = last_producer->next_producer;
		}
		last_producer->next_producer = producer;
	}
}

void leave_producer_queue(int mbox_id, PCB* producer) {
	if (TRACE)
		USLOSS_Console("TRACE: In leave producer queue\n");
	if (DEBUG)
		USLOSS_Console("DEBUG: Leaving producer queue for Mbox id: %d, pid: %d\n", mbox_id, producer->id);

	MailBox* mbox_ptr = &mailboxes[mbox_id];
	if (mbox_ptr->producers == producer) {
		mbox_ptr->producers = producer->next_producer;
	} else {
		PCB* prev_producer = mbox_ptr->producers;
		while (prev_producer->next_producer != producer) {
			if (prev_producer == NULL) {
				USLOSS_Console("ERROR: producer not found in queue. Returning\n");
				return;
			}
			prev_producer = prev_producer->next_producer;
		}
		prev_producer->next_producer = producer->next_producer;
	}
	if(mbox_ptr->status != MAILBOX_ACTIVE_ZERO){
		producer->result = put_in_slot(mbox_id, producer->message, producer->msg_size);
	}
}


// Core Funcs
/////////////////////////////////////////////////////////////
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
		//mb.status = i <= MAILBOX_TERM4 ? MAILBOX_ACTIVE : MAILBOX_INACTIVE;
		mb.status = MAILBOX_INACTIVE;
		mailboxes[i] = mb;
	}
	// Create mailboxes for interrupts
	for(int m=0; m<7; m++){
		MboxCreate(1, sizeof(int));
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
	last_clock_send = currentTime();
	blocked_waitDevice = 0;
	USLOSS_IntVec[USLOSS_DISK_INT] = disk_handler;
	USLOSS_IntVec[USLOSS_TERM_INT] = term_handler;
	USLOSS_IntVec[USLOSS_SYSCALL_INT] = syscall_handler;
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
	mb.status = slots == 0 ? MAILBOX_ACTIVE_ZERO : MAILBOX_ACTIVE;
	mb.max_slots = slots;
	mb.slot_size = slot_size;
	mb.slots_used = 0;
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
	if (TRACE)
		USLOSS_Console("TRACE: In MboxRelease (id:%d)\n", mbox_id);

	int old_state = disable_interrupts();	
	if(mailboxes[mbox_id].status == MAILBOX_INACTIVE){
		restore_interrupts(old_state);
		return -1;	
	}
	MailBox* mbox_ptr = &mailboxes[mbox_id];
	mbox_ptr->status = MAILBOX_RELEASED;
	MailSlot* slot = mbox_ptr->head;
	while(slot!=NULL){
		slot->status = MAILSLOT_INACTIVE;
		slot = slot->next_message;
	}
	
	PCB* consumer = mbox_ptr->consumers;
	//restore_interrupts(old_state);

	while (consumer!=NULL) {
		unblockProc(consumer->id);
		consumer = consumer->next_consumer;
	}
	
	PCB* producer = mbox_ptr->producers;
	while (producer!=NULL) {
		unblockProc(producer->id);
		producer = producer->next_producer;
	}

	mbox_ptr->slots_used = 0;
	//mbox_ptr->status = MAILBOX_INACTIVE;
	restore_interrupts(old_state);
	return 0;
}

void print_mbox(int mbox_id){
	USLOSS_Console("MAILBOX %d\n", mbox_id);
	MailBox* mbox_ptr = &mailboxes[mbox_id];
	MailSlot* mslot_ptr = mbox_ptr->head;
	while(mslot_ptr!=NULL){
		USLOSS_Console("message ptr %p id %d is %s\n",mslot_ptr->message,  mslot_ptr->id, mslot_ptr->message);
		mslot_ptr = mslot_ptr->next_message;
	}	
}

void print_prodq(int mbox_id){
	USLOSS_Console("PRODUCER QUEUE %d\n", mbox_id);
MailBox* mbox_ptr = &mailboxes[mbox_id];
        PCB* pcb_ptr = mbox_ptr->producers;
        while(pcb_ptr!=NULL){
                USLOSS_Console("producer ptr %p id %d \n",pcb_ptr,  pcb_ptr->id);
                pcb_ptr = pcb_ptr->next_producer;
        }
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
	int old_state = disable_interrupts();
	int resp = Send(mbox_id, msg_ptr, msg_size, true);
	restore_interrupts(old_state);
	return resp;
}

int MboxCondSend(int mbox_id, void *msg_ptr, int msg_size) {
	int old_state = disable_interrupts();
	int resp = Send(mbox_id, msg_ptr, msg_size, false);
	restore_interrupts(old_state);
	return resp;
}
/**
Helper method for both MboxSend and MboxCondSend. This is where
the actual sending of messages occurs.
Args:
    mbox_id - the ID of the mailbox
    msg_ptr - pointer to a buffer. If messageSize is nonzero, then this must be non-NULL
    msg_size - the length of the message to send
    block_on_fail - true if this send should block, false if conditional send
Return Value:
    -3: the mailbox was released but is not completely invalid yet
    -2: The system has run out of global mailbox slots, so the message could not be queued
    -1: Illegal values given as arguments including invalid mailbox ID
    0: Success
*/
int Send(int mbox_id, void *msg_ptr, int msg_size, bool block_on_fail) {
	if (TRACE)
		USLOSS_Console("TRACE: In MboxSend (id:%d) \n", mbox_id);

	MailBox* mbox_ptr = &mailboxes[mbox_id];	
	if(mbox_ptr->status == MAILBOX_RELEASED){
		return -1;	
	}
	if(mbox_ptr->status == MAILBOX_INACTIVE || msg_size>MAX_MESSAGE || msg_size<0){
		return -1;
	}
	if(msg_size>mbox_ptr->slot_size){
		return -1;
	}
	int result=0;
	// Log process that is sending
	int pid = getpid();
	PCB* producer = &shadowTable[pid%MAXPROC];;
	if (shadowTable[pid%MAXPROC].status == PCB_INACTIVE) {
		producer->id = pid;
		producer->status = PCB_PRODUCER;
		producer->next_consumer = NULL;
		producer->next_producer = NULL;
	}

	if (mbox_ptr->status == MAILBOX_ACTIVE_ZERO) {
		// Check if a recipient is waiting. If not (and if block_on_fail), place into the producer queue and block process. If no block, return -2
		
		PCB* consumer = mbox_ptr->consumers;
		if (consumer == NULL) {
				if (block_on_fail) {
					memcpy(producer->message, msg_ptr, msg_size);
					producer->msg_size = msg_size;
					producer->status = BLOCK_WAITING_ON_CONSUMER;
					join_producer_queue(mbox_id, producer);
					blockMe(BLOCK_WAITING_ON_CONSUMER);	
					
					// Check if Mailbox is released while blocked
					if(mbox_ptr->status == MAILBOX_RELEASED) {
						shadowTable[pid%MAXPROC].status = PCB_INACTIVE;
						return -3;
					}
					
				} else {
					shadowTable[pid%MAXPROC].status = PCB_INACTIVE;
					return -2;
				}
		} 
		else {
				// Deliver direct to consumer
				memcpy(consumer->message, msg_ptr, msg_size);
				consumer->msg_size = msg_size;
				leave_consumer_queue(mbox_id, consumer);
				unblockProc(consumer->id);
				//msg_received = true;
		}
	} 
	// Regular slot mailbox
	else {
		// Check if mailbox is full. If so (and if block_on_fail), place into the producer queue and block process. If no block, return -2
                if (mbox_ptr->slots_used == mbox_ptr->max_slots) {
                        if (block_on_fail) {
                                producer->status = BLOCK_WAITING_ON_CONSUMER;
				memcpy(producer->message, msg_ptr, msg_size);
				producer->msg_size = msg_size;
                                join_producer_queue(mbox_id, producer);
                                //print_prodq(mbox_id);
				blockMe(BLOCK_WAITING_ON_CONSUMER);

                                // Check if Mailbox is released while blocked
                                if(mbox_ptr->status == MAILBOX_RELEASED) {
                                    shadowTable[pid%MAXPROC].status = PCB_INACTIVE;    
						return -3;
                                }
				//USLOSS_Console("unblock in send!\n");
				result = producer->result;
				//leave_producer_queue(mbox_id, producer);
				//print_prodq(mbox_id);
                        } else {
					shadowTable[pid%MAXPROC].status = PCB_INACTIVE;
					return -2;
                        }
                }
		else{/*
		// Initializing the slot
		MailSlot slot;
		int id = get_next_mailslot_id();
		if(id==-1){
			USLOSS_Console("MboxSend_helper: Could not send, the system is out of mailbox slots.\n");
			shadowTable[pid%MAXPROC].status = PCB_INACTIVE;
			return -2;
		}

		slot.id = id;
		slot.msg_size = msg_size;	
		memcpy(slot.message, msg_ptr, msg_size);
		slot.claimant = NULL;
		slot.next_message = NULL;
		slot.status = MAILSLOT_ACTIVE;
		mailSlots[id%MAXSLOTS] = slot;


		// Adding message to slot 
		if (mbox_ptr->head == NULL) {
			mbox_ptr->head = &mailSlots[id];
		} else {
			MailSlot* slot_ptr = mbox_ptr->head;
			while (slot_ptr->next_message != NULL) {
				slot_ptr = slot_ptr->next_message;
			}
			slot_ptr->next_message = &mailSlots[id];
		}
		mbox_ptr->slots_used++;
		print_mbox(mbox_id);
		
		// Check if there is a consumer ready
		PCB* consumer = mbox_ptr->consumers;
		if (consumer != NULL) {
			mailSlots[id].status = MAILSLOT_MSG_CLAIMED; 
			mailSlots[id].claimant = consumer;

			// Copy the message onto the PCB object
			memcpy(consumer->message, msg_ptr, msg_size);
		
			if (consumer->status == BLOCK_WAITING_ON_PRODUCER) {
				if (DEBUG)
					USLOSS_Console("DEBUG: Consumer waiting. Unblocking (%d)\n", consumer->id);

				consumer->status = PCB_CONSUMER;
				leave_consumer_queue(mbox_id, consumer);
				USLOSS_Console("unblocking consumer %d and giving it message %d\n", consumer->id, id);
				unblockProc(consumer->id);	
			}
		} else {
			mailSlots[id].status = MAILSLOT_MSG_UNCLAIMED;
		}*/
		result = put_in_slot(mbox_id, msg_ptr, msg_size);
		}
	}
	shadowTable[pid%MAXPROC].status = PCB_INACTIVE;
	return result;

}

int put_in_slot(int mbox_id, void* msg_ptr, int  msg_size){
		MailBox* mbox_ptr = &mailboxes[mbox_id%MAXMBOX];
		MailSlot slot;
                int id = get_next_mailslot_id();
                if(id==-1){
                        USLOSS_Console("MboxSend_helper: Could not send, the system is out of mailbox slots.\n");
                        //shadowTable[pid%MAXPROC].status = PCB_INACTIVE;
                        return -2;
                }

                slot.id = id;
                slot.msg_size = msg_size;
                memcpy(slot.message, msg_ptr, msg_size);
                slot.claimant = NULL;
                slot.next_message = NULL;
                slot.status = MAILSLOT_ACTIVE;
                mailSlots[id%MAXSLOTS] = slot;


                // Adding message to slot 
                if (mbox_ptr->head == NULL) {
                        mbox_ptr->head = &mailSlots[id];
                } else {
                        MailSlot* slot_ptr = mbox_ptr->head;
                        while (slot_ptr->next_message != NULL) {
                                slot_ptr = slot_ptr->next_message;
                        }
                        slot_ptr->next_message = &mailSlots[id];
                }
                mbox_ptr->slots_used++;
		// Check if there is a consumer ready
                PCB* consumer = mbox_ptr->consumers;
                if (consumer != NULL) {
                        mailSlots[id].status = MAILSLOT_MSG_CLAIMED;
                        mailSlots[id].claimant = consumer;

                        // Copy the message onto the PCB object
                        memcpy(consumer->message, msg_ptr, msg_size);

                        if (consumer->status == BLOCK_WAITING_ON_PRODUCER) {
                                if (DEBUG)
                                        USLOSS_Console("DEBUG: Consumer waiting. Unblocking (%d)\n", consumer->id);

                                consumer->status = PCB_CONSUMER;
                                leave_consumer_queue(mbox_id, consumer);
                                //USLOSS_Console("unblocking consumer %d and giving it message %d\n", consumer->id, id);
                                unblockProc(consumer->id);
                        }
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
	int old_state = disable_interrupts();
	int resp = Recv(mbox_id, msg_ptr, msg_max_size, true);
	restore_interrupts(old_state);
	return resp;
}

int MboxCondRecv(int mbox_id, void *msg_ptr, int msg_max_size) {
	int old_state = disable_interrupts();
	int resp = Recv(mbox_id, msg_ptr, msg_max_size, false);
	restore_interrupts(old_state);
	return resp;
}

int Recv(int mbox_id, void *msg_ptr, int msg_max_size, bool block_on_fail) {
	if (TRACE)
		USLOSS_Console("TRACE: In MboxRecv (id:%d)\n", mbox_id);

	MailBox* mbox_ptr = &mailboxes[mbox_id%MAXMBOX];
	if (mbox_ptr->status == MAILBOX_RELEASED) {
		return -1;
	}
	if (mbox_ptr->status == MAILBOX_INACTIVE) {
		return -1;
	}
	if (msg_max_size < 0) {
		USLOSS_Console("ERROR: Invalid parameters\n");
		return -1;
	}
	//USLOSS_Console("Current mbox before receive\n");
	//print_mbox(mbox_id);
	// Log process that is receiving
	int pid = getpid();
	PCB* consumer = &shadowTable[pid%MAXPROC];
	if (shadowTable[pid%MAXPROC].status == PCB_INACTIVE) {
		consumer->id = pid;
		consumer->status = PCB_CONSUMER;
		consumer->next_consumer = NULL;
		consumer->next_producer = NULL;
	}

	// Used as a return value
	int msg_size;
	bool msg_received = false;

	// If a zero slot mailbox, check if the consumer already has a message on the table. If not, block if allowed (or fail if not)
	if (mbox_ptr->status == MAILBOX_ACTIVE_ZERO) {
		if(mbox_ptr->producers!=NULL){
			PCB* producer = mbox_ptr->producers;
			msg_size =  producer->msg_size; 
			// Trying to recieve a message that's bigger than buffer
			if(msg_size>msg_max_size){
				shadowTable[pid%MAXPROC].status = PCB_INACTIVE;
				return -1;
			}
			if (msg_ptr != NULL)
				memcpy(msg_ptr, producer->message, msg_size);	
			leave_producer_queue(mbox_id, producer);
			producer->status = PCB_PRODUCER;
			unblockProc(producer->id);
		}
		else{
			if (block_on_fail) {
                        	consumer->status = BLOCK_WAITING_ON_PRODUCER;
                                join_consumer_queue(mbox_id, consumer);
                                blockMe(BLOCK_WAITING_ON_PRODUCER);
                                        
                                // Check if Mailbox is released while blocked
                                if(mbox_ptr->status == MAILBOX_RELEASED) {
                                	shadowTable[pid%MAXPROC].status = PCB_INACTIVE;
						return -3;
                                }
					msg_size = consumer->msg_size;
					// Trying to recieve a message that's bigger than buffer
					if(msg_size>msg_max_size){
						shadowTable[pid%MAXPROC].status = PCB_INACTIVE;
						return -1;
					}
                        	if (msg_ptr != NULL)
                                	memcpy(msg_ptr, consumer->message, msg_size);
				
                        } else {
                              shadowTable[pid%MAXPROC].status = PCB_INACTIVE;          
					return -2;
                       	}
 		}
		shadowTable[pid%MAXPROC].status = PCB_INACTIVE;
		return msg_size;
	}

	else {
		// Checking if slot exists. If not, block
		while (mbox_ptr->head == NULL) {
			if (block_on_fail) {
				consumer->status = BLOCK_WAITING_ON_PRODUCER;
				join_consumer_queue(mbox_id, consumer);
				blockMe(BLOCK_WAITING_ON_PRODUCER);
			
				if (DEBUG) {
					USLOSS_Console("DEBUG: Released from block (pid:%d)\n", consumer->id);
					dumpProcesses();
				}	
				// Check if Mailbox is released while blocked
				if(mbox_ptr->status == MAILBOX_RELEASED) {
					shadowTable[pid%MAXPROC].status = PCB_INACTIVE;
					return -3;
				}
			} else {
				shadowTable[pid%MAXPROC].status = PCB_INACTIVE;
				return -2;
			}
		}
		
		MailSlot* slot_ptr = mbox_ptr->head;
		
		// Check for prior receiver
		// If unmarked, then this process is the consumer and it gets it
		// If reserved, then check for the consumer in the queue and take it off the queue
		// If no messages for the calling receiver, block and try again when unblocked
		while (!msg_received) {
			if (slot_ptr->status == MAILSLOT_MSG_UNCLAIMED) {
				slot_ptr->status = MAILSLOT_MSG_CLAIMED;
				slot_ptr->claimant = consumer;
				msg_received = true;
			} else if (slot_ptr->claimant == consumer) {
				msg_received = true;
			} else {
				slot_ptr = slot_ptr->next_message;
				while (slot_ptr != NULL && !msg_received) {
					if (slot_ptr->claimant == consumer) {
						//leave_consumer_queue(mbox_id, consumer);
						msg_received = true;
					} else {
						slot_ptr = slot_ptr->next_message;
					}
				}
			}		

			// No open mail was found
			if (!msg_received) {
				if (DEBUG)
					USLOSS_Console("DEBUG: All messages already claimed\n");
	 
				if (block_on_fail) {
					consumer->status = BLOCK_WAITING_ON_PRODUCER;
					join_consumer_queue(mbox_id, consumer);
					blockMe(BLOCK_WAITING_ON_PRODUCER);
						
					// Check if Mailbox is released while blocked
					if(mbox_ptr->status == MAILBOX_RELEASED) {
						return -3;
					}

					// Restarting the search after release from block
					slot_ptr = mbox_ptr->head;
					consumer->status = PCB_CONSUMER;

				} else {
					USLOSS_Console("ERROR: Conditional receive and all messages are already claimed\n");
					shadowTable[pid%MAXPROC].status = PCB_INACTIVE;
					return -2;
				}
			}
		}

		// Consume the message by copying it onto the provided pointer
		msg_size = slot_ptr->msg_size;
		// Trying to recieve a message that's bigger than buffer
		if(msg_size>msg_max_size){
			return -1;
		}
		if (msg_ptr != NULL)
			memcpy(msg_ptr, slot_ptr->message, msg_size);
		
		// Removing the claimed mail slot
		if (slot_ptr == mbox_ptr->head) {
			mbox_ptr->head = slot_ptr->next_message;
		} else {
			MailSlot* prev_msg = mbox_ptr->head;
			while (prev_msg->next_message != slot_ptr) {
				prev_msg = prev_msg->next_message;
			}
			prev_msg->next_message = slot_ptr->next_message;
		}

		slot_ptr->status = MAILSLOT_INACTIVE;
		mbox_ptr->slots_used--;
	}
	
	// Removing consumer from shadowTable
	shadowTable[pid%MAXPROC].status = PCB_INACTIVE;		
	//USLOSS_Console("at end of receive\n");
	//print_prodq(mbox_id);	
	// Unblocking the producer, if it's blocked
	PCB* producer_ptr = mbox_ptr->producers;
	if (producer_ptr != NULL) {
		//mbox_ptr->producers = producer_ptr->next_producer;
		leave_producer_queue(mbox_id, producer_ptr);
		//USLOSS_Console("done realesing\n");
		//if(producer_ptr->status!=PCB_PRODUCER){
			producer_ptr->status = PCB_PRODUCER;
			unblockProc(producer_ptr->id);	
		//	return msg_size;
		//}
		//producer_ptr = producer_ptr->next_producer;
	}
	
	return msg_size;
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
void waitDevice(int type, int unit, int *status) {
	int old_state = disable_interrupts();
	int mbox_id;
	if(type==USLOSS_CLOCK_DEV){
		if(unit!=0){
			USLOSS_Console("ERROR: Invalid value for unit field in waitDevice()\n");
			USLOSS_Halt(1);
		}
		mbox_id = 0;
	}
	else if(type==USLOSS_DISK_DEV){
		if(unit>1||unit<0){
			USLOSS_Console("ERROR: Invalid value for unit field in waitDevice()\n");
			USLOSS_Halt(1);
		}
		mbox_id = 1+unit;
	}
	else if(type==USLOSS_TERM_DEV){
		if(unit>3||unit<0){
			USLOSS_Console("ERROR: Invalid value for unit field in waitDevice()\n");
			USLOSS_Halt(1);
		}
		mbox_id = 3+unit;
	}
	else{
		USLOSS_Console("ERROR: Invalid device type\n");
		USLOSS_Halt(1);	
	}

	blocked_waitDevice++;
	Recv(mbox_id, status, sizeof(int), true);
	blocked_waitDevice--;
	restore_interrupts(old_state);
}

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
	if(blocked_waitDevice>0){
		return 1;
	}
	return 0;
}

/* Clock interrupt handler from Phase1
 * 
 * May Block: No
 * May Context Switch: Yes
 * Args: None
 * Return Value: None 
 */
void phase2_clockHandler(void) {
	// check wall clock time, it is has been 100ms or more, send another 
	// message on mailbox
	int now = currentTime();
	if(now-last_clock_send>=100000){
		last_clock_send = now;
		Send(MAILBOX_CLOCK, &now, sizeof(int), false);
	}
}

static void disk_handler(int type, void* unit){
	int unitNo = (int)(long)unit;
	int mbox_id = 1 + unitNo;
	int status;
	USLOSS_DeviceInput(USLOSS_DISK_DEV, unitNo, &status);
	MboxCondSend(mbox_id, &status, sizeof(int));
}

static void term_handler(int type, void* unit){
	if (TRACE)
		USLOSS_Console("TRACE: In term handler\n");

	int unitNo = (int)(long)unit;
	int mbox_id = 3 + unitNo;
	int status;
	USLOSS_DeviceInput(USLOSS_TERM_DEV, unitNo, &status);
	MboxCondSend(mbox_id, &status, sizeof(int));
}

static void syscall_handler(int type, void* args){
	USLOSS_Sysargs* syscall_args = args;
	int number = syscall_args->number;
	if(number<0 || number>49){
		USLOSS_Console("syscallHandler(): Invalid syscall number %d\n", number);
		USLOSS_Halt(-1);
	}
	(*systemCallVec[number])(syscall_args);
}

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
	if (TRACE)
		USLOSS_Console("TRACE: Disabling interrupts\n");

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
	if (TRACE)
		USLOSS_Console("TRACE: Restoring interrupts\n");

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

