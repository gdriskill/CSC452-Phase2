/* checking for release: 3 instances of XXp2 send messages to a zero-slot
 * mailbox, which causes them to block. XXp3 then releases the mailbox.
 * XXp3 is lower priority than the 3 blocked processes.
 */


#include <stdio.h>
#include <string.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>


int XXp2(char *);
int XXp3(char *);
char buf[256];


int mbox_id;



int start2(char *arg)
{
    int kid_status, kidpid;
    int result;

    USLOSS_Console("start2(): started\n");

    mbox_id  = MboxCreate(0, 50);
    USLOSS_Console("\nstart2(): MboxCreate returned id = %d\n", mbox_id);

    USLOSS_Console("\nstart2(): Creating 3 instances of XXp2\n");
    kidpid   = fork1("XXp2a", XXp2, "XXp2a", 2 * USLOSS_MIN_STACK, 1);
    kidpid   = fork1("XXp2b", XXp2, "XXp2b", 2 * USLOSS_MIN_STACK, 1);
    kidpid   = fork1("XXp2c", XXp2, "XXp2c", 2 * USLOSS_MIN_STACK, 1);

    kidpid   = fork1("XXp3",  XXp3, NULL,    2 * USLOSS_MIN_STACK, 2);

    kidpid = join(&kid_status);
    USLOSS_Console("start2(): joined with kid %d, status = %d\n\n", kidpid, kid_status);

    kidpid = join(&kid_status);
    USLOSS_Console("start2(): joined with kid %d, status = %d\n\n", kidpid, kid_status);

    kidpid = join(&kid_status);
    USLOSS_Console("start2(): joined with kid %d, status = %d\n\n", kidpid, kid_status);

    kidpid = join(&kid_status);
    USLOSS_Console("start2(): joined with kid %d, status = %d\n\n", kidpid, kid_status);

    result = MboxCondSend(mbox_id, NULL,0);

    if (result == -1)
        USLOSS_Console("failed to send to released mailbox ... success\n");
    else
        USLOSS_Console("test failed result = %d\n",result);

    quit(0);
}

int XXp2(char *arg)
{
    int result;

    USLOSS_Console("%s(): sending message to mailbox %d\n", arg, mbox_id);
    result = MboxSend(mbox_id, NULL,0);
    USLOSS_Console("%s(): after send of message, result = %d\n", arg, result);

    if (result == -3)
        USLOSS_Console("%s(): mailbox destroyed by MboxSend() call\n", arg);

    quit(3);
}

int XXp3(char *arg)
{
    int result;

    USLOSS_Console("\nXXp3(): started, releasing mailbox %d\n", mbox_id);

    result = MboxRelease(mbox_id);
    USLOSS_Console("\nXXp3(): MboxRelease returned %d\n", result);

    quit(4);
}

