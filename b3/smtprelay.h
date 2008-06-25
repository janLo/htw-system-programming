#include <stdio.h>
#include <pthread.h>

#ifdef USE_DMALLOC
  #include <dmalloc.h>
#endif

#define OK 1
#define FAIL 0


#define ERROR_PREF   		"[1;31mERROR: [0m"
#define ERROR_SWITCH_TEST(x)	if (!app->quiet) x
#define ERROR_SYS(source)	ERROR_SWITCH_TEST( put_err(source))
#define ERROR_CUSTM(error)	ERROR_SWITCH_TEST( put_err_str(error))

#ifdef IS_DEBUG
  #define DEBUG_PREF 		"[1;32mDEBUG: [0m"
  #define DEBUG_THREAD_PREF 	"[1;34mTHREAD %lu: [0m"
  #define DEBUG_SWITCH_TEST(x)  if (app->debug) x
  #define DEBUG(x) 		DEBUG_SWITCH_TEST( fprintf(stderr, DEBUG_PREF "%s\n", x));
  #define DEBUG_N(x,y) 		DEBUG_SWITCH_TEST( fprintf(stderr, DEBUG_PREF "%s=%d\n", x, y));
  #define DEBUG_S(x,y) 		DEBUG_SWITCH_TEST( fprintf(stderr, DEBUG_PREF "%s:%s\n", x, y));
  #define DEBUG_CLNT(x) 	DEBUG_SWITCH_TEST( fprintf(stderr, DEBUG_PREF DEBUG_THREAD_PREF "%s\n",pthread_self(),x));
  #define DEBUG_CLNT_N(x,y) 	DEBUG_SWITCH_TEST( fprintf(stderr, DEBUG_PREF DEBUG_THREAD_PREF "%s=%d\n",pthread_self(),x,y));
  #define DEBUG_CLNT_S(x,y) 	DEBUG_SWITCH_TEST( fprintf(stderr, DEBUG_PREF DEBUG_THREAD_PREF "%s: %s\n",pthread_self(),x,y));
  #define DEBUG_CLNT_S_S(x,y,z)	DEBUG_SWITCH_TEST( fprintf(stderr, DEBUG_PREF DEBUG_THREAD_PREF "%s %s: %s\n",pthread_self(),x,y,z));
#else
  #define DEBUG(x)
  #define DEBUG_N(x,y) 
  #define DEBUG_S(x,y) 
  #define DEBUG_CLNT(x) 	
  #define DEBUG_CLNT_N(x,y) 	
  #define DEBUG_CLNT_S(x,y) 	
  #define DEBUG_CLNT_S_S(x,y,z)	
#endif

#define ERR_OPT_SERV_ADD 	"Bind Address not valid!"
#define ERR_OPT_SERV_PORT 	"Bind Port not valid"
#define ERR_OPT_REMOTE_ADD 	"Remote Address not valid"
#define ERR_OPT_REMOTE_PORT 	"Remote Port not valid"

#define DEFAULT_BIND_ADDR "0.0.0.0"
#define DEFAULT_BIND_PORT "2345"

#define DEFAULT_REMOTE_ADDR "141.30.228.39"
#define DEFAULT_REMOTE_PORT "25"

#define SEND_MAXTRY 3

typedef struct app_settings {
  char *server_addr;
  char *server_port;
  char *remote_addr;
  char *remote_port;
  char quiet;
  char debug;
  char smtp;
} app_settings_t;

extern app_settings_t *app;

// Puts an Error with the Msg related to the errno
// at stderr
void put_err(char *src);
void put_err_str(char *err);
