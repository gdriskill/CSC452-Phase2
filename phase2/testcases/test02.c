
/* start2 attempts to create MAXMBOX + 3 mailboxes.
 * the last 3 should return -1
 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>



int start2(char *arg)
{
  int mbox_id;
  int i;

  USLOSS_Console("start2(): started, trying to create too many mailboxes.  Exactly 3 should fail.\n");
  for (i = 0; i < MAXMBOX + 3; i++) {
    mbox_id = MboxCreate(10, 50);
    if (mbox_id < 0)
      USLOSS_Console("start2(): MailBoxCreate returned id less than zero, id = %d\n", mbox_id);
  }

  quit(0);
}

