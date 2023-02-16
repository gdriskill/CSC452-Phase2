
/* Creates a 5-slot mailbox. Creates XXp1 that conditionally sends eight hello
 * messages * to the mailbox, five of which should succeed and three will
 * return -2.  XXp1 then blocks on a receive on its private mailbox.
 * Creates XXp3 which receives the five hello messages that should be available
 * from the slots.  XXp3 then sends to XXp1's private mailbox.  Since XXp3 is
 * lower priority than XXp1, XXp1 runs again.
 * XXp1 wakes up from its private mailbox and sends eight goodbye messages to
 * the mailbox, five of which should succeed and three will return -2.  XXp1
 * then quits.
 * XXp3 should pick up the five good-bye messages from the mailbox and quit.
 */

#include <stdio.h>
#include <string.h>

#include <usloss.h>
#include <phase1.h>
#include <phase2.h>

int XXp1(char *);
int XXp3(char *);
char buf[256];

int mboxId, privateMbox;



int start2(char *arg)
{
    int kidStatus, kidpid;

    USLOSS_Console("start2(): started\n");

    mboxId = MboxCreate(5, 50);
    USLOSS_Console("start2(): MboxCreate returned id = %d\n", mboxId);

    privateMbox = MboxCreate(0, 50);
    USLOSS_Console("start2(): MboxCreate returned id = %d\n", privateMbox);

    kidpid = fork1("XXp1",  XXp1, NULL,    2 * USLOSS_MIN_STACK, 1);
    kidpid = fork1("XXp3",  XXp3, NULL,    2 * USLOSS_MIN_STACK, 2);

    kidpid = join(&kidStatus);
    USLOSS_Console("start2(): joined with kid %d, status = %d\n", kidpid, kidStatus);

    kidpid = join(&kidStatus);
    USLOSS_Console("start2(): joined with kid %d, status = %d\n", kidpid, kidStatus);

    quit(0);
}

int XXp1(char *arg)
{
    int i, result;
    char buffer[20];

    USLOSS_Console("\nXXp1(): started\n");

    for (i = 0; i < 8; i++) {
        USLOSS_Console("XXp1(): conditionally sending message #%d ", i);
        USLOSS_Console("to mailbox %d\n", mboxId);
        sprintf(buffer, "hello there, #%d", i);
        result = MboxCondSend(mboxId, buffer, strlen(buffer)+1);
        USLOSS_Console("XXp1(): after conditional send of message #%d, result = %d\n", i, result);
    }

    MboxRecv(privateMbox, NULL, 0);

    for (i = 0; i < 8; i++) {
        USLOSS_Console("XXp1(): conditionally sending message #%d ", i);
        USLOSS_Console("to mailbox %d\n", mboxId);
        sprintf(buffer, "good-bye, #%d", i);
        result = MboxCondSend(mboxId, buffer, strlen(buffer)+1);
        USLOSS_Console("XXp1(): after conditional send of message #%d, result = %d\n", i, result);
    }

    quit(3);
}

int XXp3(char *arg)
{
    char buffer[100];
    int i = 0, result, count;

    /* BUGFIX: initialize buffers to predictable contents */
    memset(buffer, 'x', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';

    USLOSS_Console("\nXXp3(): started\n");

    count = 0;
    while ( (result = MboxCondRecv(mboxId, buffer, 100)) >= 0 ) {
        USLOSS_Console("XXp3(): conditionally received message #%d ", i);
        USLOSS_Console("from mailbox %d   message = '%s'\n", mboxId, buffer);
        count++;
    }
    USLOSS_Console("XXp3(): received %d hello messages from mailbox %d\n", count, mboxId);

    MboxSend(privateMbox, NULL, 0);

    /* BUGFIX: initialize buffers to predictable contents */
    memset(buffer, 'x', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';

    count = 0;
    while ( (result = MboxCondRecv(mboxId, buffer, 100)) >= 0 ) {
        USLOSS_Console("XXp3(): conditionally received message #%d ", i);
        USLOSS_Console("from mailbox %d   message = '%s', result = %d\n", mboxId, buffer, result);
        count++;
    }
    USLOSS_Console("XXp3(): After loop, result is negative; result = %d\n", result);
    USLOSS_Console("XXp3(): received %d good-bye messages from mailbox %d\n", count, mboxId);

    quit(4);
}

