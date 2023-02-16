
/* test border cases on MboxCreate */

#include <stdio.h>
#include <memory.h>

#include <usloss.h>
#include <phase1.h>
#include <phase2.h>




int start2(char *arg)
{
    int mbox_id;
    char buffer[80];
    int result;

    /* BUGFIX: initialize buffers to predictable contents */
    memset(buffer, 'x', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';

    USLOSS_Console("start2(): started, creating with size of slot too big\n");
    mbox_id = MboxCreate(10, MAX_MESSAGE + 1);
    USLOSS_Console("start2(): MailBoxCreate returned id = %d\n", mbox_id);
    if (mbox_id == -1) {
        USLOSS_Console("start2(): passed...\n");
    }
    else {
        USLOSS_Console("start2(): failed...\n");
        quit(1);
    }

    USLOSS_Console("\nstart2(): creating with size of slot = -1\n");
    mbox_id = MboxCreate(10, -1);
    USLOSS_Console("start2(): MailBoxCreate returned id = %d\n", mbox_id);
    if (mbox_id == -1) {
        USLOSS_Console("start2(): passed...\n");
    }
    else {
        USLOSS_Console("start2(): failed...\n");
        quit(1);
    }

    USLOSS_Console("\nstart2(): creating with number of slots = -1\n");
    mbox_id = MboxCreate(-1, 10);
    USLOSS_Console("start2(): MailBoxCreate returned id = %d\n", mbox_id);
    if (mbox_id == -1) {
        USLOSS_Console("start2(): passed...\n");
    }
    else {
        USLOSS_Console("start2(): failed...\n");
        quit(1);
    }

    USLOSS_Console("\nstart2(): creating with number of slots = 2 ");
    USLOSS_Console("and size of slots = 0\n");
    mbox_id = MboxCreate(2, 0);
    USLOSS_Console("start2(): MailBoxCreate returned id = %d\n", mbox_id);
    if (mbox_id == 7 || mbox_id == 0) {  // 7 for groups, 0 for solo
        USLOSS_Console("start2(): passed...\n");
    }
    else {
        USLOSS_Console("start2(): failed...\n");
        quit(1);
    }

    USLOSS_Console("\nstart2(): sending message to mailbox %d\n", mbox_id);
    result = MboxSend(mbox_id, "hello there", 0);
    USLOSS_Console("start2(): after send of message, result = %d\n", result);

    if (result == 0) {
        USLOSS_Console("start2(): passed...\n");
    }
    else {
        USLOSS_Console("start2(): failed...\n");
        quit(1);
    }

    USLOSS_Console("\nstart2(): sending message to mailbox %d\n", mbox_id);
    result = MboxSend(mbox_id, NULL, 0);
    USLOSS_Console("start2(): after send of message, result = %d\n", result);

    if (result == 0) {
        USLOSS_Console("start2(): passed...\n");
    }
    else {
        USLOSS_Console("start2(): failed...\n");
        quit(1);
    }

    USLOSS_Console("\nstart2(): attempting to receive message from ");
    USLOSS_Console("mailbox %d\n", mbox_id);
    result = MboxRecv(mbox_id, NULL, 0);
    USLOSS_Console("start2(): after receive of message, result = %d\n", result);
    if (result == 0) {
        USLOSS_Console("start2(): passed...\n");
    }
    else {
        USLOSS_Console("start2(): failed...\n");
        quit(1);
    }

    USLOSS_Console("\nstart2(): attempting to receive message from ");
    USLOSS_Console("mailbox %d\n", mbox_id);
    result = MboxRecv(mbox_id, buffer, 80);
    USLOSS_Console("start2(): after receive of message, result = %d\n", result);
    if (result == 0) {
        USLOSS_Console("start2(): passed...\n");
    }
    else {
        USLOSS_Console("start2(): failed...\n");
        quit(1);
    }

    quit(0);
}

