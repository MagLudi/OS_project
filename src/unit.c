#define _POSIX_SOURCE 1

/* systtem headers                                      */
#include <stdlib.h> 
#include <stdio.h>
#include <errno.h>
#include <float.h>
#include <stdint.h>
#include <assert.h>

/* project headers                                      */
#define ALLOCATE_
#include "utl.h"
#include "mem.h"
#include "cmd.h"
#include "minunit.h" /* Mark Fords example for unit testing */

/* global variables */


/* checking at the compile */
/* if address size for this machine is 32 bit the utlAddress_t type in utl.h has to be changed */
_Static_assert(sizeof(void *) == sizeof(uint64_t), "expected 64 bit architecture; update configuration");


/* test function declarations */
static char * allTests();
static char * testNumberStringConversion();
static char * testMem();
static char * testCmdMem();
static char * testCmdSet();

int i = 0;


int main () /* adapted from Mark Ford's example */
{
    assertionsRun=0;
    testsRun=0;


    char *errMessage_p = allTests();
    

    printf("\nAssertions run: %d\n", assertionsRun);
    if (errMessage_p != 0) {
        printf("**** TEST FAILURE ****\n");
        printf("%s\n", errMessage_p);
    }
    else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", testsRun);

    return errMessage_p != 0;
}

/* test function definitions */
static char * allTests()
{

    MU_RUN_TEST(testNumberStringConversion);
    MU_RUN_TEST(testMem);

    MU_RUN_TEST(testCmdMem);
    MU_RUN_TEST(testCmdSet); 

    return 0;
}

static char * testNumberStringConversion()
{
  UtlAddress_t num = 0;
  char * str_p = NULL;
  char * num_p = calloc(25, sizeof(char));
  
  /*1*/
  str_p = "9876543210";
  sscanf(str_p, "%llu",(unsigned long long *) &num);
  MU_ASSERT("unsigned decimal string conversion", utlAtoD(str_p) == num);

  sprintf(num_p,"Ox%llx",(unsigned long long)num);
  MU_ASSERT("hex string conversion", utlAtoD(num_p) == num);

  sprintf(num_p,"O%llo",(unsigned long long)num);
  MU_ASSERT("octal string conversion", utlAtoD(num_p) == num);

  /*2*/
  str_p = "99999999999999999999999999999999999999999999";
  sscanf(str_p, "%llu",(unsigned long long *) &num);  /* sscanf is checking for overflow/underflow*/
  MU_ASSERT("decimal string conversion overflow", utlAtoD(str_p) == num);

  sprintf(num_p,"Ox%llx",(unsigned long long) num);
  MU_ASSERT("hex string conversion overflow", utlAtoD(num_p) == num);

  sprintf(num_p,"O%llo",(unsigned long long)num);
  MU_ASSERT("octal string conversion overflow", utlAtoD(num_p) == num);

  /*3*/
  str_p = "-9999999999999999999999";
  sscanf(str_p, "%lld",(long long *) &num);  /* sscanf is  checking for overflow/underflow */
  MU_ASSERT("signed decimal string conversion underflow", utlAtoD(str_p) == num);
  
  /*4*/
  str_p = "23456678192745";
  sscanf(str_p, "%llu",(unsigned long long *) &num);
  MU_ASSERT("unsigned decimal string conversion", utlAtoD(str_p) == num);

  sprintf(num_p,"Ox%llx",(unsigned long long)num);
  MU_ASSERT("hex string conversion", utlAtoD(num_p) == num);

  sprintf(num_p,"O%llo",(unsigned long long)num);
  MU_ASSERT("octal string conversion", utlAtoD(num_p) == num);

  /*5*/
  str_p = "0987654321";
  sscanf(str_p, "%llu",(unsigned long long *) &num);
  MU_ASSERT("unsigned decimal string conversion", utlAtoD(str_p) == num);

  sprintf(num_p,"Ox%llx",(unsigned long long)num);
  MU_ASSERT("hex string conversion", utlAtoD(num_p) == num);

  sprintf(num_p,"O%llo",(unsigned long long)num);
  MU_ASSERT("octal string conversion", utlAtoD(num_p) == num);


  /*6*/
  str_p = "987-654321";
  sscanf(str_p, "%llu",(unsigned long long *) &num);
  MU_ASSERT("unsigned decimal string conversion", utlAtoD(str_p) == num);

  sprintf(num_p,"Ox%llx",(unsigned long long)num);
  MU_ASSERT("hex string conversion", utlAtoD(num_p) == num);

  sprintf(num_p,"O%llo",(unsigned long long)num);
  MU_ASSERT("octal string conversion", utlAtoD(num_p) == num);
  
  return NULL;
}
static char * testMem()
{
  /*1*/
  /* test malloc for invalid size request */

  MU_ASSERT("malloc max", myMalloc(MEM_MAX_SIZE) == NULL);
  MU_ASSERT("malloc -50", myMalloc(-50) == NULL);


  
  /*2*/
  char *addressMAX_p = myMalloc(MEM_MAX_SIZE-24);  /* 24 = sizeof(MemAllocLink) runded up to be mod 8 */
  MU_ASSERT("malloc max possible", addressMAX_p != NULL);
  MU_ASSERT("mod 8 check",((long long) addressMAX_p)%8 == 0);

  /*3*/
  MU_ASSERT("free allocated", myFreeErrorCode(addressMAX_p) == MEM_NO_ERROR);

  /*4*/
  void *address90_p = myMalloc(90);
  MU_ASSERT("malloc 90", address90_p != NULL);
  MU_ASSERT("mod 8 check",((long long) address90_p)%8 == 0);
  
  /*5*/
  void *address500000_p = myMalloc(500000);
  MU_ASSERT("malloc 500000", address500000_p != NULL);
  MU_ASSERT("mod 8 check",((long long) address500000_p)%8 == 0);
  
  /*6*/
  void *address8_p = myMalloc(8);
  MU_ASSERT("malloc 8", address8_p != NULL);
  MU_ASSERT("mod 8 check",((long long) address8_p)%8 == 0);
  
  /*7*/
  void *address2001_p = myMalloc(2001);
  MU_ASSERT("malloc 2001", address2001_p != NULL);
  MU_ASSERT("mod 8 check",((long long) address2001_p)%8 == 0);

  /*8*/
  MU_ASSERT("free 8", myFreeErrorCode(address8_p) == MEM_NO_ERROR);

  /*9*/
  void *address50_p = myMalloc(50);
  MU_ASSERT("malloc 50", address50_p != NULL);
  MU_ASSERT("mod 8 check",((long long) address50_p)%8 == 0);

  /*10*/
  MU_ASSERT("free 8", myFreeErrorCode(address8_p) == MEM_NOT_ALLOCATED);

  /* allocating max possible (max - (mod 8 sizeof(MemAllocLink))) should fail untill all prev allocated memory is free */
  /*11*/
  addressMAX_p = myMalloc(MEM_MAX_SIZE-24);
  MU_ASSERT("malloc max possible", addressMAX_p == NULL);

  /*12*/
  MU_ASSERT("free 50", myFreeErrorCode(address50_p) == MEM_NO_ERROR);
  MU_ASSERT("free 500000", myFreeErrorCode(address500000_p) == MEM_NO_ERROR); 
  MU_ASSERT("free 2001", myFreeErrorCode(address2001_p) == MEM_NO_ERROR); 
  MU_ASSERT("free 90", myFreeErrorCode(address90_p) == MEM_NO_ERROR);
  addressMAX_p = myMalloc(MEM_MAX_SIZE-24);
  MU_ASSERT("malloc max possible", addressMAX_p != NULL);
  MU_ASSERT("mod 8 check",((long long) addressMAX_p)%8 == 0);
  MU_ASSERT("free max", myFreeErrorCode(addressMAX_p) == MEM_NO_ERROR);

  /* memset and memchk test */
  /*13*/
  MU_ASSERT("memset on unallocated address", myMemset(address50_p, 2, 50) == MEM_NOT_ALLOCATED);

  /*14*/
  MU_ASSERT("malloc ", (address50_p = myMalloc(50)) != NULL);
  MU_ASSERT("malloc ", (address8_p = myMalloc(8)) != NULL);
  MU_ASSERT("memset ", myMemset(address8_p, 257, 8) == MEM_INV_VAL) ;
  MU_ASSERT("memset ", myMemset(address8_p, 2, 8) == MEM_NO_ERROR) ;
  MU_ASSERT("memchk ", myMemchk(address8_p, 2, 8) == MEM_NO_ERROR) ;  
  MU_ASSERT("memset ", myMemset(address50_p, 3, 60) == MEM_INV_SIZE) ;
  MU_ASSERT("memchk ", myMemchk(address8_p, 2, 8) == MEM_NO_ERROR) ;
  MU_ASSERT("memset ", myMemset(address50_p, 4, 30) == MEM_NO_ERROR) ;
  MU_ASSERT("memchk ", myMemchk(address50_p, 4, 30) == MEM_NO_ERROR) ;
  MU_ASSERT("memchk ", myMemchk(address50_p, 4, 31) == MEM_INV_VAL) ;
  MU_ASSERT("memchk ", myMemchk(address8_p, 5, 8) == MEM_INV_VAL) ;  
  MU_ASSERT("free ", myFreeErrorCode(address8_p) == MEM_NO_ERROR);
  MU_ASSERT("memchk ", myMemchk(address8_p, 2, 8) == MEM_NOT_ALLOCATED) ; 
  MU_ASSERT("free 50", myFreeErrorCode(address50_p) == MEM_NO_ERROR);
  
  return NULL;
}

static char * testCmdMem()
{
  /* testing for number of arguments validation  */
  /*1*/
  char *argv[2] = {"malloc","100"};
  MU_ASSERT("cmd_malloc argc", cmd_malloc (1, argv) == utlArgNumERROR);
  MU_ASSERT("cmd_malloc argc", cmd_malloc (3, argv) == utlArgNumERROR);
  
  argv[0] = "free";  argv[1] = "Oxf100";
  MU_ASSERT("cmd_free argc", cmd_free (1, argv) == utlArgNumERROR);
  MU_ASSERT("cmd_free argc", cmd_free (3, argv) == utlArgNumERROR);

  argv[0] = "memset"; 
  MU_ASSERT("cmd_set argc", cmd_memset (5, argv) == utlArgNumERROR);
  argv[0] = "memchk"; 
  MU_ASSERT("cmd_memchk argc", cmd_memchk (3, argv) == utlArgNumERROR);
  MU_ASSERT("cmd_memchk argc", cmd_memchk (5, argv) == utlArgNumERROR);

  /* testing for invalid argv */
  /*2*/
  
  argv[0] = "malloc";   argv[1] = NULL;
  MU_ASSERT("cmd_malloc argv", cmd_malloc (2, argv) == utlArgValERROR);
  argv[1] = "Ot100";
  MU_ASSERT("cmd_malloc argv", cmd_malloc (2, argv) == utlArgValERROR);
  argv[1] = "O10095";
  MU_ASSERT("cmd_malloc argv", cmd_malloc (2, argv) == utlArgValERROR);
  argv[1] = "OxF9X100";
  MU_ASSERT("cmd_malloc argv", cmd_malloc (2, argv) == utlArgValERROR);
  argv[1] = "123f0";
  MU_ASSERT("cmd_malloc argv", cmd_malloc (2, argv) == utlArgValERROR);
  
  
  /* testing allocation/deallocation */
  /*3*/
  
  /* request to free nonexistend address */
  
  argv[0] = "free";  argv[1] = "OxF100";
  MU_ASSERT("cmd_free", cmd_free (2, argv) == utlFailERROR);
  
  argv[0] = "malloc";  argv[1] = "50";
  MU_ASSERT("cmd_malloc argv decimal", cmd_malloc (2, argv) == utlNoERROR); 

  argv[1] = "O7865";
  MU_ASSERT("cmd_malloc argv octal", cmd_malloc (2, argv) == utlNoERROR);
  argv[1] = "OxaAFb50";
  MU_ASSERT("cmd_malloc argv hex", cmd_malloc (2, argv) == utlNoERROR);
  

  return NULL;
}

static char * testCmdSet()
{
  /* testing set/unset commands */
  /*1*/
  char *argv[2];
  argv[1] = calloc(80,sizeof(char));
  strcpy(argv[1],"aaa=bbb");
  MU_ASSERT("cmd_set aaa=bbb", cmd_set (2, argv) == utlNoERROR);

  strcpy(argv[1],"$aaa=ccc");
  MU_ASSERT("cmd_set $aaa=ccc", cmd_set (2, argv) == utlNoERROR);

  strcpy(argv[1],"bbb");
  MU_ASSERT("cmd_unset bbb", cmd_unset (2, argv) == utlNoERROR);

  strcpy(argv[1],"ccc=aaa");
  MU_ASSERT("cmd_set ccc=aaa", cmd_set (2, argv) == utlNoERROR);

  strcpy(argv[1],"$ccc");
  MU_ASSERT("cmd_unset $ccc", cmd_unset (2, argv) == utlNoERROR);
  
  return NULL;
}
