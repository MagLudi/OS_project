/* fio.c contains general utility functions
 *
 * FioErrno fioInit(void) -  initialized the file system inode table
 */

/* sys include files */
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

/* project headers   */
#include "pcb.h"
#include "mem.h"
#include "fio.h"
#include "fioutl.h"
#include "led.h"
#include "pushbutton.h"
#include "uart.h"
#include "lcdcConsole.h"
#include "utl.h"
#include "adc.h"
#include "ctp.h"
#include "intSerialIO.h"
#include "delay.h"

/* not declared in mem.h, since to be used by OS only */
extern void *memAlloc(unsigned size, bool setPID);
extern MemErrno memSet (void *address_p, int val, unsigned nbytes, bool checkPID);

/* function definitions */

/* initialized the file system: inode table, device table
 * must be called by the main process only (and before
 * spawning any child that should have access to file system)
 */
FioErrno fioInit(void) {
	if (fioInitialized){
		return FIO_NO_ERROR;
	}

	/* create inode table */

	inode = (INode *) myMalloc((sizeof(INode)) * FIO_MAX_FILES);
	if (!inode){
		return FIO_INIT_FAILED;
	}

	/* setup device types */

	/* ram files */
	fioDevice[RAM].type = RAM;
	fioDevice[RAM].fopen = ramOpen;
	fioDevice[RAM].fclose = fileClose;;
	fioDevice[RAM].fgetc = ramGetc;
	fioDevice[RAM].fputc = ramPutc;
	fioDevice[RAM].fputs = ramPuts;
	fioDevice[RAM].fgets = ramGets;
	fioDevice[RAM].create = fileCreate;
	fioDevice[RAM].delete = fileDelete;
	fioDevice[RAM].rewind = fileRewind;
	fioDevice[RAM].purge = filePurge;

	/* standard input/ouput device */
	fioDevice[STD].type = STD;
	fioDevice[STD].fopen = stdOpen;
	fioDevice[STD].fclose = stubI;
	fioDevice[STD].fgetc = stdGetc;
	fioDevice[STD].fputc = stdPutc;
	fioDevice[STD].fputs = stubPuts; //talk about
	fioDevice[STD].fgets = stubGets; //talk about
	fioDevice[STD].create = stubCr;
	fioDevice[STD].delete = stubC;
	fioDevice[STD].rewind = stubI;
	fioDevice[STD].purge = stubC;

	/* hardware devices */

	/* LED files */
	fioDevice[LED].type = LED;
	fioDevice[LED].fopen = ledOpen;
	fioDevice[LED].fclose = fileClose;
	fioDevice[LED].fgetc = ledGetc;
	fioDevice[LED].fputc = ledPutc;
	fioDevice[LED].fputs = stubPuts;
	fioDevice[LED].fgets = stubGets;
	fioDevice[LED].create = stubCr;
	fioDevice[LED].delete = stubC;
	fioDevice[LED].rewind = stubI;
	fioDevice[LED].purge = stubC;

	/* PUSH_BUTTON files */
	fioDevice[PUSH_BUTTON].type = PUSH_BUTTON;
	fioDevice[PUSH_BUTTON].fopen = pbOpen;
	fioDevice[PUSH_BUTTON].fclose = fileClose;
	fioDevice[PUSH_BUTTON].fgetc = pbGetc;
	fioDevice[PUSH_BUTTON].fputc = pbPutc;
	fioDevice[PUSH_BUTTON].fputs = stubPuts;
	fioDevice[PUSH_BUTTON].fgets = stubGets;
	fioDevice[PUSH_BUTTON].create = stubCr;
	fioDevice[PUSH_BUTTON].delete = stubC;
	fioDevice[PUSH_BUTTON].rewind = stubI;
	fioDevice[PUSH_BUTTON].purge = stubC;

	/* TWR_LCD file */
	fioDevice[TWR_LCD].type = TWR_LCD;
	fioDevice[TWR_LCD].fopen = lcdOpen;
	fioDevice[TWR_LCD].fclose = fileClose;
	fioDevice[TWR_LCD].fgetc = lcdGetc;
	fioDevice[TWR_LCD].fputc = lcdPutc;
	fioDevice[TWR_LCD].fputs = stubPuts;
	fioDevice[TWR_LCD].fgets = stubGets;
	fioDevice[TWR_LCD].create = stubCr;
	fioDevice[TWR_LCD].delete = stubC;
	fioDevice[TWR_LCD].rewind = stubI;
	fioDevice[TWR_LCD].purge = stubC;

	/* ANALOG file */
	fioDevice[ANALOG].type = ANALOG;
	fioDevice[ANALOG].fopen = anlgOpen;
	fioDevice[ANALOG].fclose = fileClose;
	fioDevice[ANALOG].fgetc = anlgGetc;
	fioDevice[ANALOG].fputc = anlgPutc;
	fioDevice[ANALOG].fputs = stubPuts;
	fioDevice[ANALOG].fgets = stubGets;
	fioDevice[ANALOG].create = stubCr;
	fioDevice[ANALOG].delete = stubC;
	fioDevice[ANALOG].rewind = stubI;
	fioDevice[ANALOG].purge = stubC;

	/* TOUCH_SENSOR file */
	fioDevice[TOUCH_SENSOR].type = TOUCH_SENSOR;
	fioDevice[TOUCH_SENSOR].fopen = tsOpen;
	fioDevice[TOUCH_SENSOR].fclose = fileClose;
	fioDevice[TOUCH_SENSOR].fgetc = tsGetc;
	fioDevice[TOUCH_SENSOR].fputc = tsPutc;
	fioDevice[TOUCH_SENSOR].fputs = stubPuts;
	fioDevice[TOUCH_SENSOR].fgets = stubGets;
	fioDevice[TOUCH_SENSOR].create = stubCr;
	fioDevice[TOUCH_SENSOR].delete = stubC;
	fioDevice[TOUCH_SENSOR].rewind = stubI;
	fioDevice[TOUCH_SENSOR].purge = stubC;

	/* init inode table */

	/* set all inodes starting from the last reserved entry free; */
	/* the last free points to invalid FIO_MAX_FILES index */
	firstFreeInode = LAST_RESERVED_TYPE + 1;
	unsigned short i = firstFreeInode;
	for (; i < FIO_MAX_FILES; i++) {
		inode[i].nextFreeIdx = i + 1;
	}

	/*init hw nodes*/
	for (i = HW_DEV; i <= LAST_RESERVED_TYPE; i++) {
		inode[i].accessCount = USHRT_MAX;
	}

	/* init stream table: */

	fioInitialized = true;
	return (FIO_NO_ERROR);
}

/* device independent functions */

/* opens a connection to existing, or creates and opens a file.
 * takes in: file pathname and access mode;
 * returns: an index into a stream table;
 *
 * myfopen
 *   getHwType
 *   fioDevice[type].fopen()
 */
myFILE myfopen(char *name_p, char *mode_p) {
	if (!fioInitialized){
		/* file system must be initialized first by calling process */
		return -1;
	}

	uint8_t streamIdx;
	int type;

	if ((type = getHwType(name_p)) != -1){
		/* hardware device or STD file */
		streamIdx = (*(fioDevice[type].fopen))(name_p, mode_p);
		if (streamIdx == FIO_MAX_STREAMS){
			return -1;
		}
		return ((myFILE) streamIdx);
	} else {
		/* RAM file */
		streamIdx = (*(fioDevice[RAM].fopen))(name_p, mode_p);
		if (streamIdx == FIO_MAX_STREAMS){
			return -1;
		}
		return ((myFILE) streamIdx);
	}
}

/* closes a connection to a file file;
 * takes in a stream table index
 * returns -1 if not successful
 */
int myfclose(myFILE fi) {
	if ((fi < 0) || (fi > FIO_MAX_STREAMS)){
		return -1;
	}

	if (fi <= HW_DEV) {
		 return fileRewind(fi); /* reserved streams always stay open */
	}

	int sts;
	int type;
	uint8_t streamIdx = fi;
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p){
		return -1;
	}
	if (pcb_p->stream[streamIdx].device_p == NULL){
		/* stream not allocated */
		return -1;
	}
	type = pcb_p->stream[streamIdx].device_p->type;

	sts = (*(fioDevice[type].fclose))(streamIdx);

	return sts;
}

/* writes a int converted to char to a file.
 * takes in int c, and stream table index;
 * returns the int that corresponds to the char written
 */
int myfputc(int c, myFILE fi) {
	if ((fi < 0) || (fi > FIO_MAX_STREAMS)){
		return -1;
	}

	int sts;
	int type;

	uint8_t streamIdx = fi;
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p) {
		return -1;
	}
	if (pcb_p->stream[streamIdx].device_p == NULL){
		/* stream not allocated */
		return -1;
	}
	type = pcb_p->stream[streamIdx].device_p->type;

	sts = (*(fioDevice[type].fputc))(c, streamIdx);

	return sts;
}

/* rewinds the file.
 * takes in the stream table index;
 * returns -1 if fails
 */
int myrewind(myFILE fi) {
	if ((fi < 0) || (fi > FIO_MAX_STREAMS)){
		return -1;
	}

	int sts;
	int type;
	uint8_t streamIdx = fi;
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p) {
		return -1;
	}
	if (pcb_p->stream[streamIdx].device_p == NULL){
		/* stream not allocated */
		return -1;
	}
	type = pcb_p->stream[streamIdx].device_p->type;

	sts = (*(fioDevice[type].rewind))(streamIdx);

	return sts;
}

/* reads a char a file
 * tales in a stream table index;
 * returns a char read converted to an int
 */
int myfgetc(myFILE fi) {
	if ((fi < 0) || (fi > FIO_MAX_STREAMS)){
		return -1;
	}

	int sts;
	int type;
	uint8_t streamIdx = fi;
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p) {
		return -1;
	}
	if (pcb_p->stream[streamIdx].device_p == NULL){
		/* stream not allocated */
		return -1;
	}
	type = pcb_p->stream[streamIdx].device_p->type;

	sts = (*(fioDevice[type].fgetc))(streamIdx);

	return sts;
}

int myfputs(char *s_p, myFILE fi) {
	if ((fi < 0) || (fi > FIO_MAX_STREAMS)) {
		return -1;
	}

	int sts = 0;
	int type;

	uint8_t streamIdx = fi;
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p) {
		return -1;
	}
	if (pcb_p->stream[streamIdx].device_p == NULL) {
		return -1; /* stream not allocated */
	}
	type = pcb_p->stream[streamIdx].device_p->type;

	sts = (*(fioDevice[type].fputs))(s_p, streamIdx);

	return sts;
}

int myfgets(char *s_p, int len, myFILE fi) {
	if ((fi < 0) || (fi > FIO_MAX_STREAMS)) {
		return -1;
	}

	int sts = 0;
	int type;

	uint8_t streamIdx = fi;
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p) {
		return -1;
	}
	if (pcb_p->stream[streamIdx].device_p == NULL) {
		return -1; /* stream not allocated */
	}
	type = pcb_p->stream[streamIdx].device_p->type;

	sts = (*(fioDevice[type].fgets))(s_p, len, streamIdx);

	return sts;
}

/* deletes a file - erases and frees the inode entry
 * takes in the file pathname
 * returns -1 if fails
 */
int mydelete(char *name_p) {
	if (!fioInitialized){
		return 0;
	}

	int sts;
	int type;

	if ((type = getHwType(name_p)) != -1){
		/* hardware device or STD file */
		sts = (*(fioDevice[type].delete))(name_p);
	} else {
		/* RAM file */
		sts = (*(fioDevice[RAM].delete))(name_p);
	}

	return sts;
}

/* creates a file - sets up and inode entry, but not the stream entry.
 * takes in the file path name and the permission specs;
 * returns -1 if fails.
 */
int mycreate(char *name_p, char *permSpec_p) {
	if (!fioInitialized){
		return 0;
	}

	int sts;
	int type;

	if ((type = getHwType(name_p)) != -1){
		/* hardware device or STD file */
		sts = (*(fioDevice[type].create))(name_p, NULL);
	} else {
		/* RAM file */
		sts = (*(fioDevice[RAM].create))(name_p, permSpec_p);
	}

	return sts;
}

int mypurge(char *name_p) {
	if (!fioInitialized) {
		return 0;
	}

	int sts;
	int type;

	if ((type = getHwType(name_p)) != -1) {
		/* hardware device or STD file */
		sts = (*(fioDevice[type].purge))(name_p);
	} else {
		/* RAM file */
		sts = (*(fioDevice[RAM].purge))(name_p);
	}

	return sts;
}
/****************************************************************************/

/* utils  to list dir files */

/* opens dir for record access;
 * intended use - has to be called to enable listing of its sub-directories
 *
 * takes in dir name
 * returns an stream index or -1 if fails
 */
int myopendir(char *id_p) {
	if (!id_p){
		id_p = "/";
	}

	if (!validPathName(id_p)){
		return -1;
	}

	int len = utlStrLen(id_p);
	if (len > FIO_MAX_PATH_NAME){
		return -1;
	}

	int parentInode;
	int inodeIdx;
	char *name_p;

	name_p = id_p;
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p) {
		return -1;
	}

	/* get inode for the parent directory */
	if (id_p[0] == '/') {
		/* path starts with root dir */
		if (pcb_p->currentDirInode == -1) {
			/* root wasnt set yet; is set during pcb initialization */
			return -1;
		} else {
			parentInode = ROOT_DIR;
			name_p++;
			if (*name_p == '\0') {
				return ROOT_DIR;
			}
		}
	} else {
		/* path starts from current dir */
		parentInode = pcb_p->currentDirInode;
	}

	/* see if the requested dir exists */
	FNode fn;
	fn = findFile(name_p, parentInode, false, NULL);

	inodeIdx = fn.self;
	if (inodeIdx == -1){
		/* not found and could not be created */
		return -1;
	}

	if (inode[inodeIdx].type != DIR_FILE){
		return -1;
	}

	/* see if this dir was already opened for ls */
	uint8_t streamIdx;

	bool match;
	for (streamIdx = 0, match = false; (streamIdx < FIO_MAX_STREAMS) && !match;
			streamIdx++) {
		/* stream entries that are not used have device_p = NULL; skip those */
		if ((pcb_p->stream[streamIdx].device_p != NULL)
				&& (pcb_p->stream[streamIdx].inodeIdx == inodeIdx)){
			match = true;
		}
	}

	if (match){
		return streamIdx - 1;
	}

	/* new connection: create new stream  */
	streamIdx = ramStreamSet(id_p, inodeIdx, "r");
	if (streamIdx == FIO_MAX_STREAMS){
		return -1;
	}

	/* don't need to update inode entry */
	inode[inodeIdx].accessCount++;

	return ((int) streamIdx);
}

/* get dir record.
 * takes in dir stream idx;
 * returns the name of the file in the dir record or NULL if none is read
 */
char *getDirRecord(uint8_t streamIdx, char *fileName_p) {
	if (streamIdx == FIO_MAX_STREAMS) {
		return NULL;
	}

	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p) {
		return NULL;
	}

	Block *block_p = pcb_p->stream[streamIdx].position.currBlock_p;
	if (!block_p) {
		return NULL;
	}
	if (!block_p->data_p) {
		return NULL;
	}
	DirRecord * drec_p = (DirRecord *) (block_p->data_p
			+ pcb_p->stream[streamIdx].position.offset);

	if (!fileName_p){
		fileName_p = memAlloc(FIO_MAX_FILE_NAME + 1, 0);
	}
	if (!fileName_p) {
		return NULL;
	}
	memSet(fileName_p, '\0', FIO_MAX_FILE_NAME+1, true);

	bool found = false;
	bool eof = false;

	int inodeIdx = pcb_p->stream[streamIdx].inodeIdx;
	if ((inodeIdx < 0) || (inodeIdx == USHRT_MAX)) {
		return NULL;
	}

	while ((drec_p != NULL) && !found && !eof) {
		if (drec_p->inodeIdx == 0) {
			eof = true; /* erased records will have USHRT_MAX for inode index */
		} else if (drec_p->inodeIdx != USHRT_MAX) {
			/* record not empty */
			utlStrNCpy(fileName_p, drec_p->fileName, FIO_MAX_FILE_NAME);

			found = true;
		} else {
			pcb_p->stream[streamIdx].position.offset += sizeof(DirRecord); /* skip over the empty record */

			/* check for the end of file: last block and offset at the last position */
			if ((block_p->next_p == NULL)
					&& ((inode[inodeIdx].size % FIO_BLOCK_SIZE)
							<= pcb_p->stream[streamIdx].position.offset)){
				eof = true;
			} else if ((FIO_BLOCK_SIZE - pcb_p->stream[streamIdx].position.offset)
					< sizeof(DirRecord)) {
				/* done with this block, go to next */
				block_p = block_p->next_p;
				if (!block_p) {
					eof = true;
				} else {
					pcb_p->stream[streamIdx].position.currBlock_p = block_p;
					pcb_p->stream[streamIdx].position.offset = 0;
					drec_p = (DirRecord *) block_p->data_p;
				}
			} else {
				drec_p++;
			}
		}
	}
	if (!found){
		return NULL;
	}

	pcb_p->stream[streamIdx].position.offset += sizeof(DirRecord); /* skip over the empty record */
	return (fileName_p);
}

/* get stream dedicated to a preset file: STDs, LEDs and Push Buttons */

/* get stream dedicated to an STD file.
 * takes in device id;
 * returns its dedicated stream index, or -1 if fails
 */
int getPresetDev(char *id_p) {
	if (!id_p)
		return -1;

	int streamIdx;

	streamIdx = getStdStream(id_p);

	if (streamIdx == -1) {
		streamIdx = getLedStream(id_p);
	}

	if (streamIdx == -1) {
		streamIdx = getPBStream(id_p);
	}

	if (streamIdx == -1) {
		streamIdx = getLCDStream(id_p);
	}

	if (streamIdx == -1) {
		streamIdx = getAnlgStream(id_p);
	}

	if (streamIdx == -1) {
		streamIdx = getTsStream(id_p);
	}

	return streamIdx;
}

/*the following two functions are for the uart only.
 * They are meant to get the commands and print out the results if possible
 * They come with their own supervisor calls
 */
void printStr(char *str){
	putsIntoBuffer(str);
}
char getChar(bool echo){
	di();
	char c = getcharFromBuffer();
	if (echo) {
		putcharIntoBuffer(c);
	}
	ei();
	return c;
}
