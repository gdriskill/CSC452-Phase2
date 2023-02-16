
/* A test of waitDevice for a terminal.  Can be used to test other
 * three terminals as well.
 */

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
    int  kid_status, kidpid;
    long control = 0;
    int  result;

    USLOSS_Console("start2(): started\n");

    /* see macro definition in usloss.h */
    control = USLOSS_TERM_CTRL_RECV_INT(control);

    USLOSS_Console("start2(): calling USLOSS_DeviceOutput to enable receive interrupts, control = %d\n", control);

    result = USLOSS_DeviceOutput(USLOSS_TERM_DEV, 1, (void *)control);
    if ( result != USLOSS_DEV_OK ) {
        USLOSS_Console("start2(): USLOSS_DeviceOutput returned %d ", result);
        USLOSS_Console("Halting...\n");
        USLOSS_Halt(1);
    }

    kidpid = fork1("XXp1", XXp1, NULL, 2 * USLOSS_MIN_STACK, 2);

    kidpid = join(&kid_status);
    USLOSS_Console("start2(): joined with kid %d, status = %d\n", kidpid, kid_status);

    quit(0);
}

int XXp1(char *arg)
{
    int status;

    USLOSS_Console("XXp1(): started, calling waitDevice for terminal 1\n");

    waitDevice(USLOSS_TERM_DEV, 1, &status);
    USLOSS_Console("XXp1(): after waitDevice call\n");

    USLOSS_Console("XXp1(): status = %d\n", status);

    USLOSS_Console("XXp1(): receive status for terminal 1 = %d\n", USLOSS_TERM_STAT_RECV(status));
    USLOSS_Console("XXp1(): character received = %c\n", USLOSS_TERM_STAT_CHAR(status));

    quit(3);
}

