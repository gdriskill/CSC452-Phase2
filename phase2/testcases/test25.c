/* start2 creates a mailbox with 5 slots, then creates XXp3 at priority 5.
 * XXp3 creates three instances of XXp2 at priorities 4, 2, 2 respectively.
 * Each instance of XXp2 blocks on the mail box.  The priorities of the
 * three instances of XXp2 that are blocked on the mailbox are 4, 2, 2.
 * XXp3 creates XXp1 at priority 1.
 * XXp3 sends three different messages to the mailbox.
 * Each instance of XXp2 prints the message is receives.  The priority 4
 * instance should get "First message", the two priority two instances
 * should be "Second message" and "Third message".
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <usloss.h>
#include <phase1.h>
#include <phase2.h>

int XXp1(char *);
int XXp2(char *);
int XXp3(char *);

int mbox_id;



int start2(char *arg)
{
    int kid_status, kidpid;

    USLOSS_Console("start2(): started\n");
    mbox_id = MboxCreate(5, MAX_MESSAGE);
    USLOSS_Console("\nstart2(): MboxCreate returned id = %d\n", mbox_id);

    kidpid = fork1("XXp3", XXp3, NULL, 2 * USLOSS_MIN_STACK, 4);

    kidpid = join(&kid_status);
    USLOSS_Console("\nstart2(): joined with kid %d, status = %d\n", kidpid, kid_status);

    quit(0);
}

int XXp1(char *arg)
{
    int i, result;

    USLOSS_Console("XXp1(): started\n");

    for (i = 1; i <= 3; i++) {
        USLOSS_Console("XXp1(): sending message #%d to mailbox %d\n", i, mbox_id);

        switch (i) {
        case 1:
            result = MboxSend(mbox_id, "First message", 14);
            break;
        case 2:
            result = MboxSend(mbox_id, "Second message", 15);
            break;
        case 3:
            result = MboxSend(mbox_id, "Third message", 14);
            break;
        default:
            USLOSS_Console("XXp1(): problem in switch!!!!\n");
        }

        USLOSS_Console("XXp1(): MboxSend() rc %d\n", result);
    }

    quit(1);
}

int XXp2(char *arg)
{
    char buffer[MAX_MESSAGE];
    int result;
    int my_priority = atoi(arg);

    /* BUGFIX: initialize buffers to predictable contents */
    memset(buffer, 'x', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';

    USLOSS_Console("XXp2(): started\n");

    USLOSS_Console("XXp2(): priority %d, receiving message from mailbox %d\n", my_priority, mbox_id);
    result = MboxRecv(mbox_id, buffer, MAX_MESSAGE);
    USLOSS_Console("XXp2(): priority %d, after receipt of message, result = %d   message = '%s'\n", my_priority, result, buffer);

    quit(my_priority);
}

int XXp3(char *arg)
{
    int kidpid, status, i;

    USLOSS_Console("\nXXp3(): started\n");

    USLOSS_Console("\nXXp3(): fork'ing XXp2 at priority 4\n");
    kidpid = fork1("XXp2", XXp2, "4", 2 * USLOSS_MIN_STACK, 4);

    USLOSS_Console("\nXXp3(): fork'ing XXp2 at priority 2\n");
    kidpid = fork1("XXp2", XXp2, "3", 2 * USLOSS_MIN_STACK, 3);

    USLOSS_Console("\nXXp3(): fork'ing XXp2 at priority 2\n");
    kidpid = fork1("XXp2", XXp2, "2", 2 * USLOSS_MIN_STACK, 2);

    USLOSS_Console("\nXXp3(): fork'ing XXp1 at priority 1\n");
    kidpid = fork1("XXp1", XXp1, NULL, 2 * USLOSS_MIN_STACK, 1);

    for (i = 0; i < 4; i++) {
        kidpid = join(&status);
        USLOSS_Console("XXp3(): join'd with child %d whose status is %d\n", kidpid, status);
    }

    quit(5);
}

