
/* attempts to create a single mailbox with more than MAXSLOTS slots.
 * Should fail.  Note that other testcases have already tested the
 * (legal) case of many mailboxes that overcommit the total number of
 * slots.
 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>



int start2(char *arg)
{
    int mbox_id;

    USLOSS_Console("start2(): started, trying to create a single mailbox with more than MAXSLOTS slots.  Should fail.\n");

    mbox_id = MboxCreate(MAXSLOTS+1, 50);
    if (mbox_id < 0)
        USLOSS_Console("start2(): MboxCreate returned id less than zero (as expected), id = %d\n", mbox_id);
    else
        USLOSS_Console("ERROR: MboxCreate() succeeded when it should not!\n");

    quit(0);
}

