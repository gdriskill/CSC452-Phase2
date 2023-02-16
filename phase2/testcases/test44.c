
/* Creates a mailbox with MAXSLOTS slots.  XXp1 fills half the slots using
 * conditional send.  XXp2 fills the other half of the slots using
 * conditional send.  start2 receives 10 messages from the mailbox.
 * start2 then uses conditional send to try to fill more than the 10
 * empty slots.
 */

#include <stdio.h>
#include <string.h>

#include <usloss.h>
#include <phase1.h>
#include <phase2.h>


int XXp1(char *);
int XXp2(char *);
int XXp3(char *);
char buf[256];

int mboxId;



int start2(char *arg)
{
    int kidStatus, kidpid, i, result;
    char buffer[30];

    /* BUGFIX: initialize buffers to predictable contents */
    memset(buffer, 'x', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';

    USLOSS_Console("start2(): started\n");

    mboxId = MboxCreate(MAXSLOTS, 50);
    USLOSS_Console("start2(): MboxCreate returned id = %d\n", mboxId);

    USLOSS_Console("start2(): forking XXp1\n");
    kidpid = fork1("XXp1", XXp1, "XXp1", 2 * USLOSS_MIN_STACK, 1);
    USLOSS_Console("start2(): forking XXp2\n");
    kidpid = fork1("XXp2", XXp2, "XXp2", 2 * USLOSS_MIN_STACK, 2);

    kidpid = join(&kidStatus);
    USLOSS_Console("start2(): joined with kid %d, status = %d\n", kidpid, kidStatus);

    kidpid = join(&kidStatus);
    USLOSS_Console("start2(): joined with kid %d, status = %d\n", kidpid, kidStatus);

    /* receive 10 from the mailbox */
    for (i = 0; i < 10; i++) {
        USLOSS_Console("start2(): receiving message #%d from mailbox %d\n", i, mboxId);
        result = MboxCondRecv(mboxId, buffer, 30);
        USLOSS_Console("start2(): result = %d\n", result);
        USLOSS_Console("start2(): message = '%s'\n", buffer);
    }

    /* send 25 to the mailbox, should fail on 21 */
    for (i = 0; i < 25; i++) {
        USLOSS_Console("start2(): sending message #%d to mailbox %d\n", i, mboxId);
        if (i>=20) {
            USLOSS_Console("start2(): this should return -2\n");
        }
        sprintf(buffer, "start2(): hello there, #%d", i);
        result = MboxCondSend(mboxId, buffer, strlen(buffer)+1);
        USLOSS_Console("start2(): after send of message #%d, result = %d\n", i, result);
    }

    quit(0);
}

int XXp1(char *arg)
{
    int i, result;
    char buffer[30];

    USLOSS_Console("XXp1(): started\n");

    for (i = 0; i < MAXSLOTS/2; i++) {
        if ( (i % 500 == 0) || (i > 1200) )
            USLOSS_Console("XXp1(): sending message #%d to mailbox %d\n", i, mboxId);
        sprintf(buffer, "XXp1(): hello there, #%d", i);
        result = MboxCondSend(mboxId, buffer, strlen(buffer)+1);
        if ( (i % 500 == 0) || (i > 1200) )
            USLOSS_Console("XXp1(): after send of message #%d, result = %d\n", i, result);
    }

    quit(3);
}

int XXp2(char *arg)
{
    int i, result;
    char buffer[30];

    USLOSS_Console("XXp2(): started\n");

    //after the loop in XXp1 and this loop runs, there should be MAXSLOTS-10
    //slots in use
    for (i = 0; i < (MAXSLOTS/2)-10; i++) {
        if ( (i % 500 == 0) || (i > 1200) )
            USLOSS_Console("XXp2(): sending message #%d to mailbox %d\n", i, mboxId);
        sprintf(buffer, "x2:hello there, #%d", i);
        result = MboxCondSend(mboxId, buffer, strlen(buffer)+1);
        if ( (i % 500 == 0) || (i > 1200) )
            USLOSS_Console("XXp2(): after send of message #%d, result = %d\n", i, result);
     }

     quit(3);
}

