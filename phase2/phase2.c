#include <usloss.h>
#include <usyscall.h>
#include "phase1.h"
#include "phase2.h"

typedef struct PCB {
	int id;
} PCB;

typedef struct MailBox {
	int id;
} MailBox;

typedef struct Message {
	int id;
	char message[MAX_MESSAGE];
} Message;

typedef struct MailSlot {
	Message* slot;
	Message* nextMessage; 
} MailSlot;


static MailBox* mailboxes[MAXMBOX];
static MailSlot* mailSlots[MAXSLOTS];
PCB* shadowTable[MAXPROC];

void (*systemCallVec[])(USLOSS_Sysargs *args);

/////////////////////////////////////////////////////////////

/* Similar to phase1_init. Used to initialize any data structures.
 * Do not fork1 from here
 * 
 * May Block: No
 * May Context Switch: No
 * Args: None
 * Return Value: None
 */
void phase2_init(void) {}

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
	return -1;
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
	return -1;
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
	return -1;
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
