/* linkedList.h - associated header file. Contains the data structures for linked list,
 * the necesary declarations, and the global variables used in word-count.c
 */
#ifndef GLB_
#define GLB_


/* systtem headers                                      */
#include <stdlib.h>
#include <stdio.h>


/*                                                      */
/* type definitions                                     */
/*                                                      */

typedef enum
{
  llFAIL = 0,
  llSUCCESS ,
  llERROR
} llStatus_t ;

/* linked list defs                                     */

typedef struct linkedListNode
{
  char                    *word_p  ;
  int                      count    ;
  struct linkedListNode   *prev_p  ;
  struct linkedListNode   *next_p  ;
} llLnkdListNode_t ;

typedef struct
{
  llLnkdListNode_t *firstNode_p ;
  llLnkdListNode_t *lastNode_p  ;
} llLnkdListHdr_t ;



/*                                                      */
/*  constants                                           */
/*                                                      */





/*                                                      */
/*  global variable declarations                        */
/*                                                      */

#ifndef ALLOCATE_
 #define EXTERN_ extern
#else
 #define EXTERN_ 
#endif  /* ALLOCATE_ */

EXTERN_        char             *llProgramName_p         ;
EXTERN_        llStatus_t       llStatus                ;

EXTERN_        FILE              *llInputFile_p       ;
EXTERN_        char              *llInputFileName_p   ;

EXTERN_        llLnkdListHdr_t   llList              ;


/*                                                      */
/*  function        declarations                        */
/*                                                      */

llStatus_t llInit   (int argc, char **argv_p);
llStatus_t llCleanup (void);

llStatus_t        llCreateLnkdList(llLnkdListHdr_t *list_p) ;
llLnkdListNode_t *llAppendLink_p  (char *word_p, int count,  llLnkdListHdr_t *list_p) ;
llLnkdListNode_t *llRemoveLink_p  (llLnkdListNode_t *node_p, llLnkdListHdr_t *list_p) ;
llLnkdListNode_t *llInsertLink_p (char *word_p, int count, llLnkdListNode_t *prevNode_p, llLnkdListHdr_t *list_p) ;

/*                                                      */
/* macros                                               */
/*                                                      */
#define llStatusEXIT(A,B)     if (A != llSUCCESS) {printf(B); exit(0);}
#define llStatusRETURN(A,B)   if (A != llSUCCESS) {printf(B); return(A);}
#define llNullRETURN(A,B)     if (A == NULL) {printf(B); return(A);} 


#endif /* GLB_ */
