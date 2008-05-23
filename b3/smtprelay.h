#include <stdio.h>
#include <pthread.h>

#ifdef USE_DMALLOC
  #include <dmalloc.h>
#endif

#define OK 1
#define FAIL 0


#define ERROR_PREF "[1;31mERROR: [0m"


#ifdef IS_DEBUG
  #define DEBUG_PREF 		"[1;32mDEBUG: [0m"
  #define DEBUG_THREAD_PREF 	"[1;34mTHREAD %lu: [0m"
  #define DEBUG(x) 		fprintf(stderr, DEBUG_PREF "%s\n", x)
  #define DEBUG_N(x,y) 		fprintf(stderr, DEBUG_PREF "%s=%d\n", x, y)
  #define DEBUG_CLNT(x) 	fprintf(stderr, DEBUG_PREF DEBUG_THREAD_PREF "%s\n",pthread_self(),x)
  #define DEBUG_CLNT_N(x,y) 	fprintf(stderr, DEBUG_PREF DEBUG_THREAD_PREF "%s=%d\n",pthread_self(),x,y)
  #define DEBUG_CLNT_S(x,y) 	fprintf(stderr, DEBUG_PREF DEBUG_THREAD_PREF "%s: %s\n",pthread_self(),x,y)
  #define DEBUG_CLNT_S_S(x,y,z)	fprintf(stderr, DEBUG_PREF DEBUG_THREAD_PREF "%s %s: %s\n",pthread_self(),x,y,z)
#else
  #define DEBUG
  #define DEBUG_N(x,y) 
  #define DEBUG_CLNT(x) 	
  #define DEBUG_CLNT_S(x,x) 	
  #define DEBUG_CLNT_S_S(x,x,z)	
#endif

#define DEFAULT_BINDADDR "127.0.0.1"
#define DEFAULT_BINDPORT "2345"

#define DEFAULT_REMOTEADDR "141.30.228.39"
#define DEFAULT_REMOTEPORT "25"

#define SEND_MAXTRY 3

extern char *server_addr;

// Puts an Error with the Msg related to the errno
// at stderr
void put_err(char *src);
