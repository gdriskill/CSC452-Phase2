/* test releasing an empty mailbox with a number of blocked receivers (all of
 * various lower priorities than the releaser), and then trying to receive
 * and send to the now unused mailbox.
 */


#include <stdio.h>
#include <string.h>

#include <usloss.h>
#include <phase1.h>
#include <phase2.h>

int XXp1(char *);
int XXp2(char *);
int XXp3(char *);
int XXp4(char *);
char buf[256];

int mbox_id;



int start2(char *arg)
{
    int kid_status, kidpid, pausepid;

    USLOSS_Console("start2(): started\n");

    mbox_id = MboxCreate(5, 50);
    USLOSS_Console("start2(): MboxCreate returned id = %d\n", mbox_id);

    kidpid = fork1("XXp1",  XXp1, NULL,    2 * USLOSS_MIN_STACK, 4);
    kidpid = fork1("XXp2a", XXp2, "XXp2a", 2 * USLOSS_MIN_STACK, 3);
    kidpid = fork1("XXp2b", XXp2, "XXp2b", 2 * USLOSS_MIN_STACK, 2);
    kidpid = fork1("XXp2c", XXp2, "XXp2c", 2 * USLOSS_MIN_STACK, 1);
    /* mailbox queue -> 7 -> ->6 ->5 -> 4 when released they all get released
     * then dispatcher called will run again in order 7, 6, 5, 4
     */

    pausepid = fork1("XXp4",  XXp4, "XXp4",  2 * USLOSS_MIN_STACK, 4);
    kidpid = join(&kid_status);
    if (kidpid != pausepid)
        USLOSS_Console("\n***Test Failed*** -- join with pausepid failed!\n\n");

    kidpid = fork1("XXp3",  XXp3, NULL,    2 * USLOSS_MIN_STACK, 1);

    kidpid = join(&kid_status);
    USLOSS_Console("\nstart2(): joined with kid %d, status = %d\n", kidpid, kid_status);

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

int XXp1(char *arg)
{
    int i, result;
    char buffer[20];

    /* BUGFIX: initialize buffers to predictable contents */
    memset(buffer, 'x', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';

    USLOSS_Console("\nXXp1(): started\n");

    for (i = 0; i < 5; i++) {
        USLOSS_Console("XXp1(): receiving message #%d from mailbox %d\n", i, mbox_id);
        result = MboxRecv(mbox_id, buffer, 20);
        USLOSS_Console("XXp1(): after receive of message #%d, result = %d\n", i, result);
    }

    for (i = 0; i < 5; i++) {
        USLOSS_Console("XXp1(): sending message #%d to mailbox %d\n", i, mbox_id);
        sprintf(buffer, "hello there, #%d", i);
        result = MboxSend(mbox_id, buffer, strlen(buffer)+1);
        USLOSS_Console("XXp1(): after send of message #%d, result = %d\n", i, result);
    }

    quit(3);
}

int XXp2(char *arg)
{
    int result;
    char buffer[20];

    /* BUGFIX: initialize buffers to predictable contents */
    memset(buffer, 'x', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';

    USLOSS_Console("%s(): receive message from mailbox %d, msg_size = %d\n", arg, mbox_id, 20);
    result = MboxRecv(mbox_id, buffer, 20);
    USLOSS_Console("%s(): after receive of message result = %d\n", arg, result);

    if (result == -3)
        USLOSS_Console("%s(): mailbox destroyed by MboxRecv() call\n", arg);

    quit(4);
}

int XXp3(char *arg)
{
    int result;

    USLOSS_Console("\nXXp3(): started\n");

    result = MboxRelease(mbox_id);
    USLOSS_Console("XXp3(): MboxRelease returned %d\n", result);

    quit(5);
}

int XXp4(char *arg)
{
    USLOSS_Console("\nXXp4(): started and quitting\n");
    quit(6);
}

