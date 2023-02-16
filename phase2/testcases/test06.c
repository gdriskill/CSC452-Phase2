
/* Creates two children.  Higher priority child does 6 sends to a mailbox
 * with 5 slots.  It should block when sending the last message.
 *
 * Lower priority child then does 6 receives.  As soon as the first receive
 * is done, the higher priority child is unblocked from the mailbox.
 * Remainder of receives then execute.
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

   mbox_id = MboxCreate(5, 50);
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
   int i, result;
   char buffer[20];

   USLOSS_Console("XXp1(): started\n");

   for (i = 0; i <= 5; i++) {
      USLOSS_Console("XXp1(): sending message #%d to mailbox %d\n", i, mbox_id);
      sprintf(buffer, "hello there, #%d", i);
      result = MboxSend(mbox_id, buffer, strlen(buffer)+1);
      USLOSS_Console("XXp1(): after send of message #%d, result = %d\n", i, result);
   }

   quit(3);
}


int XXp2(char *arg)
{
  char buffer[100];
  int i, result;

  /* BUGFIX: initialize buffers to predictable contents */
  memset(buffer, 'x', sizeof(buffer)-1);
  buffer[sizeof(buffer)-1] = '\0';

  USLOSS_Console("XXp2(): started\n");

  for (i = 0; i <= 5; i++) {
     USLOSS_Console("XXp2(): receiving message #%d from mailbox %d\n", i, mbox_id);
     result = MboxRecv(mbox_id, buffer, 100);
     USLOSS_Console("XXp2(): after receipt of message, result = %d    message = '%s'\n", result,buffer);
  }

  quit(4);
}

