
/* Send to a mailbox a message that is too large for the mailbox. */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>


int XXp1(char *);
char buf[256];



int start2(char *arg)
{
    int mbox_id;
    int result;

    USLOSS_Console("start2(): started\n");

    mbox_id = MboxCreate(10, 11);
    USLOSS_Console("start2(): MboxCreate returned id = %d\n", mbox_id);

    USLOSS_Console("start2(): sending message to mailbox %d\n", mbox_id);
    result = MboxSend(mbox_id, "hello there", 12);
    USLOSS_Console("start2(): after send of message, result = %d\n", result);

    if (result == -1){
        USLOSS_Console("start2(): message was too large (expected).\n");
        quit(0);
    }

    USLOSS_Console("ERROR ERROR ERROR If you get here, then your mailbox code is not checking the send-length properly.\n");

    return 0; /* so gcc will not complain about its absence... */
}

