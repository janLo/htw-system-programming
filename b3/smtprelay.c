#include <stdlib.h>
#include <signal.h>
#include "smtprelay.h"
#include "connection.h"

void put_err(char *src){
  if (src == NULL){
    fprintf(stderr, ERROR_PREF);
  } else {
    fprintf(stderr, ERROR_PREF "%s - ", src);
  }
  perror("");
}



int process_opt(int argc, char *argv[], char **addr, char **port){
  if (argc > 1) {
    //TODO Args parsen und in struct packen
    //TODO Beispiel Seite 695
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
  if(create_conn(addr,port)){
    exit(1);
  }
  return 0;
}


