
/* Test of conditional send and conditional receive that does not rely
 * on a zero-slot mailbox.  Similar to test12.c, but uses a 1-slot mailbox
 * called "pauseMbox" instead of the 0-slot privateMbox.
 */

#include <stdio.h>
#include <string.h>

#include <usloss.h>
#include <phase1.h>
#include <phase2.h>

int XXp1(char *);
int XXp3(char *);
char buf[256];

int mboxId, pauseMbox;



int start2(char *arg)
{
    int kidStatus, kidpid;

    USLOSS_Console("start2(): started\n");

    mboxId = MboxCreate(5, 50);
    USLOSS_Console("start2(): MboxCreate returned id = %d\n", mboxId);

    pauseMbox = MboxCreate(1, 50);
    USLOSS_Console("start2(): MboxCreate returned id = %d\n", pauseMbox);

    kidpid = fork1("XXp1",  XXp1, NULL, 2 * USLOSS_MIN_STACK, 1);
    kidpid = fork1("XXp3",  XXp3, NULL, 2 * USLOSS_MIN_STACK, 2);

    kidpid = join(&kidStatus);
    USLOSS_Console("\nstart2(): joined with kid %d, status = %d\n",
                   kidpid, kidStatus);

    kidpid = join(&kidStatus);
    USLOSS_Console("\nstart2(): joined with kid %d, status = %d\n",
                   kidpid, kidStatus);

    quit(0);
}

int XXp1(char *arg)
{
    int i, result;
    char buffer[20];
    int  msgNum = 0;

    USLOSS_Console("\nXXp1(): started\n");

    for (i = 0; i < 8; i++) {
        USLOSS_Console("\nXXp1(): conditionally sending message #%d to mailbox %d\n", msgNum, mboxId);
        sprintf(buffer, "hello there, #%d", msgNum);
        result = MboxCondSend(mboxId, buffer, strlen(buffer)+1);
        USLOSS_Console("XXp1(): after conditional send of message #%d, result = %d\n", msgNum, result);
        msgNum++;
    }

    MboxRecv(pauseMbox, NULL, 0); // should block on mail box

    for (i = 0; i < 8; i++) {
        USLOSS_Console("\nXXp1(): conditionally sending message #%d to mailbox %d\n", msgNum, mboxId);
        sprintf(buffer, "good-bye, #%d", i);
        result = MboxCondSend(mboxId, buffer, strlen(buffer)+1);
        USLOSS_Console("XXp1(): after conditional send of message #%d, result = %d\n", msgNum, result);
        msgNum++;
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
        USLOSS_Console("XXp3(): conditionally received message #%d from mailbox %d   message = '%s'\n", i, mboxId, buffer);
        count++;
    }
    USLOSS_Console("XXp3(): After loop, result is negative; result = %d\n", result);
    USLOSS_Console("XXp3(): received %d hello messages from mailbox %d\n", count, mboxId);

    MboxSend(pauseMbox, NULL, 0); // should release XXp1

    count = 0;
    while ( (result = MboxCondRecv(mboxId, buffer, 100)) >= 0 ) {
        USLOSS_Console("XXp3(): conditionally received message #%d from mailbox %d   message = '%s', result = %d\n", i, mboxId, buffer, result);
        count++;
    }
    USLOSS_Console("XXp3(): After loop, result is negative; result = %d\n", result);
    USLOSS_Console("XXp3(): received %d good-bye messages from mailbox %d\n", count, mboxId);

    quit(4);
}

