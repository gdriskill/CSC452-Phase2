
/* start2 creates a mailbox.
 * start2 creates two processes: XXp1 at priority 3, XXp2 at priority 4.
 * XXp1 sends a message to the mailbox and then quits.
 * XXp2 receives a message from the mailbox, then quits.
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

  mbox_id = MboxCreate(10, 50);
  USLOSS_Console("start2(): MboxCreate returned id = %d\n", mbox_id);

  kidpid = fork1("XXp1", XXp1, NULL, 2 * USLOSS_MIN_STACK, 1);
  kidpid = fork1("XXp2", XXp2, NULL, 2 * USLOSS_MIN_STACK, 2);

  kidpid = join(&kid_status);
  USLOSS_Console("start2(): joined with kid %d, status = %d\n", kidpid, kid_status);

  kidpid = join(&kid_status);
  USLOSS_Console("start2(): joined with kid %d, status = %d\n", kidpid, kid_status);

  quit(0);
}


int XXp1(char *arg)
{
  int result;

  USLOSS_Console("XXp1(): started\n");

  USLOSS_Console("XXp1(): sending message to mailbox %d\n", mbox_id);
  result = MboxSend(mbox_id, "hello there", strlen("hello there")+1);
  USLOSS_Console("XXp1(): after send of message, result = %d\n", result);

  quit(3);
}


int XXp2(char *arg)
{
  char buffer[100];
  int result;

  /* BUGFIX: initialize buffers to predictable contents */
  memset(buffer, 'x', sizeof(buffer)-1);
  buffer[sizeof(buffer)-1] = '\0';

  USLOSS_Console("XXp2(): started\n");

  USLOSS_Console("XXp2(): receiving message from mailbox %d\n", mbox_id);
  result = MboxRecv(mbox_id, buffer, 100);
  USLOSS_Console("XXp2(): after receipt of message, result = %d    message = '%s'\n", result,buffer);

  quit(4);
}

