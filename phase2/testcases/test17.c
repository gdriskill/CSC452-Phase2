
/* tests for exceeding the number of slots. start2 creates mailboxes whose
 * total slots will exceed the system limit. start2 then starts doing
 * conditional sends to each slot of each mailbox until the return code
 * of conditional send comes back as -2
 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>

int mboxids[50];



int start2(char *arg)
{
    int boxNum, slotNum, result;

    USLOSS_Console("start2(): started, trying to exceed systemwide mailslots...\n");

    /* See the previous testcase for explanation.  This is the EXACT SAME,
     * except that we call CondSend() instead.
     */

    for (boxNum = 0; boxNum < 50; boxNum++)
    {
        mboxids[boxNum] = MboxCreate(55, 0);
        if (mboxids[boxNum] < 0)
            USLOSS_Console("start2(): MailBoxCreate returned id less than zero, id = %d\n", mboxids[boxNum]);
    }

    for (boxNum = 0; boxNum < 50; boxNum++)
    {
        for (slotNum = 0; slotNum < 55; slotNum++)
        {
            result = MboxCondSend(mboxids[boxNum], NULL,0);
            if (result == -2)
            {
                USLOSS_Console("No slots available: mailbox %d and slot %d\n", boxNum, slotNum);
                quit(0);
            }
            else if (result != 0)
            {
                USLOSS_Console("UNEXPECTED ERROR %d: mailbox %d and slot %d\n", result, boxNum, slotNum);
                quit(100);
            }
        }
    }

    USLOSS_Console("ERROR: You should not get here!!!\n");
    quit(100);
}

