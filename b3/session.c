#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "session.h"
#include "smtprelay.h"

#define OK 1
#define FAIL 0

enum session {
  SESSION_RUN,
  SESSION_QUIT,
  SESSION_RESET,
  SESSION_ABORT
};
enum read {
  READ_OK,
  READ_RESET,
  READ_QUIT,
  READ_ERR,
  READ_MEM
};
enum check {
  CHECK_OK,
  CHECK_ARG,
  CHECK_ARG_MSG,
  CHECK_DELIM,
  CHECK_PREF,
};
enum check_arg{
  ARG_OK,
  ARG_BAD,
  ARG_BAD_MSG
};

typedef struct data_line{
  char *data;
  struct data_line *next;
} data_line_t;

static int write_client_msg(int fd, int status, const char *msg, char *add){
  int len, ret = FAIL;
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
    ret = OK;
  }
  free(buff);
  return ret;
}

int check_addr(char * addr){
  //TODO: Überprüfung, ob valide URL/ip
  return ARG_OK;
}
int check_mail(char * addr){
  //TODO: Überprüfung, ob valide URL/ip
  return ARG_OK;
}

static int check_input(char *buff,  char *prefix, char delim, int (*check_fkt)(char *), char **val){
  char *arg;
  int check;
  int len = strlen(buff);

  if(val != NULL){
    *val = NULL;
  }
  if((arg = strchr(buff, delim)) == NULL){
    DEBUG_CLNT_S("Delim-check failed", buff);
    return CHECK_DELIM;
  }
  *arg = '\0';
  if(strncmp(buff, prefix, strlen(prefix)) != 0){
    DEBUG_CLNT_S_S("Prefix-check failed", buff, prefix);
    return CHECK_PREF;
  }

  if(delim != '\0'){
    arg++;
    if (arg > (buff + (len - 1))){
      DEBUG_CLNT_S("No Argument after", buff);
      return CHECK_ARG;
    }

    if(check_fkt != NULL && (check = check_fkt(arg)) != ARG_OK){
      DEBUG_CLNT_S_S("Input-check failed", buff, arg);
      return (check == ARG_BAD_MSG ? CHECK_ARG_MSG : CHECK_ARG);
    }

    if(val != NULL){
      *val = malloc(sizeof(char) * (strlen(arg) + 2));
      strcpy(*val, arg);
    }
    DEBUG_CLNT_S_S("Read ", buff, arg);
  } else {
    DEBUG_CLNT_S("Read ",buff);
  }
  return CHECK_OK;

}

static int fetch_input_line(int fd, char *prefix, char delim, int (*check_fkt)(char *), char **val){
  char buff[1024];
  int len, check;

  while (1){
    memset(buff, 0, 1024);
    if((len = read(fd, buff, 1024)) > 0){
      *(buff + (len-2)) = '\0';
      check = check_input(buff, prefix, delim, check_fkt, val);
      if (check == CHECK_OK){
	return READ_OK;
      }
      if (check == CHECK_ARG_MSG){
	continue;
      }
      if (check == CHECK_ARG){
        write_client_msg(fd, 501, MSG_SYNTAX_ARG, NULL);
	continue;
      }
      if(check_input(buff, "RSET", '\0', NULL, val) == CHECK_OK){
	write_client_msg(fd, 250, MSG_RESET, NULL); 
	return READ_RESET;
      }
      if(check_input(buff, "QUIT", '\0', NULL, val) == CHECK_OK){
	write_client_msg(fd, 250, MSG_BYE, NULL); 
	return READ_QUIT;
      }
      if(check_input(buff, "NOOP", '\0', NULL, val) == CHECK_OK){
        write_client_msg(fd, 250, MSG_NOOP, NULL);
	continue;
      }
      if(check_input(buff, "VRFY", '\0', NULL, val) == CHECK_OK){
        write_client_msg(fd, 502, MSG_NOT_IMPL, "VRFY");
	continue;
      }
      if(check_input(buff, "EXPN", '\0', NULL, val) == CHECK_OK){
        write_client_msg(fd, 502, MSG_NOT_IMPL, "EXPN");
	continue;
      }
      if(check_input(buff, "HELP", '\0', NULL, val) == CHECK_OK){
        write_client_msg(fd, 502, MSG_NOT_IMPL, "HELP");
	continue;
      }
      write_client_msg(fd, 500, MSG_SYNTAX, NULL);
    } else {
      if(len == 0){
	DEBUG_CLNT("Empty Data readed");
	return READ_ERR;
	//Keine Daten
      } else {
	DEBUG_CLNT("Read error");
	put_err("Reading Data from Client");
	//lesefehler
	return READ_ERR;
      }
    }
  }
  return READ_OK;
}

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

int session_sequence(int fd){
  char *client_addr = NULL;
  char *sender_mail = NULL;
  char *rcpt_mail = NULL;
  int result;
  data_line_t * data = NULL, *walker;
  int i = 0;
  char buff[10];

  // GREET
  DEBUG_CLNT("Greet Client"); 
  if(write_client_msg(fd, 220, MSG_GREET,server_addr) == FAIL){
    DEBUG_CLNT("Write Failed, Abort Session");
    put_err("Wrie to Client");
    return SESSION_ABORT;
  }
  
  //HELO
  DEBUG_CLNT("Wait for HELO");
  result = fetch_input_line(fd, "HELO", ' ', check_addr, &client_addr);
  if (result == READ_OK){
    DEBUG_CLNT_S("Client Helo", client_addr);
    if(write_client_msg(fd, 250, MSG_HELLO, client_addr) == FAIL){
      DEBUG_CLNT("Write Failed, Abort Session");
      put_err("Wrie to Client");
      return SESSION_ABORT;
    }
  } else {
    return (result == READ_ERR ? SESSION_ABORT : (result == READ_QUIT ? SESSION_QUIT : SESSION_RESET));
  }

  //MAIL FROM
  DEBUG_CLNT("Wait for MAIL FROM");
  result = fetch_input_line(fd, "MAIL FROM", ':', check_mail, &sender_mail);
  if (result == READ_OK){
    DEBUG_CLNT_S("Sender Mail", sender_mail);
    if(write_client_msg(fd, 250, MSG_SENDER, sender_mail) == FAIL){
      DEBUG_CLNT("Write Failed, Abort Session");
      put_err("Wrie to Client");
      return SESSION_ABORT;
    }
  } else { 
    return (result == READ_ERR ? SESSION_ABORT : (result == READ_QUIT ? SESSION_QUIT : SESSION_RESET));
  }


  //RCPT TO
  DEBUG_CLNT("Wait for RCPT TO");
  result = fetch_input_line(fd, "RCPT TO", ':', check_mail, &rcpt_mail);
  if (result == READ_OK){
    DEBUG_CLNT_S("rcpt Mail", rcpt_mail);
    if(write_client_msg(fd, 250, MSG_RCPT, rcpt_mail) == FAIL){
      DEBUG_CLNT("Write Failed, Abort Session");
      put_err("Wrie to Client");
      return SESSION_ABORT;
    }
  } else { 
    return (result == READ_ERR ? SESSION_ABORT : (result == READ_QUIT ? SESSION_QUIT : SESSION_RESET));
  }

  //DATA
  DEBUG_CLNT("Wait for DATA");
  result = fetch_input_line(fd, "DATA", '\0', NULL, NULL);
  if (result == READ_OK){
    DEBUG_CLNT("DATA ....");
    if(write_client_msg(fd, 354, MSG_DATA, NULL) == FAIL){
      DEBUG_CLNT("Write Failed, Abort Session");
      put_err("Wrie to Client");
      return SESSION_ABORT;
    }
  } else { 
    return (result == READ_ERR ? SESSION_ABORT : (result == READ_QUIT ? SESSION_QUIT : SESSION_RESET));
  }

  DEBUG_CLNT("Reading DATA");
  result = read_data(fd, &data);
  if (result == READ_OK){
    DEBUG_CLNT("Readed Data:");
    walker = data;
    while (walker != NULL){
      DEBUG_CLNT_S("", walker->data);
      walker = walker->next;  
      i++;
    }
    snprintf(buff, 10, "%d", i);
    if(write_client_msg(fd, 250, MSG_DATA_ACK, buff) == FAIL){
      DEBUG_CLNT("Write Failed, Abort Session");
      put_err("Wrie to Client");
      return SESSION_ABORT;
    }    
  } else {
    if(result == READ_MEM){
      if(write_client_msg(fd, 552, MSG_MEM, NULL) == FAIL){
	DEBUG_CLNT("Write Failed, Abort Session");
	put_err("Wrie to Client");
	return SESSION_ABORT;
      } 
      return SESSION_ABORT;
    }
  }


// Read DATA!!

  //QUIT
  DEBUG_CLNT("Wait for QUIT");
  result = fetch_input_line(fd, "QUIT", '\0', NULL, NULL);
  if (result == READ_OK){
    DEBUG_CLNT("Client QUIT");
    if(write_client_msg(fd, 221, MSG_BYE, NULL) == FAIL){
      DEBUG_CLNT("Write Failed, Abort Session");
      put_err("Wrie to Client");
      return SESSION_ABORT;
    }
  } else {
    return (result == READ_ERR ? SESSION_ABORT : (result == READ_QUIT ? SESSION_QUIT : SESSION_RESET));
  }

  if(client_addr != NULL){
    free(client_addr);
  }
  if(sender_mail != NULL){
    free(sender_mail);
  }
  if(rcpt_mail != NULL){
    free(rcpt_mail);
  }
  return SESSION_QUIT;
}

int start_session(int fd){
  int session = SESSION_RUN;
  while(session == SESSION_RUN || session == SESSION_RESET){
    session = session_sequence(fd);
  }
  return 0;
}
