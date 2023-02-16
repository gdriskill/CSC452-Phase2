
/* tests for exceeding the number of slots. start2 creates mailboxes whose
 * total slots will exceed the system limit. start2 then starts doing
 * conditional sends to each slot of each mailbox until the return code
 * of conditional send comes back as -2
 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>

int mboxids[45];



int start2(char *arg)
{
    int boxNum, slotNum, result;

    USLOSS_Console("start2(): started, trying to exceed systemwide mailslots...\n");

    /* 45 mailboxes, capacity 55 each.  We'll try to send 56 messages to each one,
     * and the 56th will fail because we're using CondSend().  There will never be
     * a complete system-wide clog, however.
     */

    for (boxNum = 0; boxNum < 45; boxNum++)
    {
        mboxids[boxNum] = MboxCreate(55, 0);
        if (mboxids[boxNum] < 0)
            USLOSS_Console("start2(): MailBoxCreate returned id less than zero, id = %d\n", mboxids[boxNum]);
    }

    for (boxNum = 0; boxNum < 45; boxNum++)
    {
        for (slotNum = 0; slotNum < 56; slotNum++)
        {
            result = MboxCondSend(mboxids[boxNum], NULL,0);
            if (result == -2)
            {
                USLOSS_Console("Mailbox has no more slots available, and so CondSend() returned -2: mailbox %d and slot %d\n", boxNum, slotNum);
            }
            else if (result != 0)
            {
                USLOSS_Console("UNEXPECTED ERROR %d: mailbox %d and slot %d\n", result, boxNum, slotNum);
                quit(100);
            }
        }
    }

    quit(0);
}

