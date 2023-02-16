/* Creates two children.  Higher priority child does a send, and should
 * block.  Lower priority child then does a receive and should unblock the
 * higher priority child.
 */

#include <stdio.h>
#include <string.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>

int XXp1(char *);
int XXp2(char *);
char buf[256];

int mbox_id;



int start2(char *arg)
{
    int kid_status, kidpid;

    USLOSS_Console("start2(): started\n");

    mbox_id = MboxCreate(0, 50);
    USLOSS_Console("\nstart2(): MboxCreate returned id = %d\n", mbox_id);

    kidpid = fork1("XXp1", XXp1, NULL, 2 * USLOSS_MIN_STACK, 1);
    kidpid = fork1("XXp2", XXp2, NULL, 2 * USLOSS_MIN_STACK, 2);

    kidpid = join(&kid_status);
    USLOSS_Console("\nstart2(): joined with kid %d, status = %d\n", kidpid, kid_status);

    kidpid = join(&kid_status);
    USLOSS_Console("\nstart2(): joined with kid %d, status = %d\n", kidpid, kid_status);

    quit(0);
}

int XXp1(char *arg)
{
    int i, result;

    USLOSS_Console("XXp1(): started\n");

    for (i = 0; i <= 1; i++) {
        USLOSS_Console("XXp1(): sending message to mailbox %d\n", mbox_id);
        result = MboxSend(mbox_id, NULL,0);
        USLOSS_Console("XXp1(): after send of message, result = %d\n", result);
    }

    quit(3);
}

int XXp2(char *arg)
{
    int i, result;

    USLOSS_Console("XXp2(): started\n");

    for (i = 0; i <= 1; i++) {
        USLOSS_Console("XXp2(): receiving message from mailbox %d\n", mbox_id);
        result = MboxRecv(mbox_id, NULL,0);
        USLOSS_Console("XXp2(): after receipt of message, result = %d\n", result);
    }

    quit(4);
}

