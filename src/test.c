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

/* global variables */

/* checking at the compile */
/* if address size for this machine is 32 bit the utlAddress_t type in utl.h has to be changed 
   _Static_assert(sizeof(void *) == sizeof(uint64_t), "expected 64 bit architecture; update configuration"); */

int main ()
{
    

  printf("initial memory map\n");
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");

  printf("\nALLOCATE max size: expected to fail\n");
  void *addressMAX_p = NULL;
  addressMAX_p = myMalloc(MEM_MAX_SIZE);
  if (addressMAX_p == NULL) fprintf(stdout, "address not allocated\n");
  else fprintf(stdout,"allocated address %p\n", addressMAX_p);
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");
  
  printf("\nALLOCATE max - overhead: expected to succeed\n");
  addressMAX_p = NULL;
  addressMAX_p = myMalloc(MEM_MAX_SIZE-24);
  if (addressMAX_p == NULL) fprintf(stdout, " address not allocated\n");
  else fprintf(stdout,"allocated address %p\n", addressMAX_p);
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");
  
  printf("\nFREE %p: expected to succeed \n", addressMAX_p);
  if (myFreeErrorCode(addressMAX_p) != MEM_NO_ERROR) fprintf(stdout, " address not allocated\n");
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");
  
  printf("\nALLOCATE size 90: expected to succeed\n");
  void *address90_p = NULL;
  address90_p = myMalloc(90);
  if (address90_p == NULL) fprintf(stdout, " address not allocated\n");
  else fprintf(stdout," allocated address %p\n", address90_p);
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");
  
  printf("ALLOCATE size 50001: expected to succeed\n");
  void *address50001_p = NULL;
  address50001_p = myMalloc(50001);
  if (address50001_p == NULL) fprintf(stdout, " address not allocated\n");
  else fprintf(stdout," allocated address %p\n", address50001_p);
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");
  
  printf("\nALLOCATE size 80: expected to succeed\n");
  void *address80_p = NULL;
  address80_p = myMalloc(80);
  if (address80_p == NULL) fprintf(stdout, " address not allocated\n");
  else fprintf(stdout," allocated address %p\n", address80_p);
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");
  
  printf("\nALLOCATE size 8: expected to succeed\n");
  void *address8_p = NULL;
  address8_p = myMalloc(8);
  if (address8_p == NULL) fprintf(stdout, " address not allocated\n");
  else fprintf(stdout,"ALLOCATE address %p\n", address8_p);
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");
  
  printf("FREE %p: expected to succeed \n", address80_p);
  if (myFreeErrorCode(address80_p) != MEM_NO_ERROR) fprintf(stdout, " address not allocated\n");
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");
  
  printf("\nALLOCATE size 1000: expected to succeed\n");
  void *address1000_p = NULL;
  address1000_p = myMalloc(1000);
  if (address1000_p == NULL) fprintf(stdout, " address not allocated\n");
  else fprintf(stdout," allocated address %p\n", address1000_p);
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");
  
  printf("\nFREE %p: expected to fail\n", address80_p);
  if (myFreeErrorCode(address80_p) != MEM_NO_ERROR) fprintf(stdout, " address not allocated\n");
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");
  
  printf("\nFREE %p: expected to succeed \n", address90_p);
  if (myFreeErrorCode(address90_p) != MEM_NO_ERROR) fprintf(stdout, " address not allocated\n");
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");
  
  
  printf("\nFREE %p: expected to succeed \n", address50001_p);
  if (myFreeErrorCode(address50001_p) != MEM_NO_ERROR) fprintf(stdout, " address not allocated\n");
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");


  printf("\nALLOCATE size 20000: expected to succeed\n");
  void *address20000_p = NULL;
  address20000_p = myMalloc(20000);
  if (address20000_p == NULL) fprintf(stdout, " address not allocated\n");
  else fprintf(stdout," allocated address %p\n", address20000_p);
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");


  printf("\nFREE %p: expected to succeed \n", address8_p);
  if (myFreeErrorCode(address8_p) != MEM_NO_ERROR) fprintf(stdout, " address not allocated\n");
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");

  printf("\nALLOCATE size 16: expected to succeed\n");
  void *address16_p = NULL;
  address16_p = myMalloc(16);
  if (address1000_p == NULL) fprintf(stdout, " address not allocated\n");
  else fprintf(stdout," allocated address %p\n", address16_p);
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");
  
  printf("\nALLOCATE size 8: expected to succeed\n");
  address8_p = myMalloc(8);
  if (address8_p == NULL) fprintf(stdout, " address not allocated\n");
  else fprintf(stdout,"ALLOCATE address %p\n", address8_p);
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");


  printf("\nALLOCATE max - overhead: expected to fail\n");
  addressMAX_p = NULL;
  addressMAX_p = myMalloc(MEM_MAX_SIZE-24);
  if (addressMAX_p == NULL) fprintf(stdout, " address not allocated\n");
  else fprintf(stdout,"allocated address %p\n", addressMAX_p);
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");

    printf("\nALLOCATE size 1001: expected to succeed\n");
  void *address1001_p = myMalloc(1001);
  if (address1001_p == NULL) fprintf(stdout, " address not allocated\n");
  else fprintf(stdout,"ALLOCATE address %p\n", address1001_p);
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");

  printf("\nFREE %p: expected to succeed \n", address8_p);
  if (myFreeErrorCode(address8_p) != MEM_NO_ERROR) fprintf(stdout, " address not allocated\n");
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");

    printf("\nFREE %p: expected to succeed \n", address1001_p);
  if (myFreeErrorCode(address1001_p) != MEM_NO_ERROR) fprintf(stdout, " address not allocated\n");
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");

    printf("\nFREE %p: expected to succeed \n", address16_p);
  if (myFreeErrorCode(address16_p) != MEM_NO_ERROR) fprintf(stdout, " address not allocated\n");
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");

    printf("\nFREE %p: expected to succeed \n", address20000_p);
  if (myFreeErrorCode(address20000_p) != MEM_NO_ERROR) fprintf(stdout, " address not allocated\n");
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");

    printf("\nFREE %p: expected to succeed \n", address1000_p);
  if (myFreeErrorCode(address1000_p) != MEM_NO_ERROR) fprintf(stdout, " address not allocated\n");
  memoryMap();
  printf("______________________________________________________________\n");
  printf("\n______________________________________________________________\n");

  exit (0);
}
