#ifndef FIOUTL_
#define FIOUTL_

/* systtem headers */
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
#include "fio.h"
#include "pcb.h"

/* constants */

/* type definitions */

/* global variable declarations */

#ifndef ALLOCATE_
#define EXTERN_ extern

#else
#define EXTERN_
#endif  /* ALLOCATE_ */

/* function declarations */

/* device independent */

int fileClose(myFILE fi); /* RAM files only */
int fileCreate(char *id_p, char *permSpec_p); /* RAM files only */
int fileDelete(char *id_p); /* RAM files only */
int fileRewind(myFILE fi);
int filePurge(char *id_p);/* RAM files only */

/* device specific IO functions */

int stdOpen(char *id_p, char *mode_p);
int stdPutc(int c, myFILE fi);
int stdGetc(myFILE fi);
int stdCreate(int std); /* will be called by stdOpen only */

int ramOpen(char * id_p, char *mode_p); /* inserts stream entry = opens an IO channel to a file of ram device type*/
int ramCreate(char *name_p, unsigned short len, int parentInode,
		Position position, FileType type, char *permSpec_p);
int ramPutc(int c, myFILE fi); /* writes char to a file = c => (stream=>inode=>file of ram device type) */
int ramGetc(myFILE fi); /* reads char from a file = c <= (stream=>inode=>file of ram device type) */

uint8_t ramStreamSet(char *id_p, unsigned short inodeIdx, char *mode_p);
FNode findFile(char *name_p, int parentInode, bool create, char *permSpec_p);
int writeDirRec(char *name_p, unsigned short len, int inodeIdx, int parentInode,
		Position position);
int eraseDirRec(int inodeIdx, int parentInode);
int dirCreate(int parentIdx);
int dirDevCreate(int parentIdx, int inodeIdx);
int regCreate(void);
int ramInodeSet(unsigned short inodeIdx, FileType type);
int searchDir(char *nodeName_p, unsigned short len, int parentInode,
		Position *position_p);

int ledOpen(char * id_p, char *mode_p);
int ledCreate(int led, char *id_p);
int ledPutc(int c, myFILE fi);
int ledGetc(myFILE fi);

int pbOpen(char * id_p, char *mode_p);
int pbCreate(int pb, char *id_p);
int pbPutc(int c, myFILE fi);
int pbGetc(myFILE fi);

int lcdOpen(char * id_p, char *mode_p);
int lcdCreate(int pb, char *id_p);
int lcdPutc(int c, myFILE fi);
int lcdGetc(myFILE fi);

int anlgOpen(char * id_p, char *mode_p);
int anlgCreate(int anlg, char *id_p);
int anlgPutc(int c, myFILE fi);
int anlgGetc(myFILE fi);

int tsOpen(char * id_p, char *mode_p);
int tsCreate(int pb, char *id_p);
int tsPutc(int c, myFILE fi);
int tsGetc(myFILE fi);

/* helper functions */
unsigned short getFreeInodeIdx(void);
void releaseInodeIdx(unsigned short inodeIdx);
int getHwType(char *id_p);
int getPresetIdx(char *id_p, unsigned short len);
bool validNodeName(char *name_p, unsigned short len);
bool validPathName(char *name_p);
int getNodeName(char *name_p);
int writeDirDevRec(char* name_p, int inodeIdx, int parentInode);

int getStdStream(char *id_p);
int getLedStream(char *id_p);
int getPBStream(char *id_p);
int getLCDStream(char *id_p);
int getAnlgStream(char *id_p);
int getTsStream(char *id_p);

int ramPuts(char *str, myFILE fi);
int ramGets(char *str_p, int len, myFILE fi);

int logWrite(FioErrno error, char *str1_p, char *str2_p);
int logRead(char *str_p, int len);
int logRewind(void);
int logPurge(void);

void setPermissions(int inodeIdx, char *permSpec_p);
void setUser(int inodeIdx);
bool getPermission(int inodeIdx, char *mode_p);

void purgeInode(unsigned short inodeIdx);
bool isAnlg(myFILE fi);
int stubI(int num);
int stubC(char* str);
int stubPuts(char *str, myFILE fi);
int stubGets(char *str, int len, myFILE fi);
int stubCr(char* str, char* spec);

int fileCloseX(myFILE fi, ProcessControlBlock *pcb_p);
/* macros */

#endif /* FIOUTL_ */
