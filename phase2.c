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
static int DiskDriver(void *arg);
int P2_Spawn(char *name, int(*func)(void *), void *arg, int stackSize, int priority);
int * ptrsize = (int *)sizeof(int);
int P2_Wait(int *status);
void P2_Terminate(int status);
int P2_TermRead(int unit, int size, char *buffer);
int P2_Termwrite(int unit, int size, char *text);


static P1_Semaphore running;
//added by me:
int launch_p(void *arg);

//-------User PCB
typedef struct {
int ret_status;
int cur_status;
int paren_pid;
int operation;
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
helper_PCB helper_PCBs[P2_MAX_MBOX];
//globals
int global_parentID;
int waitStatus;
int parentPID;
int setter_user;
int pid_disk;
int *retrn;
int *status_global = 0;
int tag;
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


     //Initialize ULOSS_DISK_UNITS
	for(int i = 0; i< USLOSS_DISK_UNITS; i++){
	pid_disk = P1_Fork("Disk Driver", DiskDriver, (void *) i, USLOSS_MIN_STACK, 2, 0);
		if(pid_disk < 0){
		  USLOSS_Console("DiskDiver startup failed\n");
		  goto done;
		}
	pid_disk = P1_P(running);
	if(pid_disk < 0){
	  USLOSS_Console("DiskDiver addition to proc table failed\n");
	  goto done;
	 }
	}


	//Create the terminal device drivers
	for(int i=0; i < USLOSS_TERM_UNITS; i++){
	 //intitialize amb for each
	}


	// Initialize mailbox table
	int i;

      //	for every mailbox slot, initialize its Id to -1
	for (i=0; i < P2_MAX_MBOX; i++){
		mailboxes[i].mailboxId = -1;
	}


	tag = 0;
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

	P1_P(running);
	parentPID = P1_GetPID();
	int newPID = P1_Fork(name,launch_p,arg,stackSize,priority, 0);
	P1_V(running);

	USLOSS_Console("returning from P2_Spawn\n");
    return newPID;
}

//------------------------------------------------------
//similar to P1_Join but for user level proesses.
// returns if a child was terminated. return the PID of the child process that exited .
int P2_Wait(int *status) {
	int i;
    i = P1_Join(tag, (status+0));
    if(i == 0 ){
	//no children in the process
	return 0;
    }
    else{
	return i;
     }

}

//-------------------------------------------------------
//similar to P1_Quit except for user level processes
// returns status to P2_Wait called by its parent.
void P2_Terminate(int status){
  P1_Quit(status);

 waitStatus= P2_Wait(status_global);
}

//--------------------------------------------------------
//helper function launches func as user level
int launch_p(void *arg){


	//change mode first
	setter_user = USLOSS_PsrSet(USLOSS_PsrGet() & ~USLOSS_PSR_CURRENT_MODE);
	helper_PCB incoming_process;

	USLOSS_Console("Commensing startup\n");
	incoming_process.paren_pid = global_parentID;


	retrn = arg;
	//Sys_Terminate(retrn);


	return 0;  //change ret value later
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

int P2_MboxCreate(int slots, int size)
{
	int i;
	for (i = 0; i < P2_MAX_MBOX; i++){
		if (mailboxes[i].mailboxId == -1) {
			break;
		}
	}

	if (i == P2_MAX_MBOX) {
	return -1;
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

	mailboxes[i] = *newbox;

	return i;
}

int P2_MboxRelease(int mbox)
{
	if (mbox < 0 || mbox >= P2_MAX_MBOX || mailboxes[mbox].mailboxId == -1) {
		return -1;
	}

	P1_P(mailboxes[mbox].mutex);
	mailboxes[mbox].mailboxId = -1;

	return 0;
}

int
P2_MboxSend(int mbox, void *msg, int *size) {
	if (mbox < 0 || mbox >= P2_MAX_MBOX || mailboxes[mbox].mailboxId == -1) {
		return -1;
	}

	if (mailboxes[mbox].mailboxId ==  -1) {
		return -2;
	}

	Mailbox ptr = mailboxes[mbox];

	if (ptr.slotSize < *size) {
		return -1;
	}

	P1_P(ptr.full);
	P1_P(ptr.mutex);

	//ptr.messages[ptr.numMessages * ptr.slotSize] = msg;  //lucas needs to look at this, I cant debug it
	ptr.numMessages++;

	P1_V(ptr.mutex);
	P1_V(ptr.empty);

	return 0;
}

int P2_MboxCondSend(int mbox, void *msg, int *size){
	if (mbox < 0 || mbox >= P2_MAX_MBOX || mailboxes[mbox].mailboxId == -1) {
		return -1;
	}

	if (mailboxes[mbox].mailboxId == -1) {
		return -2;
	}

	Mailbox  ptr = mailboxes[mbox];

	if (ptr.slotSize < *size) {
		return -1;
	}

	if(ptr.numMessages == ptr.slots){
		return 1;
	}

	P1_P(ptr.full);
	P1_P(ptr.mutex);

	//ptr->message[ptr->numMessages * ptr->slotSize] = msg;  //lucas please debug or comment this line
	//ptr->numMessages++;

	P1_V(ptr.mutex);
	P1_V(ptr.empty);

	return 0;
}


int P2_MboxReceive(int mbox, void *msg, int *size) {
	if (mbox < 0 || mbox >= P2_MAX_MBOX || mailboxes[mbox].mailboxId == -1) {
		return -1;
	}	

	if (mailboxes[mbox].mailboxId == -1) {
		return -2;
	}

	Mailbox ptr = mailboxes[mbox];
	//int szze =  ptr.slots * ptr.slotSize;  //another bug, wont let it initialize with this size
	char msg_buff[100] = {'\0'};  // put 100 there to see if I could compile

	P1_P(ptr.empty);
	P1_P(ptr.mutex);

	//msg_buff = ptr.messages[ptr->numMessages * ptr->slotSize];  //another instance of a bug
	//char reset_buff[100] = {'\0'};
	//ptr.messages[ptr.numMessages * ptr.slotSize] = reset_buff;
	ptr.numMessages--;

	P1_V(ptr.full);
	P1_V(ptr.mutex);

	msg = (void *) msg_buff;

	int i;
	for (i = 0; i < ptr.slotSize && msg_buff[i] != '\0'; i++) {
	}

	*size = i;

	return 0;
}

int P2_MboxCondReceive(int mbox, void *msg, int *size){
	if (mbox < 0 || mbox >= P2_MAX_MBOX || mailboxes[mbox].mailboxId == -1) {
		return -1;
	}

	if (mailboxes[mbox].mailboxId == -1) {
		return -2;
	}

	Mailbox ptr = mailboxes[mbox];

	char msg_buff[100] = {'\0'};   //same error as before

	if (ptr.numMessages == 0){
		return 1;
	}

	P1_P(ptr.empty);
	P1_P(ptr.mutex);


        //everything below in this function is doing the thing descrived above
	//msg_buff = ptr->message[ptr->numMessages * ptr->slotSize];
	//char reset_buff[100] = {'\0'};
	//ptr->message[ptr->numMessages * ptr->slotSize] = reset_buff;
	ptr.numMessages--;

	P1_V(ptr.full);
	P1_V(ptr.mutex);

	msg = (void *) msg_buff;

	int i;
	for (i = 0; i < ptr.slotSize && msg_buff[i] != '\0'; i++) {
	}

	*size = i;

	return 0;
}



/*
P2_DiskRead-------------------------
read secors from the disk directed by unit.
track = starting track
first = first sector
buffer = where secors are copied to
*/
int P2_DiskRead(int unit, int track, int first, int sectors, void *buffer){
 //Validations:
int status =0;
 //check if kernel mode
  if((USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet())==0){
    USLOSS_Console("Not in kernel mode [p2_Spawn]");
    //goto done;
  return -1;
  }

 //check sector boundaries
 if((sectors < 0) || (sectors > USLOSS_DISK_TRACK_SIZE)){
  USLOSS_Console("Sector boundaries invalid\n");
  return -1;
 }

 //track check
 if(track < 0){
  USLOSS_Console("DiskRead Track out of bounds\n");
 // int iii = P1_getPid();
  return -1;
 }

 //buffer check, buffer may not be null
 if(buffer == NULL || sizeof((char *)buffer)< (sizeof(char) * sectors * USLOSS_DISK_SECTOR_SIZE)){
  USLOSS_Console("DiskRead buffer out of bounds\n");
  return -1;
 }

 //check mailbox is not not null

 //check first
 if(first < 0){
  USLOSS_Console("DiskRead 'first' parameter out of bounds\n");
  return -1;
 }
 //Create mailbox
// int diskMB = P2_MboxCreate(sectors, (track *4));

/*
 for(int i =0; i< sectors; i++){
   //for each sector, MbSend for each sector
   int responceMB = P2_MboxSend(diskMB, &status, ptrsize);

   helper_PCBs[i].operation = USLOSS_DISK_READ;

   //check if request is valid
   responceMB =P2_MboxReceive(diskMB, &status, ptrsize);

 }
   */
   
	USLOSS_DeviceRequest * request = (USLOSS_DeviceRequest *)malloc(sizeof(USLOSS_DeviceRequest));
	
	request->opr = USLOSS_DISK_SEEK;
	request->reg1 =(int *) track;
	
	int status2 = -1;
	while(status2 != USLOSS_DEV_READY) {
		int devx = USLOSS_DeviceInput(USLOSS_DISK_DEV, unit, &status2);
		if (status2 == USLOSS_DEV_ERROR || devx == -1){ 
			return status2;
		}
	} 
	
	int devx2 = USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, request);
	if(devx2 == -1){
		//something went wrong
	}
	int i, currSector = first, currTrack = track;;
	for (i = 0; i < sectors; i++, currSector++) {
		
		if (currSector >= USLOSS_DISK_TRACK_SIZE) {
			currSector = 0;
			
			request->opr = USLOSS_DISK_SEEK;
			request->reg1 = (int *) currTrack;
	
			status = -1;
			while(status != USLOSS_DEV_READY) {
				int devx= USLOSS_DeviceInput(USLOSS_DISK_DEV, unit, &status);
				if (status == USLOSS_DEV_ERROR || devx == -1) { 
					return status;
				}
			} 
	
			int devx = USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, request);
		  if(devx == -1){
			return -1;	
		}
		}
	
		request->opr = USLOSS_DISK_READ;
		request->reg1 = (int *) currSector;
		//request->reg2 = &buffer[USLOSS_DISK_SECTOR_SIZE * i];
		
		status = -1;
		while(status != USLOSS_DEV_READY) {
			int devx = USLOSS_DeviceInput(USLOSS_DISK_DEV, unit, &status);
			if (status == USLOSS_DEV_ERROR || devx == -1) { 
				return status;
			}
		} 
	
		int devx2 =USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, request);
		if(devx2 < 0 ){
		//something went wrong
		}
	}

   int devx =0;
return devx;
}

/*
----------------------P2_DiskWrite----------------------------------
read secors from the disk directed by unit.
track = starting track
first = first sector
buffer = where secors are copied to
*/
int P2_DiskWrite(int unit, int track, int first, int sectors, void *buffer){
//check the inputs
int status =0;
 //check if kernel mode
  if((USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet())==0){
    USLOSS_Console("Not in correctmode [p2_Spawn]");
    //goto done;
  return -1;
  }

 //check sector boundaries
 if((sectors < 0) || (sectors > USLOSS_DISK_TRACK_SIZE)){
  USLOSS_Console("Sector boundaries invalid\n");
  return -1;
 }

 //track check
 if(track < 0){
  return -1;
 }
  

 //buffer check, buffer may not be null
 if(buffer == NULL){
  //USLOSS_Console("DiskRead buffer out of bounds\n");
  return -1;
 }

 //check mailbox is not not null

 //check first
 if(first < 0){
  USLOSS_Console("DiskWrite 'first' parameter out of bounds\n");
  return -1;
 }
 //Create mailbox
 //int diskMB = P2_MboxCreate(sectors, (track *4));
 USLOSS_DeviceRequest * request = (USLOSS_DeviceRequest *)malloc(sizeof(USLOSS_DeviceRequest));
	
	request->opr = USLOSS_DISK_SEEK;
    request->reg1 = (int *)track;
/*
 for(int i =0; i< sectors; i++){
   //for each sector, MbSend for each sector
   int responceMB = P2_MboxSend(diskMB, &status, ptrsize);

   helper_PCBs[i].operation = USLOSS_DISK_WRITE;

   //check if request is valid
   responceMB =P2_MboxReceive(diskMB, &status, ptrsize);

 }
return 0;
 */
 
	int status2 = -1;
	while(status2 != USLOSS_DEV_READY) {
		int dev = USLOSS_DeviceInput(USLOSS_DISK_DEV, unit, &status2);
		if (status2 == USLOSS_DEV_ERROR || dev <0){ 
			return status2;
		}
	} 
	
	int dev2 =USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, request);
	if(dev2 <0){
	//no good
	}
	
	int i, currSector = first, currTrack = track;;
	for (i = 0; i < sectors; i++, currSector++) {
		
		if (currSector >= USLOSS_DISK_TRACK_SIZE) {
			currSector = 0;
			
			request->opr = USLOSS_DISK_SEEK;
			request->reg1 = (int *)currTrack;
	
			status = -1;
			while(status != USLOSS_DEV_READY) {
				int dev =USLOSS_DeviceInput(USLOSS_DISK_DEV, unit, &status);
				if (status == USLOSS_DEV_ERROR|| dev <0) { 
					return status;
				}
			} 
	
			int dev2 =USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, request);
		if(dev2 < 0){ //wait
		}
		}
	
		request->opr = USLOSS_DISK_WRITE;
		request->reg1 = (int *)currSector;
		//request->reg2 = &buffer[USLOSS_DISK_SECTOR_SIZE * i];
		
		int status3 = -1;
		while(status3 != USLOSS_DEV_READY) {
			int dev = USLOSS_DeviceInput(USLOSS_DISK_DEV, unit, &status3);
			if (status3 == USLOSS_DEV_ERROR || dev <0) { 
				return status3;
			}
		} 
	
		int dev2 = USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, request);
		if(dev2 <0){//asd
		}
		}
	
	
return 0;
 
}


//------------DiskDriver-----------------------------------

static int DiskDriver(void *arg){
 int *stat;
 int input= atoi((arg));
 int devOutput;
 if(input < 0){
 return -1;
 }

//create the request for the tracks based on the input
 USLOSS_DeviceRequest req = (USLOSS_DeviceRequest){
 .opr = USLOSS_DISK_TRACKS, .reg1 = &stat,
  };
// wait the device
 devOutput = USLOSS_DeviceOutput(USLOSS_DISK_DEV, input, &req);

//call wait device but not directly
 P1_WaitDevice(USLOSS_DISK_DEV, input, stat);


//if devOutput == 0: disk is done doing whatever it was doing 

 if(devOutput != 0){
  return -1;
 }
return 0;

}
//------P2_DiskSize--------
int P2_DiskSize(int unit, int *sector, int *track, int *disk) {
	
	if (unit < 0 || sector == NULL || track == NULL || disk == NULL) {
		return -1;
	}
	
	USLOSS_DeviceRequest * request = (USLOSS_DeviceRequest *)malloc(sizeof(USLOSS_DeviceRequest));
	
	request->opr = USLOSS_DISK_TRACKS;
	request->reg1 = disk;
	
	int status = -1;
	while(status != USLOSS_DEV_READY) {
		int dev = USLOSS_DeviceInput(USLOSS_DISK_DEV, unit, &status);
		if (status == USLOSS_DEV_ERROR || dev < 0){ 
			return status;
		}
	} 
	
	int dev2 = USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, request);
	if(dev2 <0 ){
	//wait
	}
	//*sector = USLOSS_SECTOR_SIZE;
	//*track = USLOSS_TRACK_SIZE;  //usloss saying these dont exist
	
	return 0;
}


/*
-----------P2_Term_Read--------------------

this routine automatically reads a line of text from the terminal indicated by unit
into the buffer of size bytes.

unit: line of text
buffer: place to put line of text
lines are delimited by '\n' in buffer
size must be >= 0 and <= P2_MAX_LINE
*/
int P2_TermRead(int unit, int size, char *buffer){
	int status = USLOSS_DEV_READY;
	int terminal2 = 0;
	int *terminal1 = NULL;
        char t[2];
	//validations:

	//check in kernel mode
	  if((USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet())==0){
           USLOSS_Console("Not in correct mode [p2_TermRead]");
           //goto done;
         return -1;
        }

	if((unit < 0) || (unit > USLOSS_TERM_UNITS)){
	 return -1;
	}

	if (size < 0 || size > P2_MAX_LINE) {
		return -1;
	}

	// Wait for a character to be transmitted to the terminal
	while (status == USLOSS_DEV_READY) {
	terminal2 = USLOSS_DeviceInput(USLOSS_TERM_DEV, unit, terminal1);
//		status = USLOSS_TERM_STAT_RECV(terminal);
	}

	if (status == USLOSS_DEV_ERROR) {
		return -1;
	}

	char curr_char = t[0];
	buffer[0] = curr_char;

	int i = 1;
	while (status == USLOSS_DEV_BUSY && i < size) {
		terminal2 =USLOSS_DeviceInput(USLOSS_TERM_DEV, unit, terminal1);
//		status = USLOSS_TERM_STAT_RECV(terminal);
		char curr_char = t[0];
		buffer[i] = curr_char;
	}

	size = i;
	return 0;
}

/*
----------P2_TermWrite-------------


*/

int P2_TermWrite(int unit, int size, char *text)  {
//	int status = USLOSS_DEV_READY;
	char terminal[2];
	int *terminal1;
	int retValTerm;
	if (size < 0 || size > P2_MAX_LINE) {
		return -1;
	}

	int i;
	for (i = 0; i < size; i++) {
		terminal[0] = text[i];
		terminal[1] = 0x1;
		retValTerm = USLOSS_DeviceOutput(USLOSS_TERM_DEV, unit, &terminal1);
	}

	size = i;
	return 0;
}

 


/*
int P2_TermRead(int unit, int size, char *buffer){

if(size > P2_MAX_LINE){
//line cant exceed p2_max_line
return -1;
}


}

int P2_Termwrite(int unit, int size, char *text){



}
*/

