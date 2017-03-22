/* ------------------------------------------------------------------------
   phase2.c
   Version: 2.0

   Skeleton file for Phase 2. These routines are very incomplete and are
   intended to give you a starting point. Feel free to use this or not.

  Authors: Yan Ochoa,
   ------------------------------------------------------------------------ */

#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>

static int ClockDriver(void *arg);
int P2_Spawn(char *name, int(*func)(void *), void *arg, int stackSize, int priority);
int P2_Wait(int *status);
static P1_Semaphore running;
//added by me:
int launch_p(void *arg);

//-------User PCB
typedef struct {
int ret_status;
int cur_status;
int paren_pid;

} helper_PCB;

//-------mailbox
typedef struct Mailbox{
  P1_Semaphore full;
  P1_Semaphore mutex;
  P1_Semaphore empty;
  int mailboxId;
  int slots;
  int slotSize;

} Mailbox;

//globals
int global_parentID;
int parentPID;

int P2_Startup(void *arg){
    int               status;
    int               result = 1; // default is there was a problem
    int               rc;
    /*
     * Check kernel mode
     */

	if((USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet())==0){
	  USLOSS_Console("Not in kernel mode [p2_startup]");
	 goto done;
	}

    /*
     * Create clock device drivers.
     */
    rc = P1_SemCreate("running", 0, &running);
    if (rc != 0) {
        USLOSS_Console("P1_SemCreate of running failed: %d\n", rc);
        goto done;
    }
    for (int i = 0; i < USLOSS_CLOCK_UNITS; i++) {
        rc = P1_Fork("Clock driver", ClockDriver, (void *) i, USLOSS_MIN_STACK, 2, 0);
        if (rc < 0) {
            USLOSS_Console("Can't create clock driver\n");
            goto done;
        }
        /*
         * Wait for the clock driver to start.
         */
        rc = P1_P(running);
        if (rc != 0) {
            USLOSS_Console("P1_P(running) failed: %d\n", rc);
            goto done;
        }
    }
    /*
     * Create the other device drivers.
     */
    // ...

    /* 
     * Create initial user-level process. You'll need to check error codes, etc. P2_Spawn
     * and P2_Wait are assumed to be the kernel-level functions that implement the Spawn and 
     * Wait system calls, respectively (you can't invoke a system call from the kernel).
     */
    rc = P2_Spawn("P3_Startup", P3_Startup, NULL,  4 * USLOSS_MIN_STACK, 3);
    if (rc < 0) {
        USLOSS_Console("Spawn of P3_Startup failed: %d\n", rc);
    } else {
        rc = P2_Wait(&status);
        if (rc < 0) {
            USLOSS_Console("Wait for P3_Startup failed: %d\n", rc);
            goto done;
        }
    }


    /*
     * Make the device drivers abort using P1_WakeupDevice.
     */

    for (int i = 0; i < USLOSS_CLOCK_UNITS; i++) {
        rc = P1_WakeupDevice(USLOSS_CLOCK_DEV, i, 1);
        if (rc < 0) {
            USLOSS_Console("Wakeup of clock device driver failed : %d\n", rc);
            goto done;
        }
    }

    /*
     * Join with the device drivers.
     */


    result = 0;
done:
    return result;
}

//-----------------------------------------------------------
static int ClockDriver(void *arg)
{
    int unit = (int) arg;
    int result = 1; // default is there was a problem
    int dummy;
    int rc; // return codes from functions we call

    /*
     * Let the parent know we are running and enable interrupts.
     */
    rc = P1_V(running);
    if (rc != 0) {
        goto done;
    }
    rc = USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);
    if (rc != 0) {
        USLOSS_Console("ClockDriver: USLOSS_PsrSet returned %d\n", rc);
        goto done;
    }
    while(1) {
        /*
         * Read new sleep requests from the clock mailbox and update the bookkeeping appropriately.
         */
        rc = P1_WaitDevice(USLOSS_CLOCK_DEV, unit, &dummy);
        if (rc == -3) { // aborted
            break;
        } else if (rc != 0) {
            USLOSS_Console("ClockDriver: P1_WaitDevice returned %d\n", rc);
            goto done;
        }
        /*
         * Compute the current time and wake up any processes
         * that are done sleeping by sending them a response.
         */
	
    }
    result = 0; // if we get here then everything is ok
done:
    return rc;
}




//--------------------------------------------------------

//--------------------------------------------------------
int P2_Spawn(char *name, int (*func)(void *arg), void *arg, int stackSize, int priority) {
// USLOSS_Console("Entered P2_Spawn");
//check for a valid priority
  if((priority < 0) || (priority > 4)){
	return -3;
  }
 //check the stacksize
 if(stackSize < USLOSS_MIN_STACK){
  return -2;
  }



  //check the mode
  if((USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet())==0){
	  USLOSS_Console("Not in kernel mode [p2_Spawn]");
	 //goto done;
	}
//USLOSS_Console("about to run\n");

  //P1_P(running);
  parentPID = P1_GetPID();
  int newPID = P1_Fork(name,launch_p,arg,stackSize,priority, 0);
  //P1_V(running);
//USLOSS_Console("abt to return from P2_Spawn\n");
    return newPID;


}
//-------------------------------------------------------
//similar to P1_Quit except for user level processes
// returns status to P2_Wait called by its parent.
void P2_Terminate(int status){
  //P1_Quit()
  //P2_Wait(status);
}



//--------------------------------------------------------
//helper function launches func as user level
int launch_p(void *arg){
//set usermode


//use a for something
int a = USLOSS_PsrSet(USLOSS_PsrGet() & ~USLOSS_PSR_CURRENT_MODE);
  helper_PCB incoming_process;

  incoming_process.paren_pid = global_parentID;

return a;  //change ret value later
}

//------------------------------------------------------
//similar to P1_Join but for user level proesses.
// returns if a child was terminated. return the PID of the child process that exited .

int P2_Wait(int *status) 
{
    return -1;
}


//--------------------------------------------------
int P2_Sleep(int seconds){
  if(seconds < 0){
  return 1;
  }

  else{
  return 0;
  }

}

