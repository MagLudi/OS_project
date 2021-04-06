#ifndef FIO_
#define FIO_

/* system headers */
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <float.h>
#include <string.h> 
#include <stdbool.h>
#include <errno.h>
#include "utl.h"
#include "usr.h"

/* constants */

#define FIO_MAX_FILES    64000 //increase max files to specified values in PS 3
#define FIO_MAX_STREAMS  UINT8_MAX /*UCHAR_MAX*/
#define FIO_MAX_FILE_NAME 14 /* to fulfill the requirement for the support of at least 64K files; */
/* 4.5.2, p. 324 */
#define FIO_MAX_PATH_NAME UCHAR_MAX
#define FIO_BLOCK_SIZE (512*2)
#define FIO_MAX_MODE 2
#define FIO_LOG_FILE "/security.log"
#define FIO_LOG_LINE 255

/* enums */

typedef enum {
	FIO_NO_ERROR = 0,
	FIO_INIT_FAILED,
	FIO_INVREQ_FILE_OPEN,
	FIO_INVREQ_FILE_DELETE,
	FIO_INVREQ_FILE_PURGE,
	FIO_INVREQ_FILE_REWIND, /* forbidden if opened for append */
	FIO_INVREQ_WRITE,
	FIO_INVREQ_READ,
	FIO_LAST_ERROR
} FioErrno;

typedef enum {
	FIRST_RESERVED_TYPE,
	STD_FILE = FIRST_RESERVED_TYPE,
	STDIN = STD_FILE,
	STDOUT,
	STDERR,
	ROOT_DIR,
	DEV_DIR,
	LED_DIR,
	PB_DIR,
	ANALOG_DIR,
	TS_DIR,
	HW_DEV,
	LED_DEV = HW_DEV,
	ORANGE = LED_DEV,
	YELLOW,
	GREEN,
	BLUE,
	PUSH_BUTTON_DEV,
	SW1 = PUSH_BUTTON_DEV,
	SW2,
	LCD,
	ANALOG_DEV,
	POTENTIOMETER = ANALOG_DEV,
	THERMISTOR,
	TOUCH_SENSOR_DEV,
	TS1 = TOUCH_SENSOR_DEV,
	TS2,
	TS3,
	TS4,
	LAST_RESERVED_TYPE = TS4,
	REG_FILE, /* regular file */
	DIR_FILE, /* directory */
	NONE
} FileType;

typedef enum {
	STD, RAM, LED, PUSH_BUTTON, ANALOG, TWR_LCD, TOUCH_SENSOR, FIO_DEV_MAX
} DevType;

/* type definitions */

typedef int myFILE; /* index into stream table */

typedef struct {
	DevType type;
	char *id_p;
} HwDevice;

typedef struct {
	uint8_t type; /* DeviceType */
	int (*fopen)(char *id_p, char *mode_p);
	int (*fclose)(myFILE fi);
	int (*fgetc)(myFILE fi);
	int (*fputc)(int c, myFILE fi);
	int (*fgets)(char *s_p, int len, myFILE fi);
	int (*fputs)(char *s_p, myFILE fi);
	int (*create)(char *id_p, char *permSpec_p);
	int (*delete) (char *id_p);
	int (*rewind)(myFILE fi);
	int (*purge)(char *id_p);
} Device;

typedef struct {

} AccessTime;

typedef struct {

} AccessRights;

/* permissions bit field struct instead of boolean flags as originally thought */
#define FIO_PERM_BIT 9;
typedef struct {
	unsigned oR :1; /* owner read */
	unsigned oW :1; /* owner write */
	unsigned oX :1; /* owner execute */

	unsigned gR :1; /* group read */
	unsigned gW :1; /* group write */
	unsigned gX :1; /* group execute */

	unsigned wR :1; /* world read */
	unsigned wW :1; /* world write */
	unsigned wX :1; /* world execute */

} Permissions;

/* 4.3.3 dir file record */
typedef struct {
	char fileName[FIO_MAX_FILE_NAME+1];
	unsigned short inodeIdx;
} DirRecord;

typedef struct block_s {
	char *data_p;
	struct block_s *next_p;
	struct block_s *prev_p;
} Block;

typedef struct {
	Block *currBlock_p;
	unsigned short offset;
} Position;

/* to list inodes: all existing files system wide */
typedef struct inode_s {
	usr_t *user_p;
	union {
		uint16_t mask;
		Permissions bit;
	} perm;
	int processID; /* creator/owner process ID */
	uint8_t type; /* FileType: dir/reg file */
	unsigned short numRec; /* if dir - num subdir records */
	unsigned short numFreeRec; /* num erased but not removed records ready to be over written with new data */
	bool lock;
	AccessTime accessTime; /* !!not used at this time */
	unsigned short accessCount; /* how many streams(opened/active instances) are associated with this file; !!not used at this time*/
	Block *firstBlock_p;
	UtlAddress_t size; /* total number of bytes written to this file; number of chars in Block.data_p chain */
	unsigned short nextFreeIdx; /* next free entry on inode array; only a free inode entry has a valid index */
	/* the last free inode will point to max size of array
	 * of inode; used together with firstFreeInode global;
	 * every time an inode is freed it will becomde new firstFreeInode and point to what was
	 * previously firstFreeInode */
} INode;

typedef struct {
	int self;
	int parent;
} FNode;
/* FILE * or  index into stream table ???
 * to list all streams - per process
 * more than one caller-per-process (and more than one process)  can be pointing to the same inode */
typedef struct {
	Device *device_p;
	char id[FIO_MAX_FILE_NAME+1]; /* file name */
	char mode[FIO_MAX_MODE + 1]; /* AccessMode; "r","r+","w","w+","a","a+","x" */
	Position position; /* current position */
	unsigned short inodeIdx; /* index into inode array */
	unsigned short nextFreeIdx; /* next free entry on stream array; only a free stream entry has a valid index; */
	/* the last free stream will point to max size of array
	 * used together with firstFreeStream in BCD
	 * every time an stream is freed (when file is closed), it will become the new
	 * firstFreeStream and point to  what was previously firstFreeInode */
} Stream; /* used as file descriptor */

/* global variable declarations */

#ifndef ALLOCATE_
extern unsigned short numHwDevice;
extern HwDevice hwDevice[];
extern bool fioInitialized;
#define EXTERN_ extern

#else
bool fioInitialized = false;
const unsigned short numHwDevice = 16;
const HwDevice hwDevice[] = {{STD, "STDIN"},
							 {STD, "STDOUT"},
							 {STD, "STDERR"},
							 {LED, "ORANGE"},
							 {LED, "YELLOW"},
							 {LED, "GREEN"},
							 {LED, "BLUE"},
							 {PUSH_BUTTON, "SW1"},
							 {PUSH_BUTTON, "SW2"},
							 {TWR_LCD, "LCD"},
							 {ANALOG, "POTENTIOMETER"},
							 {ANALOG, "THERMISTOR"},
							 {TOUCH_SENSOR, "TS1"},
							 {TOUCH_SENSOR, "TS2"},
							 {TOUCH_SENSOR, "TS3"},
							 {TOUCH_SENSOR, "TS4"}};
#define EXTERN_
#endif  /* ALLOCATE_ */

EXTERN_ INode *inode;
EXTERN_ int firstFreeInode;
EXTERN_ Device fioDevice[FIO_DEV_MAX]; /* device table */

/* function declarations */

FioErrno fioInit(void);
myFILE myfopen(char *name_p, char *mode_p);
int myfnclose(char *name_p);
int myfclose(myFILE fi);
int mycreate(char *name_p, char *permSpec_p);
int mydelete(char *name_p);
int myrewind(myFILE fi);
int myfputc(int c, myFILE fi);
int myfgetc(myFILE fi);
int myopendir(char *id_p);
char *getDirRecord(uint8_t streamIdx, char *fileName_p);
int getPresetDev(char *id_p);
int myfputs(char *s_p, myFILE fi);
int myfgets(char *s_p, int len, myFILE fi);
int mypurge(char *name_p);

void printStr(char *str);
char getChar(bool echo);

#endif /* FIO_ */
