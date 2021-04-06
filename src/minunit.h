#ifndef MIN_UNIT_
#define MIN_UNIT_

/* file: minunit.h      */
/* adapted from Mark Ford's example */
/* used for unit testing */

#define MU_ASSERT(message, test) do { assertionsRun++; if (!(test)) return message; } while (0)
#define MU_RUN_TEST(test)        do { char *message = test(); testsRun++; \
                                      if (message) return message; } while (0)

#ifndef ALLOCATE_
 #define EXTERN_ extern
#else
 #define EXTERN_ 
#endif  /* ALLOCATE_ */

EXTERN_       int testsRun;
EXTERN_     int assertionsRun;


#endif /*MIN_UNIT_*/
