#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "session.h"
#include "smtprelay.h"

static int write_client_msg(int fd, int status, const char *msg, char *add){
  int len, ret = 1;
  char *buff;
  
  if(add == NULL){
    len = strlen(msg)+10;
  } else {
    len = strlen(msg) + strlen(add) + 20;
  }
  buff = malloc(sizeof(char) * len);
  if(add == NULL){
    len = snprintf(buff, len - 1, msg , status);
  } else {
    len = snprintf(buff, len - 1, msg , status, add);
  }
  if(write(fd, buff, len+1) > 0){
    ret = 0;
  }
  free(buff);
  return ret;
}

int start_session(int fd){
   write_client_msg(fd, 220, MSG_GREET,server_addr); 

   return 0;
}
