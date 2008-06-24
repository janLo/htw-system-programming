#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include "smtprelay.h"
#include "connection.h"

// Main Config var
app_settings_t *app = NULL;

// Puts a error to stderr
void put_err(char *src){
  if (src == NULL){
    fprintf(stderr, ERROR_PREF);
  } else {
    fprintf(stderr, ERROR_PREF "%s - ", src);
  }
  perror("");
}

// also puts a error to stderr
void put_err_str(char *err){
  if(err != NULL){
    fprintf(stderr, ERROR_PREF "%s \n", err);
  } else {
    fprintf(stderr, ERROR_PREF " Unspecified\n");
  }
}

// checks a cmd port option
char *port_opt(const char *buff, char *err){
  char *port = malloc(sizeof(char)*6);
  int is_digit = 1, i;
  char *tmp = malloc(sizeof(char)*(strlen(buff)+2));
  strcpy(tmp, buff);
  tmp[5] = '\0';

  for(i = 0; i < 5; i++){
    if(!isdigit(tmp[i])){
      is_digit = 0;
      break;
    }
  }

  if(is_digit || atoi(tmp) > 65535 || atoi(buff) < 1){
    ERROR_CUSTM(err);
    free(port);
    free(tmp);
    return NULL;
  }
  strcpy(port, tmp);
  free(tmp);
  return port;
}

// my own 'make everything to ip-string' function.
// checks a cmd host option
char *host_opt(const char *buff, char *err){
  char *addr = malloc(sizeof(char)*16);
  int is_ip = 0, i,j;
  int offset;
  struct hostent *hent;
  unsigned long tmp;
  char temp[17];
  char *pos1, *pos2;

  if (strlen(buff) < 16){
    strcpy(temp, buff);
    is_ip = 1;
    
    pos2 = temp;
    for(i = 0; i < 3 ; i++){
      pos1 = pos2;
      if ((pos2 = strchr(pos1, '.')) == NULL){
	is_ip = 0;
	break;
      }
      *pos2 = '\0';
      pos2++;
      j = atoi(pos1);
      if(j > 254 && j < 1){
	is_ip = 0;
	break;
      }
    }
  }

  if (is_ip){
    strcpy(addr, buff);
    return addr;
  }

  if((hent = gethostbyname(buff)) == NULL){
    ERROR_CUSTM(err);
    free(addr);
    return NULL;
  }
  
  tmp = ntohl(((struct in_addr*)hent->h_addr_list[0])->s_addr);
  offset = 0;
  for (i = 0; i < 4; i++){
    j = snprintf(addr+offset, 4, "%lu", (tmp >> (8*(3-i))) & 0xFF);
    if(i){
      *(addr+offset-1) = '.';
    }
    offset = offset + j +1;
  }
  return addr;
}

// write some helping stuff to the tty
void write_help(char *appname){
  printf("\n"
#ifdef IS_DEBUG
	 "Usage: %s [-q] [-v] [-a <bind_address>] [-p <bind_port>]\n"
#else
	 "Usage: %s [-q] [-a <bind_address>] [-p <bind_port>]\n"
#endif
	 "          [-d <remote_address>] [-r <remote_port>]\n"
	 "\n"
	 "-q\n"
	 "\t Quiet mode (no errors will be written)\n"
	 "\n"
#ifdef IS_DEBUG
	 "-v\n"
	 "\t Verbose mode (write all the debugging messages)\n"
	 "\n"
#endif
	 "-a <bind_address>\n"
	 "\t The address the relay should listen on. \n"
	 "\t (Default: %s)\n"
	 "\n"
	 "-p <bind_port>\n"
	 "\t The port the relay should listen on.\n"
	 "\t (Default: %s)\n"
	 "\n"
	 "-d <remote_address>\n"
	 "\t The Address of the destination-Mailserver\n"
	 "\t (Default: %s)\n"
	 "\n"
	 "-s \n"
	 "\t Switches zo SMTP mode instead ob user friendly Prompt\n"
	 "\n"
	 "-r <remote_port>\n"
	 "\t The Port of the destination Mailserver\n"
	 "\t (Default: %s)\n"
	 "\n"
	 "Thats all, have fun\n",
         appname,
	 DEFAULT_BIND_ADDR,
	 DEFAULT_BIND_PORT,
	 DEFAULT_REMOTE_ADDR,
	 DEFAULT_REMOTE_PORT);

}

// getopt stuff to fill the config struct
int process_opt(int argc, char *argv[]){
  int c;
  
  app = malloc(sizeof(app_settings_t));

  app->server_addr = DEFAULT_BIND_ADDR;
  app->server_port = DEFAULT_BIND_PORT;
  app->remote_addr = DEFAULT_REMOTE_ADDR;
  app->remote_port = DEFAULT_REMOTE_PORT;
  app->debug       = 0;
  app->quiet       = 0;
  app->smtp        = 0;

  while ((c = getopt (argc, argv, "qva:p:d:r:hs")) != -1){
    switch (c) {
#ifdef IS_DEBUG
      case 'v':
	app->debug = 1;
	break;
#endif
      case 'a':
	if((app->server_addr = host_opt(optarg, ERR_OPT_SERV_ADD)) == NULL)
	  return FAIL;
	break;
      case 'p':
	if((app->server_port = port_opt(optarg, ERR_OPT_SERV_PORT)) == NULL)
	  return FAIL;
	break;
      case 'd':
	if((app->remote_addr = host_opt(optarg, ERR_OPT_REMOTE_ADD)) == NULL)
	  return FAIL;
	break;
      case 'r':
	if((app->remote_port = port_opt(optarg, ERR_OPT_REMOTE_PORT)) == NULL)
	  return FAIL;
	break;
      case 's':
	app->smtp = 1;
	break;
      case 'q':
	app->quiet = 1;
	break;
      case 'h':
	write_help(argv[0]);
	return FAIL;
      case '?':
	write_help(argv[0]);
	return FAIL;
    }
  }
  DEBUG_S("Server Address",app->server_addr);
  DEBUG_S("Server Port",app->server_port);
  DEBUG_S("Remote Address",app->remote_addr);
  DEBUG_S("Remote Port",app->remote_port);
  DEBUG_N("Debug",app->debug);
  DEBUG_N("Quiet",app->quiet);
  return OK;
}

// the main ...
int main(int argc, char *argv[]){

  if(signal(SIGABRT, sig_abrt_conn) == SIG_ERR){
    ERROR_SYS("Sighandler 'SIGABRT'");
  }
  if(signal(SIGTERM, sig_abrt_conn) == SIG_ERR){
    ERROR_SYS("Sighandler 'SIGTERM'");
  }
  if(signal(SIGQUIT, sig_abrt_conn) == SIG_ERR){
    ERROR_SYS("Sighandler 'SIGQUIT'");
  }
  if(signal(SIGINT, sig_abrt_conn) == SIG_ERR){
    ERROR_SYS("Sighandler 'SIGINT'");
  }
  
  if (process_opt(argc, argv) == FAIL){
    exit(1);
  }
  if(create_conn(app->server_addr, app->server_port)){
    exit(1);
  }
#ifdef USE_DMALLOC
  dmalloc_shutdown();
#endif
  return 0;
}


