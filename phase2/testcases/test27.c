#include <stdio.h>
#include <memory.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>


/* test overflowing the mailbox, releasing some, then overflowing again */


int mbox_ids[MAXMBOX];


int start2(char *arg)
{
    int mbox_id;
    int i, result;

    memset(mbox_ids, 0, sizeof(mbox_ids));

    USLOSS_Console("start2(): started, trying to create too many mailboxes...\n");

    // Create MAXMBOX - 7 mailboxes -- all should succeed
    //    BUGFIX: reduce the # of mailboxes by 7, since there are 7
    //            interrupt-mailboxes which must be implemented.
    for (i = 0; i < MAXMBOX-7; i++) {
        mbox_ids[i] = MboxCreate(10, 50);
        if (mbox_ids[i] < 0) {
            USLOSS_Console("start2(): MailBoxCreate returned id less than zero, i=%d   MboxCreate rc = %d\n", i, mbox_ids[i]);
            USLOSS_Halt(1);
        }
    }


    // Release 6 of the mailboxes
    for (i = 0; i < 6; i++) { 
        USLOSS_Console("start2(): calling MboxRelease(%d)\n", mbox_ids[i]);
        result = MboxRelease(mbox_ids[i]);
        USLOSS_Console("start2(): calling MboxRelease(%d) returned %d\n", mbox_ids[i], result);
    }


    // Create 8 mailboxes, which be two mailboxes too many
    for (i = 0; i < 8; i++) {
        mbox_id = MboxCreate(10, 50);
        if (mbox_id < 0) {
            USLOSS_Console("start2(): MailBoxCreate for i = %d returned id less than zero, id = %d\n", i, mbox_id);
        }
        else {
            USLOSS_Console("start2(): MailBoxCreate for i = %d returned id = %d\n", i, mbox_id);
        }
    }

    quit(0);
}

