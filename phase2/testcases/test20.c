
/* checking for release: 3 instances of XXp2 receive messages from a zero-slot
 * mailbox, which causes them to block. XXp4 then releases the mailbox.
 * All processes are at the same priority.
 */

#include <stdio.h>
#include <string.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>

int XXp2(char *);
int XXp3(char *);
int XXp4(char *);
char buf[256];

int mbox_id;



int start2(char *arg)
{
    int kid_status, kidpid, pausepid;

    USLOSS_Console("start2(): started\n");
    mbox_id  = MboxCreate(0, 50);
    USLOSS_Console("\nstart2(): MboxCreate returned id = %d\n", mbox_id);

    kidpid   = fork1("XXp2a", XXp2, "XXp2a", 2 * USLOSS_MIN_STACK, 2);
    kidpid   = fork1("XXp2b", XXp2, "XXp2b", 2 * USLOSS_MIN_STACK, 2);
    kidpid   = fork1("XXp2c", XXp2, "XXp2c", 2 * USLOSS_MIN_STACK, 2);
    pausepid = fork1("XXp4",  XXp4, "XXp4",  2 * USLOSS_MIN_STACK, 2);

    kidpid = join(&kid_status);
    if (kidpid != pausepid)
        USLOSS_Console("\n***Test Failed*** -- join with pausepid failed!\n\n");

    kidpid   = fork1("XXp3",  XXp3, NULL,    2 * USLOSS_MIN_STACK, 1);

    kidpid = join(&kid_status);
    USLOSS_Console("\nstart2(): joined with kid %d, status = %d\n", kidpid, kid_status);

    kidpid = join(&kid_status);
    USLOSS_Console("\nstart2(): joined with kid %d, status = %d\n", kidpid, kid_status);

    kidpid = join(&kid_status);
    USLOSS_Console("\nstart2(): joined with kid %d, status = %d\n", kidpid, kid_status);

    kidpid = join(&kid_status);
    USLOSS_Console("\nstart2(): joined with kid %d, status = %d\n", kidpid, kid_status);

    quit(0);
}

int XXp2(char *arg)
{
    int result;

    result = MboxRecv(mbox_id, NULL,0);
    USLOSS_Console("%s(): after recv of message, result = %d\n", arg, result);

    if (result == -3)
        USLOSS_Console("%s(): mailbox was destroyed by MboxRecv() call\n", arg);

    quit(3);
}

int XXp3(char *arg)
{
    int result;

    USLOSS_Console("XXp3(): started\n");

    result = MboxRelease(mbox_id);
    USLOSS_Console("XXp3(): MboxRelease returned %d\n", result);

    quit(4);
}

int XXp4(char *arg)
{
    USLOSS_Console("XXp4(): started and quitting\n");
    quit(4);
}

