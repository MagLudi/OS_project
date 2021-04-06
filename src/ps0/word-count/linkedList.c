/* linkedList.c - contains linked list utilities declared in linkedlist.h:	*/

/*      llCreateLinkdList - creates the list					*/
/*      llInsertLink_p - inserts link any where on the list			*/
/*      llAppendLink_p - inserts link at the end of the list			*/
/*      llRemoveLink_p - removes link from the list				*/

#define _POSIX_SOURCE 1

/* systtem headers                                      */
#include <stdlib.h> 
#include <stdio.h>
#include <errno.h>
#include <float.h>
#include <string.h> 
#include <stdbool.h>

/* project headers                                      */
#define ALLOCATE_
#include "linkedList.h"

/*                                                                      */
/* linked list functions                                                */
/*                                                                      */

/* create linked list. Takes in a pointer of type llLnkdListHdr_t to the linked list.
 * Returns the status regarding success or failure of the creation
 */
llStatus_t llCreateLnkdList(llLnkdListHdr_t *list_p) {
	llStatus_t sts = llSUCCESS;

	if (list_p == NULL) {
		llStatusRETURN(llFAIL, "\nllCreateLnkdList:: invalid list_p argument");
	}

	list_p->firstNode_p = NULL;
	list_p->lastNode_p = NULL;
	return (sts);
}

/* append link. Takes in a pointer of type character (the word), and integer
 * count, and a pointer to the list. Adds the new link to the end of the list,
 * and returns a pointer of the new link.
 */
llLnkdListNode_t *llAppendLink_p(char *word_p, int count,
		llLnkdListHdr_t *list_p) {
	llLnkdListNode_t *node_p = NULL;

	if (list_p == NULL) {
		llStatusRETURN(llFAIL, "\nllAppendLink_p:: invalid list_p argument");
	}

	node_p = (llLnkdListNode_t *) malloc(sizeof(llLnkdListNode_t));
	llNullRETURN(node_p, "llAppendLink :: failed to allocate memory");

	node_p->count = count;
	node_p->word_p = word_p;
	node_p->next_p = NULL;
	node_p->prev_p = list_p->lastNode_p;

	if (list_p->lastNode_p != NULL) {
		list_p->lastNode_p->next_p = node_p;
	} else {
		(list_p->firstNode_p = node_p);
	}

	list_p->lastNode_p = node_p;

	return (node_p);
}

/* remove link. Takes in the pointer to the link that will be removed and a pointer to the list.
 * Returns the pointer to the link that was originaly following the removed link.
 */
llLnkdListNode_t *llRemoveLink_p(llLnkdListNode_t *node_p,
		llLnkdListHdr_t *list_p) {
	llLnkdListNode_t *nextNode_p;
	if ((node_p == NULL) || (list_p == NULL)) {
		llNullRETURN(NULL, "\nllRemoveLind:: invalid arguments");
	}

	if (node_p->prev_p != NULL) {
		node_p->prev_p->next_p = node_p->next_p;
	} else {
		(list_p->firstNode_p = node_p->next_p);
	}

	if (node_p->next_p != NULL) {
		node_p->next_p->prev_p = node_p->prev_p;
	} else {
		list_p->lastNode_p = node_p->prev_p;
	}

	nextNode_p = node_p->next_p;
	free(node_p);

	return (nextNode_p);
}

/* insert link. Adds a new link in a particular location in the list. Takes in a
 * char pointer (the word), an integer counter, a pointer to the link it will follow,
 * and a pointer to the list. Returns a pointer of the newly created link.
 */
llLnkdListNode_t *llInsertLink_p(char *word_p, int count,
		llLnkdListNode_t *prevNode_p, llLnkdListHdr_t *list_p) {
	llLnkdListNode_t *node_p = NULL;

	if (list_p == NULL) {
		llNullRETURN(NULL, "\nllInsertLink:: invalid arguments");
	}

	node_p = (llLnkdListNode_t *) malloc(sizeof(llLnkdListNode_t));
	llNullRETURN(node_p, "llAppendLink :: failed to allocate memory");

	if (prevNode_p == NULL) {
		/* this is new first node */
		node_p->count = count;
		node_p->word_p = word_p;
		node_p->prev_p = NULL;
		node_p->next_p = list_p->firstNode_p;
		list_p->firstNode_p->prev_p = node_p;
		list_p->firstNode_p = node_p;
	} else {
		node_p->count = count;
		node_p->word_p = word_p;
		node_p->prev_p = prevNode_p;
		node_p->next_p = prevNode_p->next_p;

		prevNode_p->next_p = node_p;

		if (node_p->next_p != NULL) {
			node_p->next_p->prev_p = node_p;
		} else {
			list_p->lastNode_p = node_p;
		}
	}

	return (node_p);
}


