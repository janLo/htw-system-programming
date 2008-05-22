#include <stdio.h>
#include <pthread.h>

#define ERROR_PREF "[1;31mERROR: [0m"


#ifdef IS_DEBUG
  #define DEBUG_PREF "[1;32mDEBUG: [0m"
  #define DEBUG_THREAD_PREF "[1;34mTHREAD %lu: [0m"
  #define DEBUG(x) fprintf(stderr, DEBUG_PREF "%s\n", x)
  #define DEBUG_N(x,y) fprintf(stderr, DEBUG_PREF "%s=%d\n", x, y)
  #define DEBUG_CLNT(x) fprintf(stderr, DEBUG_PREF DEBUG_THREAD_PREF "%s\n",pthread_self(),x)
#else
  #define DEBUG
  #define DEBUG_N(x,y) 
#endif

#define DEFAULT_BINDADDR "127.0.0.1"
#define DEFAULT_BINDPORT "2345"


// Puts an Error with the Msg related to the errno
// at stderr
void put_err(char *src);
