
/* A test of waitDevice for the clock */

#include <stdio.h>
#include <string.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>

int XXp1(char *);
int XXp3(char *);
char buf[256];



int start2(char *arg)
{
    int kid_status, kidpid;

    USLOSS_Console("start2(): at beginning, pid = %d\n", getpid());

    kidpid = fork1("XXp1", XXp1, NULL, 2 * USLOSS_MIN_STACK, 2);
    USLOSS_Console("start2(): fork'd child %d\n", kidpid);

    kidpid = join(&kid_status);
    USLOSS_Console("start2(): joined with kid %d, status = %d\n", kidpid, kid_status);

    quit(0);
}

int XXp1(char *arg)
{
    int status;

    USLOSS_Console("XXp1(): started, calling waitDevice for clock\n");

    /* 1st clock tick */
    waitDevice(USLOSS_CLOCK_DEV, 0, &status);
    USLOSS_Console("XXp1(): after waitDevice call\n");

    USLOSS_Console("XXp1(): About to print status from clock Handler:\n");
    USLOSS_Console("XXp1(): status = %d   -- Should be near 100k, but does not have to be an exact match.  Is often 90k or so on Russ's computer.\n", status);

    /* 2nd clock tick */
    waitDevice(USLOSS_CLOCK_DEV, 0, &status);
    USLOSS_Console("XXp1(): after waitDevice call\n");

    USLOSS_Console("XXp1(): About to print status from clock Handler:\n");
    USLOSS_Console("XXp1(): status = %d   -- Should be roughly 100k more than the previous report.\n", status);

    quit(3);
}

