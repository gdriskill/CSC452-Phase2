
/* start2 creates a mailbox, sends a message to the mailbox,
 * then receives the message.
 */

#include <stdio.h>
#include <memory.h>

#include <usloss.h>
#include <phase1.h>
#include <phase2.h>

char buf[256];



int start2(char *arg)
{
  int mbox_id;
  int result;
  char buffer[80];

  /* BUGFIX: initialize buffers to predictable contents */
  memset(buffer, 'x', sizeof(buffer)-1);
  buffer[sizeof(buffer)-1] = '\0';

  USLOSS_Console("start2(): started\n");
  mbox_id = MboxCreate(10, 50);
  USLOSS_Console("start2(): MboxCreate returned id = %d\n", mbox_id);

  USLOSS_Console("start2(): sending message to mailbox %d\n", mbox_id);
  result = MboxSend(mbox_id, "hello there", 12);
  USLOSS_Console("start2(): after send of message, result = %d\n", result);

  USLOSS_Console("start2(): attempting to receive message from mailbox %d\n", mbox_id);
  result = MboxRecv(mbox_id, buffer, 80);
  USLOSS_Console("start2(): after receive of message, result = %d\n", result);
  USLOSS_Console("          message is '%s'\n", buffer);

  quit(0);
}

