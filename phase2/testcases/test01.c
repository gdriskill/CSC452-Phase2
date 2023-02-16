
/* start2 creates two mailboxes, then quits
 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>



int start2(char *arg)
{
  int mbox_id;

  USLOSS_Console("start2(): started\n");

  mbox_id = MboxCreate(10, 50);
  USLOSS_Console("start2(): MailBoxCreate returned id = %d\n", mbox_id);
  mbox_id = MboxCreate(20, 30);
  USLOSS_Console("start2(): MailBoxCreate returned id = %d\n", mbox_id);

  quit(0);
}

