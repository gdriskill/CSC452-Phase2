
/* Another test of receive buffer size being too small.  In this case,
 * the receiving process is blocked first since there are no messages.
 * When the message is sent later, the receiver should get the return
 * result that indicates the receive buffer size is too small.
 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>

int XXp1(char *);
char buf[256];
int mbox_id;



int start2(char *arg)
{
    int result, kidpid;
    char buffer[11] = "xxxxxxxxxx";

    USLOSS_Console("start2(): started\n");

    mbox_id = MboxCreate(1, 13);
    USLOSS_Console("start2(): MboxCreate returned id = %d\n", mbox_id);

    kidpid = fork1("XXp1", XXp1, NULL, 2 * USLOSS_MIN_STACK, 2);

    USLOSS_Console("start2(): Attempting to receive message from mailbox %d, ", mbox_id);
    USLOSS_Console("but it is blocked\n");
    USLOSS_Console("          because the slots are empty.\n");
    USLOSS_Console("          Later, it should fail due to the buffer size ");
    USLOSS_Console("being too small.\n");

    result = MboxRecv(mbox_id, buffer, sizeof(buffer));

    USLOSS_Console("\n");
    USLOSS_Console("start2(): after receive of message, result = %d   message is '%s'\n", result, buffer);

    if (result == -1){
        USLOSS_Console("start2(): got that message was too big. PASSED!\n");
    }
    else {
        USLOSS_Console("start2(): FAILED!\n");
        quit(0);
    }

    USLOSS_Console("start2(): joining with child\n");
    join(&result);
    result = kidpid;  // to avoid warning about kidpid set but not used

    quit(0);
}

int XXp1(char *arg)
{
    int  result;

    USLOSS_Console("\nXXp1(): started\n");
    USLOSS_Console("XXp1(): arg = '%s'\n", arg);
    result = MboxSend(mbox_id, "hello there", 12);
    USLOSS_Console("XXp1(): after send of message, result = %d\n", result);

    quit(3);
}

