/* mem.c contains memory management functions; memory to be allocated is currently set to a maximum of 128M bytes;
 * besides allocating/dealocating memory,  memory management methods include assigning values in allocated memory, 
 * and confirming that the expected values are stored.
 * this module also contains aditional functions and globals that are specificaly used
 * as helper functions and are therefore only accessible here.
 *
 * the memory management for allocation/deallocation uses two linked lists: 
 *  1. list of allocated segments, sorted by address
 *  2. list of free segments sorted by size
 *
 * given that,  'first fit' = 'best fit'  method to determine allocation is optimal.
 *
 * both lists - for allocated memory and free memory - are double linked; making free list link contain a pointer
 * to the prev link, does not waste memory (since the free link size is not larger than the alloc link size), and
 * it makes removing the link and merging address-adjacent free links easy and fast.
 *
 *
 *
 * Primary functions:
 *		myMalloc(): allocate some memory of a given size for use
 *		myFree(): deallocate memory for future use
 *		myFreeErrorCode(): deallocate memory for future use; returns error status
 *		memoryMap(): print a table of free and used memory
 *		myMemset(): assigns a value to a certain region of memory
 *		myMemchk(): check to see if a certain value is stored in a particular region of memory
 */

/* sys include files */
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "mem.h"
#include "uart.h"
#include "shell.h"
#include "pcb.h"
#include "sdram.h"
#include "lcdc.h"
#include "svc.h"

/* globals - not shared with other modules */
static char *mem_p = NULL;
static char *memLastAddress_p;
static MemAllocHeader memAllocHeader;
static MemFreeHeader memFreeHeader;
static unsigned memAllocLinkSize;

/* internal function declarations - to be used by mem utility only */
unsigned memModDouble(unsigned size);
void memInit(void);
void memFreeListInit(void);
void memAllocListInit(void);
MemAllocLink *memFindAllocPrev_p(MemAllocLink *alloc_p);
bool memMatchAlloc(MemAllocLink *alloc_p);
MemAllocLink *memInsertAllocLink_p(char *address_p, unsigned size);
void memRemoveAllocLink(MemAllocLink *alloc_p);
MemFreeLink *memFindSufficientFree(unsigned size);
MemFreeLink *memFindFreePrev_p(MemFreeLink *free_p, MemFreeLink *start_p);
bool memMatchFree_p(MemFreeLink *free_p);
MemFreeLink *memInsertFreeLink_p(char *address_p, unsigned size,
		MemFreeLink *freeFirst_p);
void memRemoveFreeLink(MemFreeLink *free_p);
void memMergeFree(char *address_p, unsigned size, MemFreeLink *adjPrior_p,
		MemFreeLink *adjPost_p);
unsigned int memCheckAddress(void *address_p);
void *memAlloc(unsigned size, int pId);
MemErrno memSet(void *address_p, int val, unsigned nbytes, bool checkPID);

/* function definitions */

/* allocate some memory of a certain size for use. Allocates memory based
 * on 8-byte allignment.
 */
void *myMalloc(unsigned size) {
	return (memAlloc(size, 0));
}
void *memAlloc(unsigned size, int pId) {

	if ((size < 0) || (size > (MEM_MAX_SIZE - memAllocLinkSize)))
		MEM_RETURN(MEM_INV_SIZE, NULL);

	memErrno = MEM_NO_ERROR;
	if (size == 0)
		return NULL;

	if (mem_p == NULL)
		memInit();

	/* ensure an 8-byte allignment*/
	size = memModDouble(size);
	unsigned requiredSize = size + memAllocLinkSize;

	MemFreeLink *free_p;
	MemAllocLink *alloc_p = NULL;
	void *address_p = NULL;

	di();
	free_p = memFindSufficientFree(requiredSize);
	if (free_p == NULL) {
		ei();
		MEM_RETURN(MEM_ALLOC_FAILED, NULL);
	}

	/* check if unused space is to small for future allocations */
	unsigned leftSize = free_p->size - requiredSize;

	if (leftSize <= memAllocLinkSize) {
		/* leftover space too small to be allocate later; add to space being reserved - to reduce fragmentation */
		size += leftSize;
		requiredSize += leftSize;
		leftSize = 0;
	}

	address_p = (char *) free_p;

	/* removed used free link */
	memRemoveFreeLink(free_p);
	ei();

	/* insert new alloc link */
	alloc_p = memInsertAllocLink_p(address_p, size);
	if (alloc_p == NULL) {
		MEM_RETURN(MEM_ALLOC_FAILED, NULL);
	}

	if (pId == 0){
		alloc_p->processID = pid();
	}else if (pId == -1){
		alloc_p->processID = NO_PID;
	}else{
		alloc_p->processID = pId;
	}

	/* if unused space left, create new free link */
	if (leftSize > 0) {
		di();
		char *newFree_p = (address_p + requiredSize);
		char *endFree_p = newFree_p + leftSize;
		MemFreeLink *adjPrior_p = NULL;
		MemFreeLink *adjPost_p = NULL;

		if ((alloc_p->next_p == NULL)
				|| (endFree_p < (char *) alloc_p->next_p)) {
			adjPost_p = (MemFreeLink *) endFree_p;
		}

		/* merge and insert new free link */
		memMergeFree(newFree_p, leftSize, adjPrior_p, adjPost_p);
		ei();
	}

	address_p += memAllocLinkSize;

	return (address_p);
}

/* deallocate memory for future use; frees the section of memory with the start address passed
 * in and merges it with any address adjacent free memory segments before creating an new link 
 * in the free link list.  unlike myFree, this returns an error status, which indicates failure 
 * in the event the memory in question was not allocated, or belongs to a different process.
 */
MemErrno myFreeErrorCode(void *address_p, int pId) {

	if ((mem_p == NULL) || (address_p < (void *) (mem_p + memAllocLinkSize))
			|| (address_p >= (void *) (memLastAddress_p - memAllocLinkSize))) {
		return (MEM_NOT_ALLOCATED);
	}

	if (address_p == NULL) {
		return (MEM_NO_ERROR);
	}

	char *startAddress_p = (char*) address_p - memAllocLinkSize;
	MemAllocLink *alloc_p = (MemAllocLink *) startAddress_p;

	if (!memMatchAlloc(alloc_p)) {
		/* was not allocated */
		return (MEM_NOT_ALLOCATED);
	}
	if (pId <= 0){
		pId = pid();
	}
	if ((alloc_p->processID != NO_PID) && (alloc_p->processID != pId)){
		return (MEM_NOT_OWNED);
	}

	unsigned size = alloc_p->size + memAllocLinkSize;

	/* check if there adjacent free sections  */
	char *endAlloc_p;
	char *endFree_p;

	MemFreeLink *adjPrior_p = NULL;
	MemFreeLink *adjPost_p = NULL;

	di();
	if (alloc_p->prev_p == NULL) {
		if (startAddress_p != mem_p) {
			adjPrior_p = (MemFreeLink *) mem_p;
		} else {
			adjPrior_p = NULL;
		}
	} else {
		endAlloc_p = ((char *) alloc_p->prev_p) + alloc_p->prev_p->size
				+ memAllocLinkSize;
		if (endAlloc_p < (char *) alloc_p) {
			adjPrior_p = (MemFreeLink *) endAlloc_p;
		} else {
			adjPrior_p = NULL;
		}
	}

	endFree_p = startAddress_p + size;
	if ((alloc_p->next_p == NULL) || (endFree_p < (char *) alloc_p->next_p)){
		adjPost_p = (MemFreeLink *) endFree_p;
	}else{
		adjPost_p = NULL;
	}

	/* remove alloc link */
	memRemoveAllocLink(alloc_p);

	/* merge if necessary and insert new free link */
	memMergeFree(startAddress_p, size, adjPrior_p, adjPost_p);
	ei();

	return (MEM_NO_ERROR);
}

/* frees up memory. simply calles myFreeErrorCode, but it won't pass the error status */
void myFree(void *address_p) {
	myFreeErrorCode(address_p, 0);
}

/* initialize utl global variables. ensures the double-word alignment
 * of the first possible address
 */
void memInit(void) {
	if (mem_p != NULL) {
		return;
	}

	mem_p = (char *) MEM_START;
	memLastAddress_p = (char *) MEM_END;

	memFreeListInit();
	memAllocListInit();

}

/* checks that the size given is a multiple of double-word size; if is not, then the 
 * size is rounded up to be mod double
 */
unsigned memModDouble(unsigned size) {
	unsigned mod = (size % DWORD);
	if (mod) {
		size = size + (DWORD - mod);
	}
	return size;
}

/* initialize the linked list representing free space; initially there will be a single
 * link present that will represent all 128M bytes of free space.
 */
void memFreeListInit(void) {
	memFreeHeader.first_p = (MemFreeLink *) mem_p;
	memFreeHeader.last_p = (MemFreeLink *) mem_p;

	/* set first free link */
	MemFreeLink *free_p;
	free_p = memFreeHeader.first_p;
	free_p->size = MEM_MAX_SIZE;
	free_p->prev_p = NULL;
	free_p->next_p = NULL;
}

/* alloc link structure will be stored in mod double-word bytes; the
 * requested size to be allocated will also be rounded up to mod
 * double-word to ensure that allocated space starts at mod double-word
 * address
 */
void memAllocListInit(void) {
	memAllocHeader.first_p = NULL;
	memAllocHeader.last_p = NULL;
	memAllocLinkSize = memModDouble(sizeof(MemAllocLink));
}

/* find location to insert alloc link into a list sorted by address */
MemAllocLink *memFindAllocPrev_p(MemAllocLink *alloc_p) {
	if ((alloc_p) == NULL
			|| (alloc_p
					>= (MemAllocLink *) (memLastAddress_p - memAllocLinkSize))) {
		return NULL;
	}

	MemAllocLink *prev_p;
	for (prev_p = memAllocHeader.first_p;
			(prev_p != NULL) && (prev_p < alloc_p); prev_p = prev_p->next_p) {
		;
	}

	if (prev_p == alloc_p) {
		/* address was already allocated */
		MEM_EXIT("memFindAllocPrev_p");
	}

	if (prev_p == NULL) {
		return memAllocHeader.last_p;
	} else {
		return prev_p->prev_p;
	}
}

/* find alloc link entry on the list */
bool memMatchAlloc(MemAllocLink *alloc_p) {
	if ((alloc_p == NULL)
			|| (alloc_p
					>= (MemAllocLink *) (memLastAddress_p - memAllocLinkSize))) {
		return false;
	}

	MemAllocLink *allocLink_p;

	for (allocLink_p = memAllocHeader.first_p;
			(allocLink_p != NULL) && (allocLink_p < alloc_p); allocLink_p =
					allocLink_p->next_p)
		;

	if (alloc_p != allocLink_p) {
		return false;
	} else {
		return true;
	}

}

/* implements first fit algorithm to find space in memory that is big
 * enough for the desired allocation; fast and efficient approach, given
 * two separate free/alloc linked list for keeping track of memory usage.
 */
MemFreeLink *memFindSufficientFree(unsigned size) {
	if ((size <= 0) || (size > MEM_MAX_SIZE)) {
		return (NULL);
	}

	MemFreeLink *free_p;

	for (free_p = memFreeHeader.first_p;
			(free_p != NULL) && (free_p->size < size); free_p =
					free_p->next_p) {
		;
	}
	return free_p;

}

/* find location for insertion of a 'free' link into a list sorted by
 * size, starting from a known position
 */
MemFreeLink *memFindFreePrev_p(MemFreeLink *free_p, MemFreeLink *start_p) {
	if ((free_p == NULL)
			|| (free_p >= (MemFreeLink*) (memLastAddress_p - memAllocLinkSize))) {
		return NULL;
	}

	MemFreeLink *prev_p;
	for (prev_p = start_p; (prev_p != NULL) && (prev_p->size < free_p->size);
			prev_p = prev_p->next_p) {
		;
	}

	if (prev_p == free_p) {
		/* address was already free - BUG */
		MEM_EXIT("memFindFreePrev_p");
	}

	if (prev_p == NULL) {
		return memFreeHeader.last_p;
	} else {
		return prev_p->prev_p;
	}
}

/* find free link entry on the list */
bool memMatchFree_p(MemFreeLink *free_p) {

	if ((free_p == NULL)
			|| (free_p >= (MemFreeLink*) (memLastAddress_p - memAllocLinkSize))) {
		return false;
	}

	MemFreeLink *freeLink_p;
	bool match = false;

	if (free_p == NULL) {
		return false;
	}

	for (freeLink_p = memFreeHeader.first_p; (freeLink_p != NULL) && !match;
			freeLink_p = freeLink_p->next_p) {
		if (freeLink_p == free_p) {
			match = true;
		}
	}

	return match;

}

/* insert allocated link */
MemAllocLink *memInsertAllocLink_p(char *address_p, unsigned size) {
	if ((address_p < mem_p)
			|| (address_p >= (memLastAddress_p - memAllocLinkSize))) {
		MEM_RETURN(MEM_INV_ADDRESS, NULL);
	}

	if ((size <= 0) || (size > MEM_MAX_SIZE)) {
		MEM_RETURN(MEM_INV_SIZE, NULL);
	}

	memErrno = MEM_NO_ERROR;

	/*  it is the first alloc link to be set */
	if ((mem_p == NULL) && (address_p == NULL)) {
		memInit();
		address_p = mem_p;
	} else if ((mem_p == NULL) || (address_p == NULL)) {
		MEM_RETURN(MEM_INV_ADDRESS, NULL);
	}

	MemAllocLink *alloc_p;
	alloc_p = (MemAllocLink *) address_p;
	alloc_p->size = size;

	/* find location to insert link in list of allocated spaces sorted by address  */
	MemAllocLink *prev_p = memFindAllocPrev_p(alloc_p);

	if (memAllocHeader.first_p == NULL) {
		/* this is the only reserved mem */
		alloc_p->prev_p = NULL;
		alloc_p->next_p = NULL;
		memAllocHeader.first_p = alloc_p;
		memAllocHeader.last_p = alloc_p;
	} else if (prev_p == NULL) {
		/* this is new first reserved mem */
		alloc_p->prev_p = NULL;
		alloc_p->next_p = memAllocHeader.first_p;
		memAllocHeader.first_p->prev_p = alloc_p;
		memAllocHeader.first_p = alloc_p;
	} else {
		alloc_p->prev_p = prev_p;
		alloc_p->next_p = prev_p->next_p;
		prev_p->next_p = alloc_p;

		if (alloc_p->next_p != NULL) {
			alloc_p->next_p->prev_p = alloc_p;
		} else {
			/* this is new last free mem */
			memAllocHeader.last_p = alloc_p;
		}
	}

	return (alloc_p);
}

/* remove allocated link */
void memRemoveAllocLink(MemAllocLink *alloc_p) {
	if (mem_p == NULL) {
		MEM_VRETURN(MEM_NOT_INITIALIZED);
	}

	if ((alloc_p == NULL) || (alloc_p < (MemAllocLink *) mem_p)
			|| (alloc_p
					>= (MemAllocLink *) (memLastAddress_p - memAllocLinkSize))) {
		MEM_VRETURN(MEM_INV_ADDRESS);
	}

	memErrno = MEM_NO_ERROR;

	if (alloc_p->prev_p != NULL) {
		alloc_p->prev_p->next_p = alloc_p->next_p;
	} else {
		/* the first alloc mem link on the list to be removed */
		memAllocHeader.first_p = alloc_p->next_p;
	}

	if (alloc_p->next_p != NULL) {
		alloc_p->next_p->prev_p = alloc_p->prev_p;
	} else {
		/* the last alloc mem link on the list to be removed */
		memAllocHeader.last_p = alloc_p->prev_p;
	}
}

/* insert a free link into a list sorted by size; searches for appropriate
 * location from a starting position
 */
MemFreeLink *memInsertFreeLink_p(char *address_p, unsigned size,
		MemFreeLink *freeFirst_p) {
	if ((address_p == NULL) || (address_p < mem_p)
			|| (address_p >= (memLastAddress_p - memAllocLinkSize)))
		MEM_RETURN(MEM_INV_ADDRESS, NULL);
	if ((size <= 0) || (size > MEM_MAX_SIZE))
		MEM_RETURN(MEM_INV_SIZE, NULL);

	memErrno = MEM_NO_ERROR;

	/* this is the first free link to be set */
	if (mem_p == NULL) {
		memInit();
		return (memFreeHeader.first_p);
	}

	MemFreeLink *free_p;

	free_p = (MemFreeLink *) address_p;
	free_p->size = size;

	/* insert new link */
	if (freeFirst_p == NULL) {
		freeFirst_p = memFreeHeader.first_p;
	}

	/* find prev alloc link in sorted by address list of allocated spaces starting from freeFirst_p */
	di();
	MemFreeLink *prev_p = memFindFreePrev_p(free_p, freeFirst_p);

	if (memFreeHeader.first_p == NULL) {
		/* this is the only free mem */
		free_p->prev_p = NULL;
		free_p->next_p = NULL;
		memFreeHeader.first_p = free_p;
		memFreeHeader.last_p = free_p;
	} else if (prev_p == NULL) {
		/* this is new first free  mem */
		free_p->prev_p = NULL;
		free_p->next_p = memFreeHeader.first_p;
		memFreeHeader.first_p->prev_p = free_p;
		memFreeHeader.first_p = free_p;
	} else {
		free_p->prev_p = prev_p;
		free_p->next_p = prev_p->next_p;
		prev_p->next_p = free_p;

		if (free_p->next_p != NULL) {
			free_p->next_p->prev_p = free_p;
		} else {
			/* this is new last free mem */
			memFreeHeader.last_p = free_p;
		}
	}
	ei();

	return (free_p);
}

/* remove free link */
void memRemoveFreeLink(MemFreeLink *free_p) {
	if (mem_p == NULL) {
		MEM_VRETURN(MEM_NOT_INITIALIZED);
	}

	if ((free_p == NULL) || (free_p < (MemFreeLink *) mem_p)
			|| (free_p >= (MemFreeLink *) (memLastAddress_p - memAllocLinkSize))) {
		MEM_VRETURN(MEM_INV_ADDRESS);
	}

	if (free_p->prev_p != NULL) {
		free_p->prev_p->next_p = free_p->next_p;
	} else {
		/* the first free mem link on the list to be removed */
		memFreeHeader.first_p = free_p->next_p;
	}

	if (free_p->next_p != NULL) {
		free_p->next_p->prev_p = free_p->prev_p;
	} else {
		/* the last free mem link on the list to be removed */
		memFreeHeader.last_p = free_p->prev_p;
	}

}

/* check for adjacent free links in memory. If any are found, merge them
 * into one large free link
 */
void memMergeFree(char *address_p, unsigned size, MemFreeLink *adjPrior_p,
		MemFreeLink *adjPost_p) {
	if ((address_p == NULL) || (address_p < mem_p)
			|| (address_p >= (memLastAddress_p - memAllocLinkSize))) {
		MEM_VRETURN(MEM_INV_ADDRESS);
	}

	MemFreeLink *freePrev_p = NULL;

	if (adjPrior_p != NULL) {
		address_p = (char *) adjPrior_p;
		size += adjPrior_p->size;
		freePrev_p = adjPrior_p->prev_p;

		/* remove adjacent link after merge */
		memRemoveFreeLink(adjPrior_p);
	}

	if ((adjPost_p != NULL)
			&& (adjPost_p != (MemFreeLink *) memLastAddress_p)) {
		size += adjPost_p->size;
		if ((freePrev_p == NULL) || (freePrev_p->size < adjPost_p->size)) {
			freePrev_p = adjPost_p->prev_p;
		}

		/* remove adjacent link after merge */
		memRemoveFreeLink(adjPost_p);
	}

	memInsertFreeLink_p(address_p, size, freePrev_p);
}

/* allocated address = header information + space allocated for the calling
 * process to use; the address returned to the calling process is offset
 * from the beginning of the allocated space * by the size of the 'allocated link'
 */
void memoryMap(void) {
	if (mem_p == NULL) {
		memInit();
	}

	char *address_p = mem_p;
	MemFreeLink *free_p;
	MemAllocLink *alloc_p = memAllocHeader.first_p;

	char *endAddress_p;
	SVCprintStr(
			"\r\n_____________________________________________________________\r\n");
	SVCprintStr(
			"  PID    |   free   |    addrress           |  size           \r\n");
	SVCprintStr(
			"_________|__________|_______________________|________________\r\n\n");
	while ((address_p != NULL) && (address_p < memLastAddress_p)) {
		char str[shMAX_BUFFERSIZE + 1];
		if (alloc_p != (MemAllocLink *) address_p) {
			/* this address is not allocated */
			free_p = (MemFreeLink *) address_p;
			endAddress_p = address_p + free_p->size;
			snprintf(str, shMAX_BUFFERSIZE,
					"         |   Y      |   %p       | %u\r\n", address_p,
					free_p->size);

			address_p = endAddress_p++;

		} else {
			unsigned size = alloc_p->size + memAllocLinkSize;
			endAddress_p = address_p + size;
			snprintf(str, shMAX_BUFFERSIZE,
					" %d       |   N      |   %p       | %u\r\n",
					alloc_p->processID, address_p, alloc_p->size);

			address_p = endAddress_p++;
			alloc_p = alloc_p->next_p;
		}
		SVCprintStr(str);
	}
}

/* assigns a value to a certain region of memory */
MemErrno myMemset(void *address_p, int val, unsigned nbytes) {
	return memSet(address_p, val, nbytes, true);
}
MemErrno memSet(void *address_p, int val, unsigned nbytes, bool checkPID) {
	if ((mem_p == NULL) || (address_p == NULL)
			|| (address_p < (void *) (mem_p + memAllocLinkSize))
			|| (address_p >= (void *) (memLastAddress_p - memAllocLinkSize))) {
		return (MEM_NOT_ALLOCATED);
	}

	if ((val > UCHAR_MAX) || ((val < 0) && (val < CHAR_MIN))) {
		return (MEM_INV_VAL);
	}

	char *startAddress_p = (char*) address_p - memAllocLinkSize;
	MemAllocLink *alloc_p = (MemAllocLink *) startAddress_p;

	if (!memMatchAlloc(alloc_p)) {
		/* was not allocated */
		return (MEM_NOT_ALLOCATED);
	}

	if (checkPID && (alloc_p->processID != pid())) {
		return (MEM_NOT_OWNED);
	}

	if (nbytes > alloc_p->size) {
		return (MEM_INV_SIZE);
	}

	/* set the value */
	for (; nbytes > 0; nbytes--, address_p++) {
		*((char *) address_p) = (char) val;
	}

	return (MEM_NO_ERROR);
}

/* check to see if a certain value is stored in a particular region of memory */
MemErrno myMemchk(void *address_p, int val, unsigned nbytes) {
	if ((mem_p == NULL) || (address_p == NULL)
			|| (address_p < (void *) (mem_p + memAllocLinkSize))
			|| (address_p >= (void *) (memLastAddress_p - memAllocLinkSize))) {
		return (MEM_NOT_ALLOCATED);
	}

	if ((val > UCHAR_MAX) || ((val < 0) && (val < CHAR_MIN))) {
		return (MEM_INV_VAL);
	}

	char *startAddress_p = (char*) address_p - memAllocLinkSize;
	MemAllocLink *alloc_p = (MemAllocLink *) startAddress_p;

	if (!memMatchAlloc(alloc_p)) {
		/* was not allocated */
		return (MEM_NOT_ALLOCATED);
	}

	if (alloc_p->processID != pid()) {
		return (MEM_NOT_OWNED);
	}

	if (nbytes > alloc_p->size) {
		return (MEM_INV_SIZE);
	}

	/* check to see if the value in question is stored */
	for (; nbytes > 0; nbytes--, address_p++) {
		if (*((char *) address_p) != (char) val) {
			return (MEM_INV_VAL);
		}
	}

	return (MEM_NO_ERROR);
}

/* copy into reserved memory a string of bytes;
 * will check if destination address is allocated by the calling process, and if
 * it is large enough for the source string to fit;
 * takes in:  memory address, address of a byte string, and number of bytes to be coppied
 * returns: error status
 */
MemErrno myMemcpy(void *dst_p, const void *src_p, unsigned nbytes) {
	/* check that destination address is not out of bounds of memory */
	if ((mem_p == NULL) || (dst_p == NULL)
			|| (dst_p < (void *) (mem_p + memAllocLinkSize))
			|| (dst_p >= (void *) (memLastAddress_p - memAllocLinkSize))){
		return (MEM_NOT_ALLOCATED);
	}

	/* if source address is NULL or numer of bytes is 0, no action is taken */
	if ((src_p == NULL) || (nbytes == 0)){
		return (MEM_NO_ERROR);
	}

	unsigned int memSize = memCheckAddress(dst_p);

	/* check if the requested number of bytes is not out of bounds */
	if (nbytes > memSize){
		return (MEM_NOT_ALLOCATED);
	}

	int i = 0;

	for (i = 0; i < nbytes; i++){
		*((char *) dst_p++) = *((char *) src_p++);
	}

	return (MEM_NO_ERROR);
}

/* check if address is within allocated memory segment; if so, returns the number of bytes
 * allocated starting from the given address
 */
unsigned int memCheckAddress(void *address_p) {
	/* check that destination address is not out of bounds of memory */
	if ((mem_p == NULL) || (address_p == NULL)
			|| (address_p < (void *) (mem_p + memAllocLinkSize))
			|| (address_p >= (void *) (memLastAddress_p - memAllocLinkSize))){
		return (0);
	}

	MemAllocLink *allocLink_p;
	bool found = false;
	unsigned int nbytesLeft = 0;
	void *startAddress_p;

	/* since alloc links are sorted by address, no need to search past link with greater starting address */
	for (allocLink_p = memAllocHeader.first_p;
			(allocLink_p != NULL)
					&& (address_p
							>= (startAddress_p =
									(void *) (((void *) allocLink_p)
											+ memAllocLinkSize))) && !found;
			allocLink_p = allocLink_p->next_p) {
		if (address_p < (startAddress_p + allocLink_p->size)) {
			found = true;
			if (allocLink_p->processID != pid()){
				nbytesLeft = 0;
			}else{
				nbytesLeft = (allocLink_p->size - (address_p - startAddress_p));
			}
		}
	}

	return nbytesLeft;
}

/* this function frees space that was allocated with the specified pid */
void memExit(pid_t pId) {
	if (mem_p == NULL){
		return;
	}

	MemAllocLink *alloc_p = memAllocHeader.first_p;

	if (pId == NO_PID){
		pId = pid();
	}

	di();
	while (alloc_p != NULL) {
		MemAllocLink *next_p = alloc_p->next_p;
		if (alloc_p->processID == pId) {
			myFreeErrorCode((void *) (((void *) alloc_p) + memAllocLinkSize),
					pId);
		}
		alloc_p = next_p;
	}
	ei();
}
