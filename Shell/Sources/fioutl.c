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
#include "tm.h"
#include "rtc.h"

/* external mem functions; */
/* are not declared in mem.h, since to be used by OS only */
extern void *memAlloc(unsigned size, int pId);
extern MemErrno memSet (void *address_p, int val, unsigned nbytes, bool checkPID);

/* local function declarations */

/* stub functions */
int stubI(int num) {
	return 0;
}
int stubC(char* str) {
	return 0;
}
int stubPuts(char *str, myFILE fi) {
	return 0;
}
int stubGets(char *str, int len, myFILE fi) {
	return 0;
}

int stubCr(char* str, char* spec) {
	return 0;
}

/* a helper function to close a file.
 * takes in the stream index;
 * returns -1 if fails.
 */
int fileClose(myFILE fi) {
	if ((fi < 0) || (fi > FIO_MAX_STREAMS)){
		return -1;
	}

	uint8_t streamIdx = (uint8_t) fi;
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p) {
		return -1;
	}

	/* update inode entry */
	inode[pcb_p->stream[streamIdx].inodeIdx].accessCount--;
	pcbReleaseStreamIdx(streamIdx);

	return (0);
}

/* a helper function to delete a file.
 * NOTE: delete does execute if there is open stream/connection for this file
 * takes in the stream index;
 * returns -1 if not successful
 */
int fileDelete(char *id_p) {
	if (!fioInitialized){
		return 0;
	}

	int type;
	if (!id_p){
		return -1;
	}
	if (!validPathName(id_p)){
		return -1;
	}

	/* hardware device or STD file do not get created/deleted/fclosed, only fopened */
	if ((type = getHwType(id_p)) != -1){
		return 0;
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
	if (id_p[0] == '/'){
		/* path starts with root dir */
		parentInode = ROOT_DIR;
		name_p++;
	}else{
		/* path starts from current dir */
		parentInode = pcb_p->currentDirInode;
	}

	/* see if the requested file exists:  look for the file (in inode table), and create if not found */

	FNode fn;
	fn = findFile(name_p, parentInode, false, NULL);
	inodeIdx = fn.self;

	if (inodeIdx == -1){
		/* not found */
		return 0;
	}
	if (inodeIdx == ROOT_DIR) {
		return 0;
	}

	/* check access count */
	if (inode[inodeIdx].accessCount > 0){
		/* open connections exist */
		return -1;
	}

	if ((inode[inodeIdx].type == DIR_FILE) && (inode[inodeIdx].numRec > 0)){
		/* file type is dir, and it is not empty */
		return 0;
	}

	if (!usrAdmin() || !getPermission((int) inodeIdx, "x")) {
		logWrite(FIO_INVREQ_FILE_DELETE, id_p, NULL);
		return -1;
	}

	/* erase from parent dir record */
	eraseDirRec(inodeIdx, fn.parent);

	/* free allocated blocks, and release inode entry */
	releaseInodeIdx(inodeIdx);

	return 0;
}

/* if open connections exist, the file cannot be purged */
int filePurge(char *id_p) {
	if (!fioInitialized) {
		return 0;
	}
	int type;
	if (!id_p || !validPathName(id_p))
		return -1;

	/* hardware device or STD file do not get created/deleted/fclosed, only fopened */
	if ((type = getHwType(id_p)) != -1) {
		return 0;
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
		parentInode = ROOT_DIR;
		name_p++;
	} else {
		/* path starts from current dir */
		parentInode = pcb_p->currentDirInode;
	}

	FNode fn;
	fn = findFile(name_p, parentInode, false, NULL);
	inodeIdx = fn.self;

	if (inodeIdx == -1) {
		/* not found  */
		return 0;
	}

	if (inode[inodeIdx].type == DIR_FILE) {
		/* file type is dir, it doesn't get purged */
		return 0;
	}

	/* check if file can be purged by this user */
	if (!getPermission((int) inodeIdx, "x")) {
		logWrite(FIO_INVREQ_FILE_PURGE, id_p, NULL);
		return -1;
	}

	/* free allocated blocks, and relase inode entry */
	purgeInode(inodeIdx);

	return 0;
}

/* device specific functions */

/* create (inode entry)/ open (stream entry) an STD file
 * takes in the name of the device
 * returns -1 if not successful
 *
 * stdOpen
 *   stdCreate
 * create (inode entry)/ open (stream entry) an STD file
 */

int stdOpen(char *id_p, char *mode_p) {
	if (!id_p)
		return -1;

	unsigned short std;
	if (utlStrCmp("STDIN", id_p)){
		std = STDIN;
	}else if (utlStrCmp("STDOUT", id_p)){
		std = STDOUT;
	}else if (utlStrCmp("STDERR", id_p)){
		std = STDERR;
	}else{
		return -1;
	}

	/* since std entries/channels are reserved, no need to search inode table */
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p) {
		return -1;
	}

	if (pcb_p->stream[std].nextFreeIdx == FIO_MAX_FILES){
		/* stream was already opened */
		return std;
	}

	pcb_p->stream[std].device_p = &fioDevice[STD];

	int inodeIdx = stdCreate(std);

	if (inodeIdx == -1){
		/* could not be opened */
		return (-1);
	}

	utlStrNCpy(pcb_p->stream[std].id, id_p, FIO_MAX_FILE_NAME);
	pcb_p->stream[std].position.currBlock_p = NULL; /* not used by std devices */
	pcb_p->stream[std].position.offset = 0; /* not used by std devices */

	pcb_p->stream[std].inodeIdx = (unsigned short) inodeIdx; /* note: inodeIdx = std */
	pcb_p->stream[std].nextFreeIdx = FIO_MAX_FILES;

	return (inodeIdx);
}

/* stdCreate
 * inserts inode entry = creates a file of std device type
 * takes in the name of the device
 * returns -1 if not successful
 */
int stdCreate(int std) {
	if (std == -1){
		return -1;
	}

	inode[std].type = STD_FILE;
	inode[std].lock = false;
	inode[std].accessCount = 0;
	inode[std].size = 0;

	return (std);
}

/* writes char to a file = c => (stream=>inode=>file of std device type) */
int stdPutc(int c, myFILE fi) {
	if ((fi < 0) || (fi > FIO_MAX_STREAMS)){
		return -1;
	}

	uint8_t streamIdx = fi;
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (pcb_p->stream[streamIdx].device_p->type != STD){
		return -1;
	}

	int inodeIdx = pcb_p->stream[streamIdx].inodeIdx;

	if ((inodeIdx < 0) || (inodeIdx > FIO_MAX_FILES)){
		return -1;
	}
	if (inode[inodeIdx].type != STD_FILE){
		return -1;
	}

	char str[shMAX_BUFFERSIZE + 1];
	if (streamIdx == STDIN) {
		snprintf(str, shMAX_BUFFERSIZE, "You can't write out to stdin.");
	} else if ((streamIdx == STDERR)) {
		if ((c < 32) || (c > 126)){
			snprintf(str, shMAX_BUFFERSIZE, "Error:: %x\r\n", c);
		}else{
			snprintf(str, shMAX_BUFFERSIZE, "Error:: %c\r\n", (char) c);
		}
	} else if (streamIdx == STDOUT) {
		if ((c < 32) || (c > 126)){
			snprintf(str, shMAX_BUFFERSIZE, "%x\r\n", c);
		}else{
			snprintf(str, shMAX_BUFFERSIZE, "%c\r\n", (char) c);
		}
	} else {
		return -1;
	}
	uartPuts(UART2_BASE_PTR, str);
	return 0;
}

/* reads char from a file = c <= (stream=>inode=>file of std device type) */
int stdGetc(myFILE fi) {
	if ((fi < 0) || (fi > FIO_MAX_STREAMS)){
		return -1;
	}

	uint8_t streamIdx = fi;
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (pcb_p->stream[streamIdx].device_p->type != STD){
		return -1;
	}

	int inodeIdx = pcb_p->stream[streamIdx].inodeIdx;

	if ((inodeIdx < 0) || (inodeIdx > FIO_MAX_FILES)){
		return -1;
	}
	if (inode[inodeIdx].type != STD_FILE){
		return -1;
	}

	unsigned char c;

	if (streamIdx == STDIN) {
		uartPuts(UART2_BASE_PTR, "Please enter a char: ");
		c = uartGetchar(UART2_BASE_PTR);
		uartPuts(UART2_BASE_PTR, "\r\n");
	} else if ((streamIdx == STDERR)) {
		uartPuts(UART2_BASE_PTR, "You can't get input from stderr");
		c = '0';
	} else if (streamIdx == STDOUT) {
		uartPuts(UART2_BASE_PTR, "You can't get input from stdout");
		c = '0';
	} else {
		return -1;
	}

	return (int) c;
}

/* create/open functions for RAM files
 * creates inode/stream entries:
 * calls ramCreate (inode) if file does not exit, and then opens an IO channel (stream)
 * to a file of ram device type: reg files and directories
 *
 * ramOpen
 *    dirCreate  (if root dir)
 *    findFile (process node in pathname)
 *      getNodeName
 *      searchDir (if final node)
 *      ramCreate (if final node)
 *         dirCreate  (if node is directory)
 *            ramInodeSet  (init inode entry)
 *         regCreate  (if node is reg file)
 *            ramInodeSet  (init inode entry)
 *         writeDirRec  (update parent dir record)
 *      findFile (if intermediate node)
 *    ramStreamSet (init stream entry)
 *
 * takes in file path name;
 * returns stream index or -1 if unsuccessful.
 */

int ramOpen(char * id_p, char *mode_p) {
	if (!id_p){
		return -1;
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
	if (id_p[0] == '/'){
		/* path starts with root dir */
		if (pcb_p->currentDirInode == -1){
			/* root wasn't set yet; is set during pcb initialization */
			Position pos;
			inodeIdx = ramCreate(id_p, len, -1, pos, DIR_FILE, NULL); /* set root */

			if (inodeIdx == -1){
				return -1;
			}
			ramStreamSet(id_p, ROOT_DIR, mode_p);
			return ROOT_DIR; /* dedicated entry in the stream table */
		}else{
			parentInode = ROOT_DIR;
			name_p++;
		}
	}else{
		parentInode = pcb_p->currentDirInode; /* path starts from current dir */
	}

	/* see if the requested file exists:  look for the file (in inode table), and create if not found */
	FNode fn;
	fn = findFile(name_p, parentInode, true, NULL);

	inodeIdx = fn.self;

	/*inodeIdx = findFile(name_p, &parentInode, true); */

	if (inodeIdx == -1){
		return -1; /* not found and could not be created */
	}

	if (inode[inodeIdx].type == DIR_FILE){
		return ROOT_DIR; /* stream entry dedicated to all dir nodes */
	}
	/* symbolic, since directories don't need streams associate with them */
	/* so all dirs will point to this one stream */

	/* set stream for this file                                                                  */
	/* for reg files:                                                                            */
	/* see if the connection was already established: stream entry already exists for this file; */
	/* check by matching inodeIdx; linear search should be fast;								 */
	/* dont need to do it for dir files, since they dont have stream entries					 */

	uint8_t streamIdx;

	bool match;
	for (streamIdx = 0, match = false; (streamIdx < FIO_MAX_STREAMS) && !match;
			streamIdx++) {
		/* stream entries that are not used have device_p = NULL; skip those */
		if ((pcb_p->stream[streamIdx].device_p != NULL)
				&& (pcb_p->stream[streamIdx].inodeIdx == inodeIdx)
				&& utlStrNCmp(mode_p, pcb_p->stream[streamIdx].mode,
						FIO_MAX_MODE)) {
			match = true;
		}
	}

	if (match){
		return streamIdx - 1;
	}

	/* new connection: create new stream  */
	streamIdx = ramStreamSet(id_p, inodeIdx, mode_p);
	if (streamIdx == FIO_MAX_STREAMS){
		return -1;
	}

	/* update inode entry */
	inode[inodeIdx].accessCount++;

	return ((int) streamIdx);

}

/* recursive function: check for the existence of each dir node in the path name in its parent
 * directory; if any intermediate node does not exist the function fails, else finds or creates
 * the final node (reg file or dir) in its parent dir
 *
 * findFile
 *    getNodeName
 *    searchDir (if final node)
 *    ramCreate (if final node)
 *       dirCreate  (if directory)
 *          ramInodeSet
 *       regCreate  (if reg file)
 *          ramInodeSet
 *       writeDirRec  (update parent dir record)
 *    findFile (if intermediate node)
 *
 * takes in file pathname, the inode index of the parent (directory in which the
 * file resides) and a flag indicating if the file should be created if not found.
 * return the inode index or -1 if not successful
 */
FNode findFile(char *name_p, int parentInode, bool create, char *permSpec_p){
	FNode fn = { -1, -1 };

	int inodeIdx = -1;
	int len = utlStrLen(name_p);

	int nodeNameLen;

	nodeNameLen = getNodeName(name_p);
	if (nodeNameLen == -1) {
		return fn;
	}

	Position position;

	if (nodeNameLen == len){
		/* end node: file to be opened */
		/* if exists return its inode index, else create first */

		inodeIdx = searchDir(name_p, nodeNameLen, parentInode, &position);

		if ((inodeIdx == -1) && create) {
			inodeIdx = ramCreate(name_p, nodeNameLen, parentInode, position,
					REG_FILE, permSpec_p);
		}

		fn.self = inodeIdx;
		fn.parent = parentInode;
		return fn;
	} else if (nodeNameLen == (len - 1)){
		/* end node: dir to be opened - the last char is '/' */
		/* if exists return its inode index, else create first */
		inodeIdx = searchDir(name_p, nodeNameLen, parentInode, &position);
		if ((inodeIdx == -1) && create){
			inodeIdx = ramCreate(name_p, nodeNameLen, parentInode, position,
					DIR_FILE, permSpec_p);
		}
		fn.self = inodeIdx;
		fn.parent = parentInode;
		return fn;
	} else {
		/* intermediate node: directory to be found */
		/* search current directory for the node; it it exists set parentInode to this node and repeat */
		/* else return fail status */

		inodeIdx = searchDir(name_p, nodeNameLen, parentInode, &position);
		if (inodeIdx == -1) {
			fn.self = -1;
			return fn;
		} /* directory in the path name does not exist */

		if (inode[inodeIdx].type == REG_FILE) {
			fn.self = -1;
			return fn;
		} /* directory in the path name does not exist */
		/* this intermediate node is a file name */

		parentInode = inodeIdx;
		name_p = name_p + nodeNameLen + 1; /* skip over '/' */
		return (findFile(name_p, parentInode, create, permSpec_p));

	}

}

/* creates a file of ram device type:  inserts inode entry
 * position indicates where in parent directory data block the new file info is to be recorded
 *
 *    ramCreate
 *     dirCreate  (if directory)
 *       ramInodeSet
 *     regCreate  (if reg file)
 *       ramInodeSet
 *     writeDirRec  (update parent dir record)
 *
 * takes in file name (node name - not the full path name), length of the file name
 * the inode index of the dir file in which it is to be recorded, the position
 * of where in the parent dir data block it is to be recorded, and the type of the file:
 * reg or dir.
 */
int ramCreate(char *name_p, unsigned short len, int parentInode,
		Position position, FileType type, char *permSpec_p) {
	int inodeIdx;

	if (type == DIR_FILE) {

		inodeIdx = getPresetIdx(name_p, len);
		if (inodeIdx != -1){
			dirDevCreate(parentInode, inodeIdx);
		}else{
			inodeIdx = dirCreate(parentInode);
		}
	} else {
		inodeIdx = regCreate();
	}

	int sts;
	sts = writeDirRec(name_p, len, inodeIdx, parentInode, position);

	if (sts == -1){
		return -1;
	}

	setUser(inodeIdx);
	setPermissions(inodeIdx, permSpec_p);

	return inodeIdx;
}

/* records the created file and its inode in the parent directory
 * position at which data to be recorded was returned by searchDir()
 * nodes are not recorded in alphabetical order. Takes in the file
 * node name (not path name), name length, its inode index, the inode
 * index of the parent, and the position at which to write the record;
 * returns -1 if fails.
 */
int writeDirRec(char *name_p, unsigned short len, int inodeIdx, int parentInode,
		Position position) {
	if (parentInode == -1) {
		/* root dir has no parent record */
		return 0;
	}
	/* record */
	DirRecord *drec_p;

	/* check if there are freed record slots from previous files being deleted */
	/* if so, find the first free record slot and updated with current node info */
	bool updated = false;
	if (inode[parentInode].numFreeRec > 0) {
		unsigned short size = 0;
		Block *block_p = inode[parentInode].firstBlock_p;
		drec_p = (DirRecord *) block_p->data_p;
		while (!updated && drec_p != NULL) {
			if (drec_p->inodeIdx == USHRT_MAX) {
				utlStrMCpy(drec_p->fileName, name_p, len, FIO_MAX_FILE_NAME);
				drec_p->inodeIdx = inodeIdx;
				inode[parentInode].numFreeRec--;
				updated = true;
			} else {
				size += sizeof(DirRecord);
				if ((FIO_BLOCK_SIZE - size) < sizeof(DirRecord)) {
					block_p = block_p->next_p;
					if (!block_p) {
						drec_p = NULL;
					} else {
						drec_p = (DirRecord *) block_p->data_p;
					}
				} else {
					drec_p++;
				}
			}
		}
		/* if not updated (couldn't find free slot - bug), continue as if nothing happened :) */
	}

	if (!updated) {
		/* check if there is room in the current block, which was found during the search of the parent dir */
		/* if not, create and link a new block */
		if ((FIO_BLOCK_SIZE - position.offset) < sizeof(DirRecord)){
			/* get a new block */
			Block *block_p = memAlloc(sizeof(Block), -1);
			if (!block_p){
				return -1;
			}
			position.currBlock_p->next_p = block_p;
			block_p->next_p = NULL;
			block_p->prev_p = position.currBlock_p;
			block_p->data_p = memAlloc(FIO_BLOCK_SIZE, -1);
			if (!block_p->data_p){
				return -1;
			}
			if (memSet(block_p->data_p, 0, FIO_BLOCK_SIZE, false) != MEM_NO_ERROR){
				return -1;
			}
			position.currBlock_p = block_p;
			position.offset = 0;
		}

		drec_p = (DirRecord *) (position.currBlock_p->data_p + position.offset);
		utlStrMCpy(drec_p->fileName, name_p, len, FIO_MAX_FILE_NAME);
		drec_p->inodeIdx = inodeIdx;

		inode[parentInode].size = inode[parentInode].size + sizeof(DirRecord);
	}
	inode[parentInode].numRec++;
	return 0;

}

/* removes the record from dir data
 * takes in the inode index of the file to be erased and the inode index
 * of the dir from which the file is to be erased;
 * returns -1 if fails
 */
int eraseDirRec(int inodeIdx, int parentInode) {
	/* record */
	DirRecord *drec_p;
	bool updated = false;

	if (inode[parentInode].numRec <= 0){
		return -1;
	}

	unsigned short size = 0;

	Block *block_p = inode[parentInode].firstBlock_p;
	if (!block_p){
		return -1;
	}
	drec_p = (DirRecord *) block_p->data_p;
	if (!drec_p){
		return -1;
	}

	while (!updated && (drec_p != NULL)) {
		if (drec_p->inodeIdx == inodeIdx) {

			drec_p->inodeIdx = USHRT_MAX;

			memSet(drec_p->fileName, '\0', FIO_MAX_FILE_NAME, false);

			inode[parentInode].numFreeRec++;
			inode[parentInode].numRec--;
			updated = true;
		} else {
			size += sizeof(DirRecord);
			if ((FIO_BLOCK_SIZE - size) < sizeof(DirRecord)) {
				block_p = block_p->next_p;
				if (!block_p) {
					drec_p = NULL;
				} else {
					drec_p = (DirRecord *) block_p->data_p;
				}

			} else {
				drec_p++;
			}
		}
	}
	return 0;
}

/* creates file of type dir.
 * takes in inode index of parent dir
 * returns inode index of created dir or -1 if fails
 */
int dirCreate(int parentIdx) {
	int inodeIdx;
	if (parentIdx == -1) {
		/* create root dir */
		inodeIdx = ROOT_DIR;
		parentIdx = ROOT_DIR;
	} else {
		inodeIdx = getFreeInodeIdx();

		if (inodeIdx == FIO_MAX_FILES) {
			/* maxed out; cannot create anymore files */
			return -1;
		}
	}

	if (ramInodeSet(inodeIdx, DIR_FILE) == -1){
		return -1;
	}

	/* init directory: write self ('.') and parent('..') entries */
	DirRecord *drec_p;

	/* self */
	drec_p = (DirRecord *) inode[inodeIdx].firstBlock_p->data_p;
	utlStrMCpy(drec_p->fileName, ".", 1, FIO_MAX_FILE_NAME);
	drec_p->inodeIdx = inodeIdx;
	inode[inodeIdx].size += sizeof(DirRecord);

	/* parent */
	drec_p++;
	utlStrMCpy(drec_p->fileName, "..", 2, FIO_MAX_FILE_NAME);
	drec_p->inodeIdx = parentIdx;
	inode[inodeIdx].size += sizeof(DirRecord);

	return ((int) inodeIdx);
}

int dirDevCreate(int parentIdx, int inodeIdx) {

	if (ramInodeSet(inodeIdx, DIR_FILE) == -1) {
		return -1;
	}

	/* init directory: write self ('.') and parent('..') entries */
	DirRecord *drec_p;

	/* self */
	drec_p = (DirRecord *) inode[inodeIdx].firstBlock_p->data_p;
	utlStrMCpy(drec_p->fileName, ".", 1, FIO_MAX_FILE_NAME);
	drec_p->inodeIdx = inodeIdx;
	inode[inodeIdx].size += sizeof(DirRecord);

	/* parent */
	drec_p++;
	utlStrMCpy(drec_p->fileName, "..", 2, FIO_MAX_FILE_NAME);
	drec_p->inodeIdx = parentIdx;
	inode[inodeIdx].size += sizeof(DirRecord);

	return inodeIdx;
}

/* regCreate
 *   ramInodeSet
 */
int regCreate(void) {
	int inodeIdx = getFreeInodeIdx();
	if (inodeIdx == FIO_MAX_FILES){
		/* maxed out; cannot create anymore files */
		return -1;
	}

	if (ramInodeSet(inodeIdx, REG_FILE) == -1){
		releaseInodeIdx(inodeIdx);
		return -1;
	}

	return inodeIdx;
}

/* sets up - initialized inode entry
 * takes in inode index, and file type
 * returns -1 if fails
 */
int ramInodeSet(unsigned short inodeIdx, FileType type) {
	inode[inodeIdx].type = type;
	inode[inodeIdx].lock = false;
	inode[inodeIdx].accessCount = 0;
	inode[inodeIdx].size = 0;

	/* init start block */
	inode[inodeIdx].firstBlock_p = memAlloc(sizeof(Block), -1);
	if (inode[inodeIdx].firstBlock_p == NULL){
		return (-1);
	}

	inode[inodeIdx].firstBlock_p->next_p = NULL;
	inode[inodeIdx].firstBlock_p->prev_p = NULL;

	inode[inodeIdx].firstBlock_p->data_p = memAlloc(FIO_BLOCK_SIZE, -1);
	if (inode[inodeIdx].firstBlock_p->data_p == NULL){
		return (FIO_INIT_FAILED);
	}
	if (memSet(inode[inodeIdx].firstBlock_p->data_p, 0, FIO_BLOCK_SIZE, false)
			!= MEM_NO_ERROR){
		return -1;
	}

	return inodeIdx;
}

/* requests and sets up a stream table entry
 * takes in file path name, and inode index
 * returns FIO_MAX_STREAMS if fails
 */
uint8_t ramStreamSet(char *id_p, unsigned short inodeIdx, char *mode_p) {
	if (!id_p) {
		return -1;
	}

	unsigned len = utlStrLen(id_p);
	if (len > FIO_MAX_PATH_NAME) {
		return -1;
	}

	/* check permissions */
	if (!mode_p || (utlStrLen(mode_p) > FIO_MAX_MODE)) {
		/* default mode */
		mode_p = "r";
	}

	if (!getPermission((int) inodeIdx, mode_p)) {
		/* log */
		logWrite(FIO_INVREQ_FILE_OPEN, id_p, mode_p);
		return -1;
	}

	/* new connection: create new stream  */
	uint8_t streamIdx;
	if (inodeIdx == ROOT_DIR) {
		streamIdx = ROOT_DIR;
	} else {
		streamIdx = pcbGetFreeStreamIdx();
	}

	if (streamIdx == FIO_MAX_STREAMS) {
		return FIO_MAX_STREAMS;
	}

	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p) {
		return -1;
	}
	pcb_p->stream[streamIdx].device_p = &fioDevice[RAM];

	utlStrCpy(pcb_p->stream[streamIdx].id, id_p);

	utlStrCpy(pcb_p->stream[streamIdx].mode, mode_p);

	if (*mode_p == 'a') {
		/* append; start access at the end of the file */
		Block *block_p = inode[inodeIdx].firstBlock_p;
		while (block_p->next_p != NULL)
			block_p = block_p->next_p;
		pcb_p->stream[streamIdx].position.currBlock_p = block_p;

		pcb_p->stream[streamIdx].position.offset = (inode[inodeIdx].size
				% FIO_BLOCK_SIZE);
	} else {
		/* start access from the beginning */
		pcb_p->stream[streamIdx].position.currBlock_p =
				inode[inodeIdx].firstBlock_p;
		pcb_p->stream[streamIdx].position.offset = 0;
	}

	pcb_p->stream[streamIdx].inodeIdx = (unsigned short) inodeIdx;

	return streamIdx;
}

/* looks for a file (reg or dir) in the 'parent' directory;
 * takes in a file node name, name length, parend dir inode
 * index and a position struct address. If found, returns the
 * inode index of the file;
 * it also returns where in the block data record of the parent
 * the search ended via pisition_p argument if file is not found,
 * position_p will be used to record to be created file info;
 * dir entries are not recorded in alphabetical order
 */
int searchDir(char *nodeName_p, unsigned short len, int parentInode,
		Position *position_p) {
	if (!nodeName_p || (len <= 0) || (parentInode == -1)){
		return -1;
	}

	if (inode[parentInode].size == 0){
		return -1;
	}

	int inodeIdx = -1;
	int numRecords = inode[parentInode].size / sizeof(DirRecord);
	int numRecordsPerBlock = FIO_BLOCK_SIZE / sizeof(DirRecord);

	int i, j;
	Block *block_p = inode[parentInode].firstBlock_p;
	bool found = false;

	j = 0;
	while ((block_p != NULL) && !found && (j < numRecords)) {
		DirRecord *drec_p;
		drec_p = (DirRecord *) block_p->data_p;

		for (i = 0; (i < numRecordsPerBlock) && (j < numRecords) && !found;
				i++, j++, drec_p++) {
			/* comparing a len size section of nodeName_p */
			/*  FileName is either terminated by '\0' or is FIO_MAX_FILE_NAME long */

			if (utlStrMCmp(drec_p->fileName, nodeName_p, len,
					FIO_MAX_FILE_NAME)) {
				found = true;

				inodeIdx = drec_p->inodeIdx;
			}
		}
		position_p->currBlock_p = block_p;
		position_p->offset = i * sizeof(DirRecord);
		block_p = block_p->next_p;
	}

	return inodeIdx;
}

/* writes char to a RAM file
 * takes in int for a character to be written, and file stream index;
 * returns an int = written character or -1 if fails
 */
int ramPutc(int c, myFILE fi){
	if ((fi < 0) || (fi > FIO_MAX_STREAMS)){
		return -1;
	}

	uint8_t streamIdx = fi;
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p) {
		return -1;
	}
	if (pcb_p->stream[streamIdx].device_p == NULL){
		/* stream not allocated */
		return -1;
	}
	if (pcb_p->stream[streamIdx].device_p->type != RAM){
		return -1;
	}

	if ((pcb_p->stream[streamIdx].mode[0] != 'w')
			&& (pcb_p->stream[streamIdx].mode[1] != '+')
			&& (pcb_p->stream[streamIdx].mode[0] != 'a')) {
		logWrite(FIO_INVREQ_WRITE, pcb_p->stream[streamIdx].id, NULL);
		return -1;
	}

	int inodeIdx = pcb_p->stream[streamIdx].inodeIdx;

	if ((inodeIdx < 0) || (inodeIdx > FIO_MAX_FILES)){
		return -1;
	}
	if (inode[inodeIdx].type != REG_FILE){
		return -1;
	}

	if (pcb_p->stream[streamIdx].position.offset == FIO_BLOCK_SIZE) {
		Block *block_p = pcb_p->stream[streamIdx].position.currBlock_p;
		if (block_p->next_p == NULL){
			/* allocate new block */
			block_p->next_p = memAlloc(sizeof(Block), -1);
			if (!block_p->next_p){
				return -1;
			}

			pcb_p->stream[streamIdx].position.currBlock_p = block_p->next_p;
			pcb_p->stream[streamIdx].position.currBlock_p->prev_p = block_p;
			pcb_p->stream[streamIdx].position.currBlock_p->next_p = NULL;

			pcb_p->stream[streamIdx].position.currBlock_p->data_p = memAlloc(
					FIO_BLOCK_SIZE, -1);
			if (pcb_p->stream[streamIdx].position.currBlock_p->data_p == NULL){
				return -1;
			}
			if (memSet(pcb_p->stream[streamIdx].position.currBlock_p->data_p, 0,
					FIO_BLOCK_SIZE, false) != MEM_NO_ERROR) {
				return -1;
			}
			pcb_p->stream[streamIdx].position.offset = 0;

		} else {
			pcb_p->stream[streamIdx].position.currBlock_p =
					pcb_p->stream[streamIdx].position.currBlock_p->next_p;
			pcb_p->stream[streamIdx].position.offset = 0;
		}

	}

	char *data_p = pcb_p->stream[streamIdx].position.currBlock_p->data_p
			+ pcb_p->stream[streamIdx].position.offset;

	*data_p = (unsigned char) c;
	pcb_p->stream[streamIdx].position.offset++;

	inode[inodeIdx].size++;

	return c;
}

/* rewinds a RAM file
 * takes in a file stream index;
 * returns -1 if fails
 */
int fileRewind(myFILE fi){
	if ((fi < 0) || (fi > FIO_MAX_STREAMS)){
		return -1;
	}
	uint8_t streamIdx = fi;
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p) {
		return -1;
	}
	if (!pcb_p->stream[streamIdx].device_p){
		/* stream not allocated */
		return -1;
	}
	if (pcb_p->stream[streamIdx].device_p->type != RAM){
		return -1;
	}

	int inodeIdx = pcb_p->stream[streamIdx].inodeIdx;
	if ((inodeIdx < 0) || (inodeIdx > FIO_MAX_FILES)){
		return -1;
	}
	if (pcb_p->stream[streamIdx].mode[0] == 'a') {
		logWrite(FIO_INVREQ_FILE_REWIND, pcb_p->stream[streamIdx].id, NULL);
		return -1;
	}

	pcb_p->stream[streamIdx].position.currBlock_p = inode[inodeIdx].firstBlock_p;
	pcb_p->stream[streamIdx].position.offset = 0;

	return 0;
}

/* reads char from a RAM file
 * file stream index;
 * returns an int = read character or -1 if fails
 */
int ramGetc(myFILE fi) {
	if ((fi < 0) || (fi > FIO_MAX_STREAMS)){
		return -1;
	}
	uint8_t streamIdx = fi;
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p) {
		return -1;
	}
	if (!pcb_p->stream[streamIdx].device_p) {
		/* stream not allocated */
		return -1;
	}
	if (pcb_p->stream[fi].device_p->type != RAM){
		return -1;
	}

	if ((pcb_p->stream[streamIdx].mode[0] != 'r')
			&& (pcb_p->stream[streamIdx].mode[1] != '+')) {
		logWrite(FIO_INVREQ_READ, pcb_p->stream[streamIdx].id, NULL);
		return -1;
	}

	int inodeIdx = pcb_p->stream[streamIdx].inodeIdx;
	if ((inodeIdx < 0) || (inodeIdx > FIO_MAX_FILES)){
		return -1;
	}
	if (inode[inodeIdx].type != REG_FILE){
		return -1;
	}

	Block *block_p;

	if (pcb_p->stream[streamIdx].position.offset == FIO_BLOCK_SIZE) {
		block_p = pcb_p->stream[streamIdx].position.currBlock_p->next_p;
		pcb_p->stream[streamIdx].position.offset = 0;
		pcb_p->stream[streamIdx].position.currBlock_p = block_p;
	} else {
		block_p = pcb_p->stream[streamIdx].position.currBlock_p;
	}

	if (block_p == NULL) {
		return -1; /*?? EOF late */
	}

	/* check for the end of file: last block and offset at the last position */
	if ((block_p->next_p == NULL)
			&& ((inode[inodeIdx].size % FIO_BLOCK_SIZE)
					<= pcb_p->stream[fi].position.offset)){
		return -1;
	}

	char *data_p = block_p->data_p + pcb_p->stream[streamIdx].position.offset;

	if (data_p == NULL){
		return -1;
	}

	unsigned char c = *data_p;
	pcb_p->stream[streamIdx].position.offset++;

	return (int) c;
}

int ramPuts(char *str_p, myFILE fi) {
	if ((fi < 0) || (fi > FIO_MAX_STREAMS)) {
		return -1;
	}
	if (!str_p) {
		return -1;
	}

	uint8_t streamIdx = fi;
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p)
		return -1;
	if (pcb_p->stream[streamIdx].device_p == NULL) {
		/* stream not allocated */
		return -1;
	}
	if (pcb_p->stream[streamIdx].device_p->type != RAM) {
		return -1;
	}

	if ((pcb_p->stream[streamIdx].mode[0] != 'w')
			&& (pcb_p->stream[streamIdx].mode[1] != '+')
			&& (pcb_p->stream[streamIdx].mode[0] != 'a')) {
		if (fi != pcb_p->fiLog) {
			logWrite(FIO_INVREQ_WRITE, pcb_p->stream[streamIdx].id, NULL);
		}
		return -1;
	}

	int c = 0;
	while ((*str_p != '\0') && (c != -1)) {
		c = ramPutc(*str_p, fi);
		str_p++;
	}
	return (0);
}

/* adapted with modifications from The C Programming Language by B.Kernighan, D.Ritchie
 * str_p must be len+1 long, since it will be terminated by '\0';
 * New line ('\n'), end of file, or len argument (whichever reached first),
 * determine the lenth of the output string pointed by str_p
 */
int ramGets(char *str_p, int len, myFILE fi) {
	if ((fi < 0) || (fi > FIO_MAX_STREAMS)) {
		return -1;
	}
	if (!str_p) {
		return -1;
	}

	uint8_t streamIdx = fi;
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p)
		return -1;
	if (!pcb_p->stream[streamIdx].device_p) {
		/* stream not allocated */
		return -1;
	}
	if (pcb_p->stream[fi].device_p->type != RAM) {
		return -1;
	}

	if ((pcb_p->stream[streamIdx].mode[0] != 'r')
			&& (pcb_p->stream[streamIdx].mode[1] != '+')) {
		logWrite(FIO_INVREQ_READ, pcb_p->stream[streamIdx].id, NULL);
		return -1;
	}

	int c;
	int n = len;

	*str_p = '\0';

	while ((--n > 0) && ((c = ramGetc(fi)) != -1) && (c != -1)) {
		*str_p = (char) c;
		str_p++;
		if (c == '\n') {
			break;
		}
	}
	*str_p = '\0';

	return (0);
}

/* open led device file.
 * takes in name/id of the device
 * returns stream index (= inode index) for this device, or -1 if fails
 */
int ledOpen(char * id_p, char *mode_p){
	if (!id_p){
		return -1;
	}

	int led;
	led = getLedStream(id_p);
	if (led == -1){
		return -1;
	}

	/* since led entries/channels are reserved, no need to search inode table */
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p) {
		return -1;
	}

	if (pcb_p->stream[led].nextFreeIdx == FIO_MAX_STREAMS){
		/* stream was already opened */
		return led;
	}

	int inodeIdx = ledCreate(led, id_p);

	if (inodeIdx == -1){
		/* could not be opened */
		return (-1);
	}

	if (inode[inodeIdx].accessCount >= 1){
		/* led accessed by another process*/
		return (-1);
	}else{
		/*reserve led for this process process*/
		inode[inodeIdx].accessCount++;
	}

	pcb_p->stream[led].device_p = &fioDevice[LED];

	utlStrNCpy(pcb_p->stream[led].id, id_p, FIO_MAX_FILE_NAME);
	pcb_p->stream[led].position.currBlock_p = NULL; /* not used by led devices */
	pcb_p->stream[led].position.offset = 0; /* not used by led devices */

	pcb_p->stream[led].inodeIdx = (unsigned short) inodeIdx; /* note: inodeIdx = led */
	pcb_p->stream[led].nextFreeIdx = FIO_MAX_STREAMS;

	return (inodeIdx);
}

/* inserts inode entry = creates a file of led device type at a
 * reserved spot. Takes in the dedicated inode index for this
 * the device;
 * returns inode index for this device, or -1 if fails
 */
int ledCreate(int led, char *id_p) {
	if (led == -1){
		return -1;
	}

	if (inode[led].accessCount != USHRT_MAX) {
		return led;
	}

	inode[led].type = LED_DEV;
	inode[led].lock = false;
	inode[led].accessCount = 0;
	inode[led].size = 0;

	int sts;
	sts = writeDirDevRec(id_p, led, LED_DIR);

	if (sts == -1) {
		return -1;
	}

	return (led);
}

/* write to LED to turn one on or off. Takes in an int storing
 * the ASCII of the original char (witch is either a 1 for on
 * or 0 for off) and the LED stream. If the char passed in isn't
 * a 1 or a 0, then nothing happens.
 */
int ledPutc(int c, myFILE fi) {
	if ((fi < 0) || (fi > FIO_MAX_STREAMS)){
		return -1;
	}

	uint8_t streamIdx = fi;
	ProcessControlBlock *pcb_p = getCurrentPCB();

	if (pcb_p->stream[streamIdx].device_p == NULL) {
		/* stream not allocated */
		return -1;
	}

	if (pcb_p->stream[streamIdx].device_p->type != LED){
		return -1;
	}

	int inodeIdx = pcb_p->stream[streamIdx].inodeIdx;

	if ((inodeIdx < 0) || (inodeIdx > FIO_MAX_FILES)){
		return -1;
	}
	if (inode[inodeIdx].type != LED_DEV){
		return -1;
	}

	char on = '1';
	char off = '0';
	if(c == (int) off){
		/*turn off led*/
		if (streamIdx == ORANGE){
			ledOrangeOff();
		}else if (streamIdx == YELLOW){
			ledYellowOff();
		}else if (streamIdx == GREEN){
			ledGreenOff();
		}else if (streamIdx == BLUE){
			ledBlueOff();
		} else {
			return -1;
		}
	}else if(c == (int) on){
		/*turn on led*/
		if (streamIdx == ORANGE){
			ledOrangeOn();
		}else if (streamIdx == YELLOW){
			ledYellowOn();
		}else if (streamIdx == GREEN){
			ledGreenOn();
		}else if (streamIdx == BLUE){
			ledBlueOn();
		} else {
			return -1;
		}
	}

	return 0;
}
int ledGetc(myFILE fi) {
	if ((fi < 0) || (fi > FIO_MAX_STREAMS)){
		return -1;
	}

	uint8_t streamIdx = fi;
	ProcessControlBlock *pcb_p = getCurrentPCB();

	if (pcb_p->stream[streamIdx].device_p == NULL) {
		/* stream not allocated */
		return -1;
	}

	if (pcb_p->stream[streamIdx].device_p->type != LED){
		return -1;
	}

	int inodeIdx = pcb_p->stream[streamIdx].inodeIdx;

	if ((inodeIdx < 0) || (inodeIdx > FIO_MAX_FILES)){
		return -1;
	}
	if (inode[inodeIdx].type != LED_DEV){
		return -1;
	}

	bool status;
	unsigned char on = '1';
	unsigned char off = '0';
	if (streamIdx == ORANGE) {
		status = ledOrangeStat();
	} else if (streamIdx == YELLOW) {
		status = ledYellowStat();
	} else if (streamIdx == GREEN) {
		status = ledGreenStat();
	} else if (streamIdx == BLUE) {
		status = ledBlueStat();
	} else {
		return -1;
	}

	if(status) {
		return (int) on;
	} else {
		return (int) off;
	}
}

/* pbOpen
 *   pbCreate
 * create (inode entry)/ open (stream entry) an STD file
 */
int pbOpen(char * id_p, char *mode_p){
	if (!id_p){
		return -1;
	}

	int pb;
	pb = getPBStream(id_p);
	if (pb == -1){
		return -1;
	}

	/* since pb entries/channels are reserved, no need to search inode table */
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p) {
		return -1;
	}

	if (pcb_p->stream[pb].nextFreeIdx == FIO_MAX_STREAMS){
		/* stream was already opened */
		return pb;
	}

	int inodeIdx = pbCreate(pb, id_p);

	if (inodeIdx == -1){
		/* could not be opened */
		return (-1);
	}

	if (inode[inodeIdx].accessCount >= 1){
		/* led accessed by another process*/
		return (-1);
	}else{
		/*reserve led for this process process*/
		inode[inodeIdx].accessCount++;
	}

	pcb_p->stream[pb].device_p = &fioDevice[PUSH_BUTTON];

	utlStrNCpy(pcb_p->stream[pb].id, id_p, FIO_MAX_FILE_NAME);
	pcb_p->stream[pb].position.currBlock_p = NULL; /* not used by pb devices */
	pcb_p->stream[pb].position.offset = 0; /* not used by pb devices */

	pcb_p->stream[pb].inodeIdx = (unsigned short) inodeIdx; /* note: inodeIdx = pb */
	pcb_p->stream[pb].nextFreeIdx = FIO_MAX_STREAMS;

	return (inodeIdx);
}

/* inserts inode entry = creates a file of push button device type
 * at a reserved spot. Takes in the dedicated inode index for this
 * the device;
 * returns inode index) for this device, or -1 if fails
 */
int pbCreate(int pb, char *id_p) {
	if (pb == -1)
		return -1;

	if (inode[pb].accessCount != USHRT_MAX) {
		return pb;
	}

	inode[pb].type = PUSH_BUTTON_DEV;
	inode[pb].lock = false;
	inode[pb].accessCount = 0;
	inode[pb].size = 0;

    int sts;
	sts = writeDirDevRec(id_p, pb, PB_DIR);

	if (sts == -1) {
		return -1;
	}

	return (pb);
}

int pbPutc(int c, myFILE fi) {
	uartPuts(UART2_BASE_PTR,
			"Why are you writing to a button? You do realize that isn't possible?\r\n");
	return 0;
}
int pbGetc(myFILE fi) {
	if ((fi < 0) || (fi > FIO_MAX_STREAMS)){
		return -1;
	}

	uint8_t streamIdx = fi;
	ProcessControlBlock *pcb_p = getCurrentPCB();

	if (pcb_p->stream[streamIdx].device_p == NULL) {
		/* stream not allocated */
		return -1;
	}

	if (pcb_p->stream[streamIdx].device_p->type != PUSH_BUTTON){
		return -1;
	}

	int inodeIdx = pcb_p->stream[streamIdx].inodeIdx;

	if ((inodeIdx < 0) || (inodeIdx > FIO_MAX_FILES)){
		return -1;
	}
	if (inode[inodeIdx].type != PUSH_BUTTON_DEV){
		return -1;
	}

	int i;
	unsigned char on = '1';
	unsigned char off = '0';
	if (streamIdx == SW1) {
		i = sw1In();
	} else if (streamIdx == SW2) {
		i = sw2In();
	} else {
		return -1;
	}

	if(i == 1) {
		return (int) on;
	} else {
		return (int) off;
	}
}

/* open the lcd device file.
 * takes in name/id of the device
 * returns stream index (= inode index) for this device, or -1 if fails
 */
int lcdOpen(char * id_p, char *mode_p){
	if (!id_p){
		return -1;
	}

	int lcd;
	lcd = getLCDStream(id_p);
	if (lcd == -1){
		return -1;
	}

	/* since lcd entries/channels are reserved, no need to search inode table */
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p) {
		return -1;
	}

	if (pcb_p->stream[lcd].nextFreeIdx == FIO_MAX_STREAMS){
		/* stream was already opened */
		return lcd;
	}

	int inodeIdx = lcdCreate(lcd, id_p);

	if (inodeIdx == -1){
		/* could not be opened */
		return (-1);
	}

	if (inode[inodeIdx].accessCount >= 1){
		/* led accessed by another process*/
		return (-1);
	}else{
		/*reserve led for this process process*/
		inode[inodeIdx].accessCount++;
	}

	pcb_p->stream[lcd].device_p = &fioDevice[TWR_LCD];

	utlStrNCpy(pcb_p->stream[lcd].id, id_p, FIO_MAX_FILE_NAME);
	pcb_p->stream[lcd].position.currBlock_p = NULL; /* not used by lcd devices */
	pcb_p->stream[lcd].position.offset = 0; /* not used by lcd devices */

	pcb_p->stream[lcd].inodeIdx = (unsigned short) inodeIdx; /* note: inodeIdx = lcd */
	pcb_p->stream[lcd].nextFreeIdx = FIO_MAX_STREAMS;

	return (inodeIdx);
}

int lcdCreate(int lcd, char *id_p){
	if (lcd == -1){
		return -1;
	}

	if (inode[lcd].accessCount != USHRT_MAX) {
		return lcd;
	}

	inode[lcd].type = LCD;
	inode[lcd].lock = false;
	inode[lcd].accessCount = 0;
	inode[lcd].size = 0;

	int sts;
	sts = writeDirDevRec(id_p, lcd, DEV_DIR);

	if (sts == -1) {
		return -1;
	}

	return (lcd);
}
int lcdPutc(int c, myFILE fi){
	if ((fi < 0) || (fi > FIO_MAX_STREAMS)){
		return -1;
	}

	uint8_t streamIdx = fi;
	ProcessControlBlock *pcb_p = getCurrentPCB();

	if (pcb_p->stream[streamIdx].device_p == NULL) {
		/* stream not allocated */
		return -1;
	}

	if (pcb_p->stream[streamIdx].device_p->type != TWR_LCD){
		return -1;
	}

	int inodeIdx = pcb_p->stream[streamIdx].inodeIdx;

	if ((inodeIdx < 0) || (inodeIdx > FIO_MAX_FILES)){
		return -1;
	}
	if (inode[inodeIdx].type != LCD){
		return -1;
	}

	char str;
	char t[2];
	if (streamIdx == LCD) {
		if ((c < 32) || (c > 126)){
			snprintf(t, 1, "%x", c);
			str = t[1];
		}else{
			str = (char) c;
		}
	} else {
		return -1;
	}
	lcdcConsolePutc(&console, str);
	return 0;
}
int lcdGetc(myFILE fi){
	uartPuts(UART2_BASE_PTR, "You can't read from the LCD\r\n");
	return 0;
}

int anlgOpen(char * id_p, char *mode_p){
	if (!id_p){
		return -1;
	}

	int anlg;
	anlg = getAnlgStream(id_p);
	if (anlg == -1){
		return -1;
	}

	/* since analog entries/channels are reserved, no need to search inode table */
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p) {
		return -1;
	}

	if (pcb_p->stream[anlg].nextFreeIdx == FIO_MAX_STREAMS){
		/* stream was already opened */
		return anlg;
	}

	int inodeIdx = anlgCreate(anlg, id_p);

	if (inodeIdx == -1){
		/* could not be opened */
		return (-1);
	}

	if (inode[inodeIdx].accessCount >= 1){
		/* led accessed by another process*/
		return (-1);
	}else{
		/*reserve led for this process process*/
		inode[inodeIdx].accessCount++;
	}

	pcb_p->stream[anlg].device_p = &fioDevice[ANALOG];

	utlStrNCpy(pcb_p->stream[anlg].id, id_p, FIO_MAX_FILE_NAME);
	pcb_p->stream[anlg].position.currBlock_p = NULL; /* not used by analog devices */
	pcb_p->stream[anlg].position.offset = 0; /* not used by analog devices */

	pcb_p->stream[anlg].inodeIdx = (unsigned short) inodeIdx; /* note: inodeIdx = analog */
	pcb_p->stream[anlg].nextFreeIdx = FIO_MAX_STREAMS;

	return (inodeIdx);
}
int anlgCreate(int anlg, char *id_p){
	if (anlg == -1){
		return -1;
	}

	if (inode[anlg].accessCount != USHRT_MAX) {
		return anlg;
	}

	inode[anlg].type = ANALOG_DEV;
	inode[anlg].lock = false;
	inode[anlg].accessCount = 0;
	inode[anlg].size = 0;

	int sts;
	sts = writeDirDevRec(id_p, anlg, ANALOG_DIR);

	if (sts == -1) {
		return -1;
	}

	return (anlg);
}
int anlgPutc(int c, myFILE fi){
	uartPuts(UART2_BASE_PTR,
			"Writing to either of the analog devices isn't possible\r\n");
	return 0;
}
int anlgGetc(myFILE fi){
	if ((fi < 0) || (fi > FIO_MAX_STREAMS)){
		return -1;
	}

	uint8_t streamIdx = fi;
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (pcb_p->stream[streamIdx].device_p == NULL) {
		/* stream not allocated */
		return -1;
	}

	if (pcb_p->stream[streamIdx].device_p->type != ANALOG){
		return -1;
	}

	int inodeIdx = pcb_p->stream[streamIdx].inodeIdx;

	if ((inodeIdx < 0) || (inodeIdx > FIO_MAX_FILES)){
		return -1;
	}
	if (inode[inodeIdx].type != ANALOG_DEV){
		return -1;
	}

	if (streamIdx == POTENTIOMETER) {
		return (int) adc_read(ADC_CHANNEL_POTENTIOMETER);
	} else if (streamIdx == THERMISTOR) {
		return (int) adc_read(ADC_CHANNEL_TEMPERATURE_SENSOR);
	} else {
		return -1;
	}

	return 0;

}

int tsOpen(char * id_p, char *mode_p){
	if (!id_p){
		return -1;
	}

	int ts;
	ts = getTsStream(id_p);
	if (ts == -1){
		return -1;
	}

	/* since ts entries/channels are reserved, no need to search inode table */
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p) {
		return -1;
	}

	if (pcb_p->stream[ts].nextFreeIdx == FIO_MAX_STREAMS){
		/* stream was already opened */
		return ts;
	}

	int inodeIdx = tsCreate(ts, id_p);

	if (inodeIdx == -1){
		/* could not be opened */
		return (-1);
	}

	if (inode[inodeIdx].accessCount >= 1){
		/* led accessed by another process*/
		return (-1);
	}else{
		/*reserve led for this process process*/
		inode[inodeIdx].accessCount++;
	}

	pcb_p->stream[ts].device_p = &fioDevice[TOUCH_SENSOR];

	utlStrNCpy(pcb_p->stream[ts].id, id_p, FIO_MAX_FILE_NAME);
	pcb_p->stream[ts].position.currBlock_p = NULL; /* not used by lcd devices */
	pcb_p->stream[ts].position.offset = 0; /* not used by lcd devices */

	pcb_p->stream[ts].inodeIdx = (unsigned short) inodeIdx; /* note: inodeIdx = lcd */
	pcb_p->stream[ts].nextFreeIdx = FIO_MAX_STREAMS;

	return (inodeIdx);
}
int tsCreate(int ts, char *id_p){
	if (ts == -1){
		return -1;
	}

	if (inode[ts].accessCount != USHRT_MAX) {
		return ts;
	}

	inode[ts].type = TOUCH_SENSOR_DEV;
	inode[ts].lock = false;
	inode[ts].accessCount = 0;
	inode[ts].size = 0;

	int sts;
	sts = writeDirDevRec(id_p, ts, TS_DIR);

	if (sts == -1) {
		return -1;
	}

	return (ts);
}
int tsPutc(int c, myFILE fi){
	uartPuts(UART2_BASE_PTR, "...this is the push buttons all over again\r\n");
	return 0;
}
int tsGetc(myFILE fi){
	if ((fi < 0) || (fi > FIO_MAX_STREAMS)){
		return -1;
	}

	uint8_t streamIdx = fi;
	ProcessControlBlock *pcb_p = getCurrentPCB();

	if (pcb_p->stream[streamIdx].device_p == NULL) {
		/* stream not allocated */
		return -1;
	}

	if (pcb_p->stream[streamIdx].device_p->type != TOUCH_SENSOR){
		return -1;
	}

	int inodeIdx = pcb_p->stream[streamIdx].inodeIdx;

	if ((inodeIdx < 0) || (inodeIdx > FIO_MAX_FILES)){
		return -1;
	}
	if (inode[inodeIdx].type != TOUCH_SENSOR_DEV){
		return -1;
	}

	int status;
	unsigned char on = '1';
	unsigned char off = '0';
	if (streamIdx == TS1) {
		status = electrode_in(0);
	} else if (streamIdx == TS2) {
		status = electrode_in(1);
	} else if (streamIdx == TS3) {
		status = electrode_in(2);
	} else if (streamIdx == TS4) {
		status = electrode_in(3);
	} else {
		return -1;
	}

	/* Returns 1 when pushbutton is depressed and 0 otherwise */
	if(status) {
		return (int) on;
	} else {
		return (int) off;
	}
}

/* create without opening it:
 *   creates inode
 *   calls ramCreate
 *
 * takes int the file path name;
 * returns the inode index or -1 if fails
 *
 * creates inode
 * calls ramCreate
 */

int fileCreate(char * id_p, char *permSpec_p) {
	if (!id_p){
		return -1;
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
	if (id_p[0] == '/'){
		/* path starts with root dir */
		if (pcb_p->currentDirInode == -1) {
			/* root wasnt set yet; is set during pcb initialization */
			Position pos;
			inodeIdx = ramCreate(id_p, len, -1, pos, DIR_FILE, NULL);

			if (inodeIdx == -1){
				return -1;
			}
			return ROOT_DIR; /* dedicated entry in the stream table */
		} else {
			parentInode = ROOT_DIR;
			name_p++;
		}
	} else {
		/* path starts from current dir */
		parentInode = pcb_p->currentDirInode;
	}

	/* see if the requested file exists:  look for the file (in inode table), and create if not found */
	FNode fn;
	fn = findFile(name_p, parentInode, true, permSpec_p);

	inodeIdx = fn.self;

	if (inodeIdx == -1){
		return -1; /* not found and could not be created */
	}

	return (inodeIdx);

}

/* helper functions                        */

/* the next two function reserve/release inodes (create/delete files) in the inode table */

/* get the index of the first free inode:
 * takes in no arguments;
 * returns the inode index or FIO_MAX_FILES if there are no free inodes left
 */
unsigned short getFreeInodeIdx(void) {
	unsigned short inodeIdx = firstFreeInode;

	if (firstFreeInode != FIO_MAX_FILES) {
		/* reset pointer to first free inode entry */
		firstFreeInode = inode[firstFreeInode].nextFreeIdx;

		/* reset inode */
		inode[inodeIdx].type = NONE;
		inode[inodeIdx].numRec = 0;
		inode[inodeIdx].numFreeRec = 0;
		inode[inodeIdx].lock = false;
		inode[inodeIdx].accessCount = 0;
		inode[inodeIdx].firstBlock_p = NULL;
		inode[inodeIdx].size = 0;
	}
	return (inodeIdx);
}

/* frees the inode entry.
 * the new free inode is set to be the first free inode
 * takes in the inode index;
 * returns void;
 */
void releaseInodeIdx(unsigned short inodeIdx) {
	if ((inodeIdx < 0) || (inodeIdx >= FIO_MAX_FILES)){
		/* check for valid index */
		return;
	}

	myFree(inode[inodeIdx].user_p);

	/* free allocated blocks */
	Block *block_p = inode[inodeIdx].firstBlock_p;
	while (block_p != NULL) {
		Block *next_p = block_p->next_p;
		myFree(block_p->data_p);
		myFree(block_p);
		block_p = next_p;
	}

	/* reset inode */
	inode[inodeIdx].processID = -1;

	inode[inodeIdx].nextFreeIdx = firstFreeInode;
	firstFreeInode = inodeIdx;

}

/* finds the next node name in the path name.
 * searches for the next '/'; if name ends with '/' - it is a dir name
 * else it is a file name.
 * takes in the path name;
 * returns the length of the first node found in this path name
 */
int getNodeName(char *name_p) {
	if (!name_p){
		return -1;
	}

	int len = utlStrLen(name_p);
	if (len == 0){
		return -1;
	}

	/* get next node */
	int i;

	i = utlCharFind(name_p, '/', len);

	if (i != -1){
		/* dir name */
		len = i;
	}

	if ((len == 0) || !validNodeName(name_p, len)){
		return -1;
	}else{
		return len;
	}
}

/* validates the node name.
 * valid name: first character must be a letter; name may consist of letters, _, .  and numbers;
 * cannot be longer than FIO_MAX_FILE_NAME.
 * takes in node name and node name length;
 * returns boolean
 */
bool validNodeName(char *name_p, unsigned short len) {
	if (!name_p){
		return false;
	}
	if (len <= 0){
		return false;
	}

	if (len > FIO_MAX_FILE_NAME){
		return false;
	}

	if ((name_p[0] < 'A') || (name_p[0] > 'z')){
		return false;
	}
	int i;
	for (i = 1; (i < len) && (name_p[i] != '\0'); i++) {
		if (((name_p[i] < 'A') || (name_p[i] > 'z'))
				&& ((name_p[i] < '0') || (name_p[i] > '9'))
				&& (name_p[i] != '_') && (name_p[i] != '.')){
			return false;
		}
	}
	return true;
}

/* returns the type of a hardware device corresponding to its id
 * preset in hwDevice[].
 * takes in device id;
 * returns device type;
 */
int getHwType(char *id_p) {
	if (!id_p){
		return -1;
	}
	if (!validPathName(id_p)){
		return -1;
	}

	int type = -1;

	int i;
	bool match;
	for (i = 0, match = false; (i < numHwDevice) && !match; i++) {
		if (utlStrCmp(id_p, hwDevice[i].id_p)) {
			type = (int) hwDevice[i].type;
			match = true;
		}
	}
	return type;
}

/* validate path name.
 * takes in path name;
 * returns boolean
 */
bool validPathName(char *name_p) {
	if (!name_p){
		return false;
	}

	int len = utlStrLen(name_p);
	if (len <= 0){
		return false;
	}

	if (len > FIO_MAX_PATH_NAME){
		return false;
	}

	if ((name_p[0] != '/') && ((name_p[0] < 'A') || (name_p[0] > 'z'))){
		return false;
	}

	int i;
	for (i = 1; (i < len) && (name_p[i] != '\0'); i++) {
		if (((name_p[i] < 'A') || (name_p[i] > 'z'))
				&& ((name_p[i] < '0') || (name_p[i] > '9'))
				&& (name_p[i] != '/') && (name_p[i] != '_')
				&& (name_p[i] != '.')){
			return false;
		}
	}
	return true;
}

/****************************************************************/

int writeDirDevRec(char* name_p, int inodeIdx, int parentInode) {
	/* record */
	DirRecord *drec_p;

	/* check if there are freed record slots from previous files being deleted */
	/* if so, find the first free record slot and updated with current node info */
	bool updated = false;

	unsigned short size = 0;
	Block *block_p = inode[parentInode].firstBlock_p;
	drec_p = (DirRecord *) block_p->data_p;
	while (!updated && drec_p != NULL) {
		if (drec_p->fileName[0] == '\0') {
			utlStrMCpy(drec_p->fileName, name_p, utlStrLen(name_p),
					FIO_MAX_FILE_NAME);
			drec_p->inodeIdx = inodeIdx;
			inode[parentInode].numFreeRec--;
			updated = true;
		} else {
			size += sizeof(DirRecord);
			if ((FIO_BLOCK_SIZE - size) < sizeof(DirRecord)) {
				block_p = block_p->next_p;
				if (!block_p)
					drec_p = NULL;
				else
					drec_p = (DirRecord *) block_p->data_p;
			} else
				drec_p++;
		}
	}

	if (!updated)
		return -1;

	inode[parentInode].numRec++;
	return 0;

}

int getPresetIdx(char *id_p, unsigned short len) {
	if (!id_p) {
		return -1;
	}

	if (utlStrNCmp("dev", id_p, len)) {
		return DEV_DIR;
	} else if (utlStrNCmp("led", id_p, len)) {
		return LED_DIR;
	} else if (utlStrNCmp("pb", id_p, len)) {
		return PB_DIR;
	} else if (utlStrNCmp("analog", id_p, len)) {
		return ANALOG_DIR;
	} else if (utlStrNCmp("ts", id_p, len)) {
		return TS_DIR;
	} else {
		return -1;
	}
}

 /* get stream dedicated to an STD file.
  * takes in device id;
  * returns its dedicated stream index, or -1 if fails
  */
int getStdStream(char *id_p) {
	if (!id_p){
		return -1;
	}

	if (utlStrCmp("STDIN", id_p)){
		return STDIN;
	}else if (utlStrCmp("STDOUT", id_p)){
		return STDOUT;
	}else if (utlStrCmp("STDERR", id_p)){
		return STDERR;
	}else{
		return -1;
	}

}

/* get stream dedicated to a LED device.
 * takes in device id;
 * returns its dedicated stream index, or -1 if fails
 */
int getLedStream(char *id_p) {
	if (!id_p){
		return -1;
	}

	if (utlStrCmp("ORANGE", id_p)){
		return ORANGE;
	}else if (utlStrCmp("YELLOW", id_p)){
		return YELLOW;
	}else if (utlStrCmp("GREEN", id_p)){
		return GREEN;
	}else if (utlStrCmp("BLUE", id_p)){
		return BLUE;
	}else{
		return -1;
	}
}

/* get stream dedicated to a PUSH BUTTON device.
 * takes in device id;
 * returns its dedicated stream index, or -1 if fails
 */
int getPBStream(char *id_p) {
	if (!id_p){
		return -1;
	}

	if (utlStrCmp("SW1", id_p)){
		return SW1;
	}else if (utlStrCmp("SW2", id_p)){
		return SW2;
	}else{
		return -1;
	}
}

/* get stream dedicated to the LCD device.
 * takes in device id;
 * returns its dedicated stream index, or -1 if fails
 */
int getLCDStream(char *id_p){
	if (!id_p) {
		return -1;
	}

	if (utlStrCmp("LCD", id_p)) {
		return LCD;
	} else {
		return -1;
	}
}

/* get stream dedicated to an ANALOG device.
 * takes in device id;
 * returns its dedicated stream index, or -1 if fails
 */
int getAnlgStream(char *id_p){
	if (!id_p) {
		return -1;
	}

	if (utlStrCmp("POTENTIOMETER", id_p)){
		return POTENTIOMETER;
	}else if (utlStrCmp("THERMISTOR", id_p)){
		return THERMISTOR;
	}else{
		return -1;
	}
}

/* get stream dedicated to a TOUCH SENSOR device.
 * takes in device id;
 * returns its dedicated stream index, or -1 if fails
 */
int getTsStream(char *id_p){
	if (!id_p) {
		return -1;
	}

	if (utlStrCmp("TS1", id_p)){
		return TS1;
	}else if (utlStrCmp("TS2", id_p)){
		return TS2;
	}else if (utlStrCmp("TS3", id_p)){
		return TS3;
	}else if (utlStrCmp("TS4", id_p)){
		return TS4;
	}else{
		return -1;
	}
}

bool isAnlg(myFILE fi){
	return (fi == POTENTIOMETER) || (fi == THERMISTOR);
}

void purgeInode(unsigned short inodeIdx) {
	if ((inodeIdx < 0) || (inodeIdx >= FIO_MAX_FILES)) {
		/* check for valid index */
		return;
	}

	/* keep first block and free all the rest */
	Block *block_p = inode[inodeIdx].firstBlock_p->next_p;
	while (block_p != NULL) {
		Block *next_p = block_p->next_p;
		myFree(block_p->data_p);
		myFree(block_p);
		block_p = next_p;
	}

	inode[inodeIdx].firstBlock_p->next_p = NULL;
	memSet(inode[inodeIdx].firstBlock_p->data_p, 0, FIO_BLOCK_SIZE, false);
	inode[inodeIdx].size = 0;
}

void setUser(int inodeIdx) {
	if ((inodeIdx < 0) || (inodeIdx > FIO_MAX_FILES)) {
		return;
	}
	inode[inodeIdx].user_p = memAlloc(sizeof(usr_t), false);
	usrGetCurrent(inode[inodeIdx].user_p);
}

/* permissions mask: 9 bits:
 *    default perm (e.g. file is not explicitely created but fopen-ed) is set to read and write for all;
 */
void setPermissions(int inodeIdx, char *permSpec_p) {

	if ((inodeIdx < 0) || (inodeIdx > FIO_MAX_FILES)) {
		return;
	}

	inode[inodeIdx].perm.mask = 0;

	if (!permSpec_p) {
		inode[inodeIdx].perm.bit.oR = 1;
		inode[inodeIdx].perm.bit.oW = 1;
		inode[inodeIdx].perm.bit.oX = 1;

		inode[inodeIdx].perm.bit.gR = 1;
		inode[inodeIdx].perm.bit.gW = 1;

		inode[inodeIdx].perm.bit.wR = 1;
		inode[inodeIdx].perm.bit.wW = 1;

	}

	else {
		unsigned len = utlStrLen(permSpec_p);

		if ((len >= 1) && (permSpec_p[0] != '-')) {
			inode[inodeIdx].perm.bit.oR = 1;
		}
		if ((len >= 2) && (permSpec_p[1] != '-')) {
			inode[inodeIdx].perm.bit.oW = 1;
		}
		if ((len >= 3) && (permSpec_p[2] != '-')) {
			inode[inodeIdx].perm.bit.oX = 1;
		}

		if ((len >= 1) && (permSpec_p[3] != '-')) {
			inode[inodeIdx].perm.bit.gR = 1;
		}
		if ((len >= 2) && (permSpec_p[4] != '-')) {
			inode[inodeIdx].perm.bit.gW = 1;
		}
		if ((len >= 3) && (permSpec_p[5] != '-')) {
			inode[inodeIdx].perm.bit.gX = 1;
		}

		if ((len >= 1) && (permSpec_p[6] != '-')) {
			inode[inodeIdx].perm.bit.wR = 1;
		}
		if ((len >= 2) && (permSpec_p[7] != '-')) {
			inode[inodeIdx].perm.bit.wW = 1;
		}
		if ((len >= 3) && (permSpec_p[8] != '-')) {
			inode[inodeIdx].perm.bit.wX = 1;
		}
	}
}

bool getPermission(int inodeIdx, char *access_p) {
	if ((inodeIdx < 0) || (inodeIdx > FIO_MAX_FILES) || !access_p) {
		return false;
	}

	usr_t user;
	usrGetCurrent(&user);

	if ((*access_p == 'w') || (*access_p == 'a')) {
		if (inode[inodeIdx].perm.bit.wW) {
			return true;
		}

		if (utlStrNCmp(user.name, inode[inodeIdx].user_p->name,
				USR_MAX_NAME_LEN) && inode[inodeIdx].perm.bit.oW) {
			return true;
		}

		if (utlStrNCmp(user.group, inode[inodeIdx].user_p->group,
				USR_MAX_NAME_LEN) && inode[inodeIdx].perm.bit.gW) {
			return true;
		}

	} else if (*access_p == 'r') {
		if (inode[inodeIdx].perm.bit.wR) {
			return true;
		}

		if (utlStrNCmp(user.name, inode[inodeIdx].user_p->name,
				USR_MAX_NAME_LEN) && inode[inodeIdx].perm.bit.oR) {
			return true;
		}

		if (utlStrNCmp(user.group, inode[inodeIdx].user_p->group,
				USR_MAX_NAME_LEN) && inode[inodeIdx].perm.bit.gR) {
			return true;
		}
	} else if (*access_p == 'x') {
		if (inode[inodeIdx].perm.bit.wX) {
			return true;
		}

		if (utlStrNCmp(user.name, inode[inodeIdx].user_p->name,
				USR_MAX_NAME_LEN) && inode[inodeIdx].perm.bit.oX) {
			return true;
		}

		if (utlStrNCmp(user.group, inode[inodeIdx].user_p->group,
				USR_MAX_NAME_LEN) && inode[inodeIdx].perm.bit.gX) {
			return true;
		}
	}

	return false;

}

int logWrite(FioErrno error, char *str1_p, char *str2_p) {
	char msg[FIO_LOG_LINE];
	bool inSVC = true;

	ProcessControlBlock *pcb_p = getCurrentPCB();
	usr_t user;
	usrGetCurrent(&user);

	if (!str1_p) {
		str1_p = "";
	}else if(utlStrCmp(str1_p, FIO_LOG_FILE)){
		inSVC = false;
	}
	if (!str2_p) {
		str2_p = "";
	}

	switch (error) {
	case (FIO_INVREQ_FILE_OPEN):
		sprintf(msg,
				"::%-8.8s: attempted to open file %.50s for %.2s access without permission\r\n",
				user.name, str1_p, str2_p);
		break;
	case (FIO_INVREQ_FILE_DELETE):
		sprintf(msg,
				"::%-8.8s: attempted to delete file  %.50s without permission\r\n",
				user.name, str1_p);
		break;
	case (FIO_INVREQ_FILE_PURGE):
		inSVC = false;
		sprintf(msg,
				"::%-8.8s: attempted to purge file  %.50s without permission\r\n",
				user.name, str1_p);
		break;
	case (FIO_INVREQ_FILE_REWIND): /* forbiden if opened for append */
		sprintf(msg,
				"::%-8.8s: attempted to rewind file  %.50s opened in append mode\r\n",
				user.name, str1_p);
		break;
	case (FIO_INVREQ_WRITE):
		sprintf(msg,
				"::%-8.8s: attempted to write to file  %.50s opened for read only\r\n",
				user.name, str1_p);
		break;
	case (FIO_INVREQ_READ):
		sprintf(msg,
				"::%-8.8s: attempted to read file  %.50s opened for write only\r\n",
				user.name, str1_p);
		break;
	default:
		inSVC = false;
		sprintf(msg, "::%-20.20s: %.50s %.50s\r\n", user.name, str1_p, str2_p);
		break;
	}

	int sts;
	if (is_set()) {
		utlDate_t date = tmCurrentRTC(inSVC);
		char dateStr[50];

		sprintf(dateStr, "%-10.10s  %02d, %d %02d:%02d:%02d.%-10u ::",
				date.month_p, date.day, date.year, date.hour, date.min,
				date.sec, date.msec);
		sts = ramPuts(dateStr, pcb_p->fiLog);
	}
	sts = ramPuts(msg, pcb_p->fiLog);

	return sts;
}

int logRead(char *str_p, int len) {
	if (!str_p) {
		return -1;
	}

	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p) {
		return -1;
	}

	/* only admin can open log file with access mode 'r'; and 'a' for all other users */
	usr_t user;
	usrGetCurrent(&user);

	/* only admin can open log file with access mode 'r'; and 'a' for all other users */
	if (user.fiLog <= LAST_RESERVED_TYPE) {
		logWrite(FIO_INVREQ_READ, FIO_LOG_FILE, NULL);
		return -1;
	}

	return (ramGets(str_p, len, user.fiLog));
}

int logRewind(void) {
	usr_t user;
	usrGetCurrent(&user);

	if (user.fiLog == -1) {
		logWrite(FIO_INVREQ_FILE_REWIND, FIO_LOG_FILE, NULL);
		return -1;
	}

	return (fileRewind(user.fiLog));
}

int logPurge(void) {
	usr_t user;
	usrGetCurrent(&user);

	if (user.fiLog == -1) {
		//SVCgetTimeStamp();
		logWrite(FIO_INVREQ_FILE_PURGE, FIO_LOG_FILE, NULL);
		return -1;
	}
	fileRewind(user.fiLog);

	/* permission checking is doen in mypurge() */
	int sts = filePurge(FIO_LOG_FILE);

	if (sts == -1) {
		//SVCgetTimeStamp();
		logWrite(FIO_INVREQ_FILE_PURGE, FIO_LOG_FILE, NULL);
		return -1;
	}

	/* re-open append mode stream: stream with append access mode cannot be rewound */
	ProcessControlBlock *pcb_p = getCurrentPCB();
	if (!pcb_p) {
		//SVCgetTimeStamp();
		logWrite(FIO_INVREQ_FILE_PURGE, FIO_LOG_FILE, NULL);
		return -1;
	}
	fileCloseX(pcb_p->fiLog, pcb_p);
	pcb_p->fiLog = myfopen(FIO_LOG_FILE, "a");

	    return sts;
}

int fileCloseX(myFILE fi, ProcessControlBlock *pcb_p) {
	if (!pcb_p) {
		pcb_p = getCurrentPCB();
	}

	if ((fi < 0) || (fi > FIO_MAX_STREAMS)) {
		return -1;
	}
	uint8_t streamIdx = (uint8_t) fi;

	/* update inode entry */
	inode[pcb_p->stream[streamIdx].inodeIdx].accessCount--;
	pcbReleaseStreamIdxX(streamIdx, pcb_p);
	return (0);
}
