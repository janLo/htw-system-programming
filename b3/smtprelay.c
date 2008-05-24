#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include "smtprelay.h"
#include "connection.h"

char *server_addr = NULL;

void put_err(char *src){
  if (src == NULL){
    fprintf(stderr, ERROR_PREF);
  } else {
    fprintf(stderr, ERROR_PREF "%s - ", src);
  }
  perror("");
}
void put_err_str(char *err){
  if(err != NULL){
    fprintf(stderr, ERROR_PREF "%s \n", err);
  } else {
    fprintf(stderr, ERROR_PREF " Unspecified\n");
  }
}

char *port_opt(char *buff, char *err){
  char *port = malloc(sizeof(char)*6);
  int is_digit = 1, i;
  buff[5] = '\0';

  for(i = 0; i < 5; i++){
    if(!isdigit(buff[i])){
      is_digit = 0;
      break;
    }
  }

  if(is_digit || atoi(buff) > 65535 || atoi(buff) < 1){
    put_err_str(err);
    free(port);
    return NULL;
  }
  strcpy(port, buff);
  return port;
}

char *host_opt(char *buff, char *err){
  char *addr = malloc(sizeof(char)*16);
  int is_ip = 0, i;
  struct hostent *hent;

  if (strlen(buff) == 15){
    is_ip = 1;
    for(i = 0; i < 4 ; i++){
      offset = i * 4;
      for(j = 0; j < 3; j++){
	if(!isdigit(buff[(offset+j)])){
	  is_ip = 0;
	  break;
	}
      }
      if(!is_ip || !strncmp(&(buff[offset + 4]))){
	is_ip = 0;
	break;
      }
    }
  }

  if (is_ip){
    strcmp(addr, buff);
    return addr;
  }
  
  if((hent = gethostbyname(buff)) == NULL){
    put_err_str(err);
    free(addr);
    return NULL;
  }
  
  tmp = ntohl(((struct in_addr*)hent->h_addr_list[0])->s_addr);

  for (i = 0; i < 4; i++){
    offset = i*4
    snprintf(buff+offset, 4, "%d", (tmp >> (8*i)) & 0xFF);
    if(i){
      *(buff+offset-1) = '.';
    }

  printf("%d\n", bla);

  }




}

int process_opt(int argc, char *argv[], char **addr_ptr, char **port_ptr){
  char *addr = malloc(sizeof(char)*16);
  char buff[1024];

  

  if (argc == 3) {
    *port = argv[2];
    *addr = argv[1];
  } else {
    *port = DEFAULT_BINDPORT;
    *addr = DEFAULT_BINDADDR;
  }
  return 0;
}



int main(int argc, char *argv[]){
  char *addr;
  char *port;


  if(signal(SIGABRT, sig_abrt_conn) == SIG_ERR){
    put_err("Sighandler 'SIGABRT'");
  }
  if(signal(SIGTERM, sig_abrt_conn) == SIG_ERR){
    put_err("Sighandler 'SIGTERM'");
  }
  if(signal(SIGQUIT, sig_abrt_conn) == SIG_ERR){
    put_err("Sighandler 'SIGQUIT'");
  }
  if(signal(SIGINT, sig_abrt_conn) == SIG_ERR){
    put_err("Sighandler 'SIGINT'");
  }
  
  if (process_opt(argc, argv, &addr, &port)){
    exit(1);
  }
  server_addr = addr;
  if(create_conn(addr,port)){
    exit(1);
  }
#ifdef USE_DMALLOC
  dmalloc_shutdown();
#endif
  return 0;
}


