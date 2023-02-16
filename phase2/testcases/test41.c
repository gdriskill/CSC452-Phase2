
/* Attempt to receive a message from a mailbox using a buffer that is
 * too small for the message.
 */

#include <stdio.h>

#include <usloss.h>
#include <phase1.h>
#include <phase2.h>

char buf[256];
int mbox_id;



int start2(char *arg)
{
    int result;
    char buffer[11] = "xxxxxxxxxx";

    USLOSS_Console("start2(): started\n");

    mbox_id = MboxCreate(10, 13);
    USLOSS_Console("start2(): MboxCreate returned id = %d\n", mbox_id);

    result = MboxSend(mbox_id, "hello there", 12);
    USLOSS_Console("start2(): after send of message, result = %d\n", result);

    USLOSS_Console("start2(): Attempting to receive message fm mailbox %d.\n", mbox_id);
    USLOSS_Console("          Should fail here because its receive buffer\n");
    USLOSS_Console("          is too small\n");

    result = MboxRecv(mbox_id, buffer, sizeof(buffer));
    USLOSS_Console("start2(): after receive of message, result = %d   message is '%s'\n", result, buffer);

    if (result == -1){
        USLOSS_Console("start2(): got that message was too big. PASSED!\n");
        quit(0);
    }
    else {
        USLOSS_Console("start2(): FAILED!\n");
        quit(0);
    }

    quit(0);
}

