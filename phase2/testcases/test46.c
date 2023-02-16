
/* This test case creates a single mailbox, has 3 high priority processes block
 * on it, and then a lower priority process releases it.  The release SHOULD
 * unblock the other processes.
 */

#include <phase1.h>
#include <phase2.h>
#include <usloss.h>
#include <stdio.h>

int Child1(char *);
int Child2(char *);

int mailbox;



int start2(char *arg)
{
    int kidPid;

    USLOSS_Console("start2(): started.  Creating mailbox.\n");

    mailbox = MboxCreate(0, 0);
    if (mailbox == -1) {
        USLOSS_Console("start2(): got non-zero mailbox result. Quitting...\n");
        quit(1);
    }

    USLOSS_Console("\nstart2(): calling fork1 for Child1a\n");
    fork1("Child1a", Child1, "Child1a", USLOSS_MIN_STACK, 1);

    USLOSS_Console("start2(): calling fork1 for Child1b\n");
    fork1("Child1b", Child1, "Child1b", USLOSS_MIN_STACK, 1);

    USLOSS_Console("start2(): calling fork1 for Child1c\n");
    fork1("Child1c", Child1, "Child1c", USLOSS_MIN_STACK, 1);

    USLOSS_Console("start2(): calling fork1 for Child2\n");
    fork1("Child2", Child2, NULL, USLOSS_MIN_STACK, 2);

    USLOSS_Console("\nstart2(): Parent done forking.\n");

    int status;

    kidPid = join(&status);
    USLOSS_Console("Process %d joined with status: %d\n\n", kidPid, status);

    kidPid = join(&status);
    USLOSS_Console("Process %d joined with status: %d\n\n", kidPid, status);

    kidPid = join(&status);
    USLOSS_Console("Process %d joined with status: %d\n\n", kidPid, status);

    kidPid = join(&status);
    USLOSS_Console("Process %d joined with status: %d\n\n", kidPid, status);

    quit(0);
}

int Child1(char *arg)
{
    int result;

    USLOSS_Console("\n%s(): starting, blocking on a mailbox receive\n", arg);
    result = MboxRecv(mailbox, NULL, 0);
    USLOSS_Console("%s(): result = %d\n", arg, result);

    return 1;
}

int Child2(char *arg)
{
    USLOSS_Console("\nChild2(): starting, releasing mailbox\n\n");
    MboxRelease(mailbox);
    USLOSS_Console("Child2(): done\n");

    return 9;
}

