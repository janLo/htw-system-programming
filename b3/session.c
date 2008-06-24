#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "session.h"
#include "smtprelay.h"
#include "smtp_session.h"
#include "user_session.h"



// Every Hostname longer than 1 char is OK for me
// (I've host aliases with 2 chars)
int check_addr(char * addr){
  return ((strlen(addr) > 1) ? ARG_OK : ARG_BAD);
}

// I think, a 3 char Name, a @ and a valid 
// hostname is enough to say 'the adress is valid'.
// If not, the Forward Server will tell us.
int check_mail(char * addr){
  char *pos = strchr(addr, '@');
  if(pos != NULL){
    if (check_addr(pos+1) == ARG_OK){
      if ((pos - addr) > 2){
	return ARG_OK;
      }
    }
  }
  return ARG_BAD;
}



// Read the DATA Block
int read_data(int fd, data_line_t **data_head){
  data_line_t *walker = NULL , *tmp;
  char buffer[1024];
  char *c;

  *data_head = NULL;

  while(read(fd, buffer, 1024) > 0){
    if (strncmp(buffer, ".\r\n", 3) == 0){
      return READ_OK;
    }
    if((c = strchr(buffer, '\n')) != NULL){
      *c = '\0';
    }
    if((c = strchr(buffer, '\r')) != NULL){
      *c = '\0';
    }
    DEBUG_CLNT_S("DATA Line", buffer);
    
    if((tmp = malloc(sizeof(data_line_t))) == NULL){
      return READ_MEM;
    }
    if((tmp->data = malloc(sizeof(char) * (strlen(buffer) + 2))) == NULL){
      return READ_MEM;
    }
    strcpy(tmp->data, buffer);
    tmp->next = NULL;

    if(*data_head == NULL){
      *data_head = tmp;
    }
    if(walker != NULL){
      walker->next = tmp;
    }
    walker = tmp;
  }
  return READ_ERR;
}


// puts the forward prtocol to the client
void put_forward_proto(int fd, int status, data_line_t *proto){
  data_line_t *walker;
  walker = proto;
  while (walker != NULL){
    DEBUG_CLNT_S("Write Proto", walker->data);
    if(app->smtp){
      smtp_write_client_msg(fd, status, SMTP_MSG_PROTO, walker->data);
    } else {
      user_write_client_msg(fd, (status == 250 ? 1 : 2), USER_MSG_PROTO, walker->data);
    }
    walker = walker->next;
  }
}


int start_session(int fd){
  if(app->smtp){
    return smtp_start_session(fd);
  } else {
    return user_start_session(fd);
  }
}
