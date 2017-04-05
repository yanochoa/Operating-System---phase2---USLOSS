/* ------------------------------------------------------------------------
   phase2.c
   Version: 2.0

   Skeleton file for Phase 2. These routines are very incomplete and are
   intended to give you a starting point. Feel free to use this or not.

  Authors: Yan Ochoa, Lucas Robbins
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
	int numMessages;
	void * messages;
	
	//int id;
	//int slots;
	//int slotSize;
	//int openslots;

} Mailbox;

Mailbox mailboxes[P2_MAX_MBOX]; 

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
	
	// Initialize mailbox table
	int i;
	for (i=0; i < P2_MAX_MBOX; i++){
		mailboxes[i] = NULL;
	}

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

//------------------------------------------------------
//similar to P1_Join but for user level proesses.
// returns if a child was terminated. return the PID of the child process that exited .
int     
P2_Wait(int *status) 
{
    return -1;
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


//--------------------------------------------------
int P2_Sleep(int seconds){
  if(seconds < 0){
  return 1;
  }

  else{
  return 0;
  }

}

int
P2_MboxCreate(int slots, int size)
{
	int i;
	for (i = 0; i < P2_MAX_MBOX; i++){
		if (mailboxes[i] == NULL) {
			break;
		}
	}
	
	if (i == P2_MAX_MBOX) {
		return -1
	}
	
	Mailbox * newbox = (Mailbox *)malloc(sizeof(Mailbox));
	P1_SemCreate("full", size, &newbox->full);
	P1_SemCreate("mutex", 1, &newbox->mutex);
	P1_SemCreate("empty", size, &newbox->empty);
	newbox->slots = slots;
	newbox->slotSize = size;
	newbox->mailboxId = i;
	newbox->numMessages = 0;
	newbox->messages = (void *) malloc(sizeof(char) * size * slots);
	
	mailboxes[i] = newbox;
	
	return i;
}

int 
P2_MboxRelease(int mbox)
{
	if (mbox < 0 || mbox >= P2_MAX_MBOX || mailboxes[mbox]==NULL) {
		return -1;
	}
	
	P1_P(mailboxes[mbox]->mutex);
	mailboxes[mbox] = NULL;
	
	return 0;
}

int
P2_MboxSend(int mbox, void *msg, int *size) 
{
	if (mbox < 0 || mbox >= P2_MAX_MBOX || mailboxes[mbox]==NULL) {
		return -1;
	}
	
	if (mailboxes[mbox] == NULL) {
		return -2;
	}
	
	Mailbox * ptr = mailboxes[mailbox];
	
	if (ptr->slotSize < *size) {
		return -1;
	}
	
	P1_P(ptr->full);
	P1_P(ptr->mutex);
	
	ptr->message[ptr->numMessages * ptr->slotSize] = msg;
	ptr->numMessages++;
	
	P1_V(ptr->mutex);
	P1_V(ptr->empty);
	
	return 0;
}

int
P2_MboxCondSend(int mbox, void *msg, int *size)
{
	if (mbox < 0 || mbox >= P2_MAX_MBOX || mailboxes[mbox]==NULL) {
		return -1;
	}
	
	if (mailboxes[mbox] == NULL) {
		return -2;
	}
	
	Mailbox * ptr = mailboxes[mailbox];
	
	if (ptr->slotSize < *size) {
		return -1;
	}
	
	if(ptr->numMessages == ptr->slots){
		return 1;
	}
	
	P1_P(ptr->full);
	P1_P(ptr->mutex);
	
	ptr->message[ptr->numMessages * ptr->slotSize] = msg;
	ptr->numMessages++;
	
	P1_V(ptr->mutex);
	P1_V(ptr->empty);
	
	return 0;
}


int
P2_MboxReceive(int mbox, void *msg, int *size)
{
	if (mbox < 0 || mbox >= P2_MAX_MBOX || mailboxes[mbox]==NULL) {
		return -1;
	}	
	
	if (mailboxes[mbox] == NULL) {
		return -2;
	}
	
	Mailbox * ptr = mailboxes[mailbox];
	
	char msg_buff[ptr->slots * ptr->slotSize] = {'\0'};
	
	P1_P(ptr->empty);
	P1_P(ptr->mutex);
	
	msg_buff = ptr->message[ptr->numMessages * ptr->slotSize];
	char reset_buff[ptr->slots * ptr->slotSize] = {'\0'};
	ptr->message[ptr->numMessages * ptr->slotSize] = reset_buff;
	ptr->numMessages--;
	
	P1_V(ptr->full);
	p1_V(ptr->mutex);
	
	msg = (void *) msg_buff;

	int i;
	for (i = 0; i < ptr->slotSize && msg_buff[i] != '\0'; i++) {
	}
	
	*size = i;
	
	return 0;
}

int
P2_MboxCondReceive(int mbox, void *msg, int *size)
{
	if (mbox < 0 || mbox >= P2_MAX_MBOX || mailboxes[mbox]==NULL) {
		return -1;
	}
	
	if (mailboxes[mbox] == NULL) {
		return -2;
	}
	
	Mailbox * ptr = mailboxes[mailbox];
	
	char msg_buff[ptr->slots * ptr->slotSize] = {'\0'};
	
	if (numMessages == 0){
		return 1;
	}
	
	P1_P(ptr->empty);
	P1_P(ptr->mutex);
	
	msg_buff = ptr->message[ptr->numMessages * ptr->slotSize];
	char reset_buff[ptr->slots * ptr->slotSize] = {'\0'};
	ptr->message[ptr->numMessages * ptr->slotSize] = reset_buff;
	ptr->numMessages--;
	
	P1_V(ptr->full);
	p1_V(ptr->mutex);
	
	msg = (void *) msg_buff;

	int i;
	for (i = 0; i < ptr->slotSize && msg_buff[i] != '\0'; i++) {
	}
	
	*size = i;
	
	return 0;
}


int
P2_TermRead(int unit, int size, char *buffer) 
{
	int status = USLOSS_DEV_READY;
	char terminal[2];
	
	if (size < 0 || size > P2_MAX_LINE) {
		return -1;
	}
	
		// Wait for a character to be transmitted to the terminal
	while (status == USLOSS_DEV_READY) {
		USLOSS_DeviceInput(USLOSS_TERM_DEV, unit, &terminal);
		status = USLOSS_TERM_STAT_RECV(terminal);
	}
	
	if (status == USLOSS_DEV_ERROR) {
		return -1;
	}
	
	char curr_char = terminal[0];
	buffer[0] = curr_char;
	
	i = 1;
	while (status == USLOSS_DEV_BUSY && i < size) {
		USLOSS_DeviceInput(USLOSS_TERM_DEV, unit, terminal);
		status = USLOSS_TERM_STAT_RECV(terminal);
		char curr_char = terminal[0];
		buffer[i] = curr_char;
	}
	
	size = i;
	return 0;
}

void
P2_TermWrite(int unit, int size, char *text) 
{
	int status = USLOSS_DEV_READY;
	char terminal[2];
	
	if (size < 0 || size > P2_MAX_LINE) {
		return -1;
	}
	
	int i;
	for (i = 0; i < size; i++) {
		terminal[0] = text[i];
		terminal[1] = 0x1;
		USLOSS_DeviceOutput(USLOSS_TERM_DEV, unit, &terminal);
	}
	
	size = i;
	return 0;
}

int 
P2_DiskRead(int unit, int track, int first, int sectors, void *buffer) {
	
	if (first < 0 || sectors < 0 || sectors > 4 * USLOSS_DISK_TRACK_SIZE) {
		return -1;
	}
	
	if (buffer == NULL || sizeof((char*)buffer) < (sizeof(char) * sectors * USLOSS_DISK_SECTOR_SIZE)) {
		return -1;
	}
	
	USLOSS_DeviceRequest * request = (USLOSS_DeviceRequest *)malloc(sizeof(USLOSS_DeviceRequest));
	
	request->opr = USLOSS_DISK_SEEK;
	request->reg1 = track;
	
	int status = -1;
	while(status != USLOSS_DEV_READY) {
		USLOSS_DeviceInput(USLOSS_DISK_DEV, unit, &status);
		if (status == USLOSS_DEV_ERROR){ 
			return status;
		}
	} 
	
	USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, request);
	
	int i, currSector = first, currTrack = track;;
	for (i = 0; i < sectors; i++, currSector++) {
		
		if (currSector >= USLOSS_DISK_TRACK_SIZE) {
			currSector = 0;
			
			request->opr = USLOSS_DISK_SEEK;
			request->reg1 = currTrack;
	
			status = -1;
			while(status != USLOSS_DEV_READY) {
				USLOSS_DeviceInput(USLOSS_DISK_DEV, unit, &status);
				if (status == USLOSS_DEV_ERROR) { 
					return status;
				}
			} 
	
			USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, request);
		}
	
		request->opr = USLOSS_DISK_READ;
		request->reg1 = currSector;
		request->reg2 = &buffer[USLOSS_DISK_SECTOR_SIZE * i];
		
		status = -1;
		while(status != USLOSS_DEV_READY) {
			USLOSS_DeviceInput(USLOSS_DISK_DEV, unit, &status);
			if (status == USLOSS_DEV_ERROR) { 
				return status;
			}
		} 
	
		USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, request);
	}
	
	return 0;
}

int 
P2_DiskWrite(int unit, int track, int first, int sectors, void *buffer) {
	
	if (first < 0 || sectors < 0 || sectors > 4 * USLOSS_DISK_TRACK_SIZE) {
		return -1;
	}
	
	if (buffer == NULL || sizeof((char*)buffer) < (sizeof(char) * sectors * USLOSS_DISK_SECTOR_SIZE)) {
		return -1;
	}
	
	
	USLOSS_DeviceRequest * request = (USLOSS_DeviceRequest *)malloc(sizeof(USLOSS_DeviceRequest));
	
	request->opr = USLOSS_DISK_SEEK;
	request->reg1 = track;
	
	int status = -1;
	while(status != USLOSS_DEV_READY) {
		USLOSS_DeviceInput(USLOSS_DISK_DEV, unit, &status);
		if (status == USLOSS_DEV_ERROR){ 
			return status;
		}
	} 
	
	USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, request);
	
	
	int i, currSector = first, currTrack = track;;
	for (i = 0; i < sectors; i++, currSector++) {
		
		if (currSector >= USLOSS_DISK_TRACK_SIZE) {
			currSector = 0;
			
			request->opr = USLOSS_DISK_SEEK;
			request->reg1 = currTrack;
	
			status = -1;
			while(status != USLOSS_DEV_READY) {
				USLOSS_DeviceInput(USLOSS_DISK_DEV, unit, &status);
				if (status == USLOSS_DEV_ERROR) { 
					return status;
				}
			} 
	
			USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, request);
		}
	
		request->opr = USLOSS_DISK_WRITE;
		request->reg1 = currSector;
		request->reg2 = &buffer[USLOSS_DISK_SECTOR_SIZE * i];
		
		status = -1;
		while(status != USLOSS_DEV_READY) {
			USLOSS_DeviceInput(USLOSS_DISK_DEV, unit, &status);
			if (status == USLOSS_DEV_ERROR) { 
				return status;
			}
		} 
	
		USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, request);
	}
	
	
	return 0;
}

int 
P2_DiskSize(int unit, int *sector, int *track, int *disk) {
	
	if (unit < 0 || sector == NULL || track == NULL || disk == NULL) {
		return -1;
	}
	
	USLOSS_DeviceRequest * request = (USLOSS_DeviceRequest *)malloc(sizeof(USLOSS_DeviceRequest));
	
	request->opr = USLOSS_DISK_TRACKS;
	request->reg1 = disk;
	
	int status = -1;
	while(status != USLOSS_DEV_READY) {
		USLOSS_DeviceInput(USLOSS_DISK_DEV, unit, &status);
		if (status == USLOSS_DEV_ERROR){ 
			return status;
		}
	} 
	
	USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, request);
	
	*sector = USLOSS_SECTOR_SIZE;
	*track = USLOSS_TRACK_SIZE;
	
	return 0;
}
