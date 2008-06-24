#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "smtprelay.h"
#include "session.h"
#include "sender.h"



// Write something to the client
int smtp_write_client_msg(int fd, int status, const char *msg, char *add){
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

// Check the Input of the Client
static int smtp_check_input(char *buff,  char *prefix, char delim, int (*check_fkt)(char *), char **val){
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

// Fetches a Input Line from the Socket and check them
static int smtp_fetch_input_line(int fd, char *prefix, char delim, int (*check_fkt)(char *), char **val){
  char buff[1024];
  int len, check;

  while (1){
    memset(buff, 0, 1024);
    if((len = read(fd, buff, 1024)) > 0){
      *(buff + (len-2)) = '\0';
      check = smtp_check_input(buff, prefix, delim, check_fkt, val);
      if (check == CHECK_OK){
	return READ_OK;
      }
      if (check == CHECK_ARG_MSG){
	continue;
      }
      if (check == CHECK_ARG){
        smtp_write_client_msg(fd, 501, SMTP_MSG_SYNTAX_ARG, NULL);
	continue;
      }
      if(smtp_check_input(buff, "RSET", '\0', NULL, val) == CHECK_OK){
	smtp_write_client_msg(fd, 250, SMTP_MSG_RESET, NULL); 
	return READ_RESET;
      }
      if(smtp_check_input(buff, "QUIT", '\0', NULL, val) == CHECK_OK){
	smtp_write_client_msg(fd, 250, SMTP_MSG_BYE, NULL); 
	return READ_QUIT;
      }
      if(smtp_check_input(buff, "NOOP", '\0', NULL, val) == CHECK_OK){
        smtp_write_client_msg(fd, 250, SMTP_MSG_NOOP, NULL);
	continue;
      }
      if(smtp_check_input(buff, "VRFY", '\0', NULL, val) == CHECK_OK){
        smtp_write_client_msg(fd, 502, SMTP_MSG_NOT_IMPL, "VRFY");
	continue;
      }
      if(smtp_check_input(buff, "EXPN", '\0', NULL, val) == CHECK_OK){
        smtp_write_client_msg(fd, 502, SMTP_MSG_NOT_IMPL, "EXPN");
	continue;
      }
      if(smtp_check_input(buff, "HELP", '\0', NULL, val) == CHECK_OK){
        smtp_write_client_msg(fd, 502, SMTP_MSG_NOT_IMPL, "HELP");
	continue;
      }
      if(smtp_check_input(buff, "HELO", '\0', NULL, val) == CHECK_OK
	  || smtp_check_input(buff, "MAIL FROM", '\0', NULL, val) == CHECK_OK
	  || smtp_check_input(buff, "DATA", '\0', NULL, val) == CHECK_OK
	  || smtp_check_input(buff, "RCPT TO", '\0', NULL, val) == CHECK_OK){
        smtp_write_client_msg(fd, 503, SMTP_MSG_SEQ, NULL);
	continue;
      }
      smtp_write_client_msg(fd, 500, SMTP_MSG_SYNTAX, NULL);
    } else {
      if(len == 0){
	DEBUG_CLNT("Empty Data readed");
	return READ_ERR;
	//Keine Daten
      } else {
	DEBUG_CLNT("Read error");
	ERROR_SYS("Reading Data from Client");
	//lesefehler
	return READ_ERR;
      }
    }
  }
  return READ_OK;
}

// process the session Sequence
int smtp_session_sequence(int fd, mail_data_t **mail_d){
  mail_data_t * data  ;
  int result;
  data_line_t * walker;
  int i = 0;

  data = *mail_d;

  //MAIL FROM
  DEBUG_CLNT("Wait for MAIL FROM");
  result = smtp_fetch_input_line(fd, "MAIL FROM", ':', check_mail, &(data->sender_mail));
  if (result == READ_OK){
    DEBUG_CLNT_S("Sender Mail", data->sender_mail);
    if(smtp_write_client_msg(fd, 250, SMTP_MSG_SENDER, data->sender_mail) == FAIL){
      DEBUG_CLNT("Write Failed, Abort Session");
      ERROR_SYS("Wrie to Client");
      return SESSION_ABORT;
    }
  } else { 
    return (result == READ_ERR ? SESSION_ABORT : (result == READ_QUIT ? SESSION_QUIT : SESSION_RESET));
  }

  //RCPT TO
  DEBUG_CLNT("Wait for RCPT TO");
  result = smtp_fetch_input_line(fd, "RCPT TO", ':', check_mail, &(data->rcpt_mail));
  if (result == READ_OK){
    DEBUG_CLNT_S("rcpt Mail", data->rcpt_mail);
    if(smtp_write_client_msg(fd, 250, SMTP_MSG_RCPT, data->rcpt_mail) == FAIL){
      DEBUG_CLNT("Write Failed, Abort Session");
      ERROR_SYS("Wrie to Client");
      return SESSION_ABORT;
    }
  } else { 
    return (result == READ_ERR ? SESSION_ABORT : (result == READ_QUIT ? SESSION_QUIT : SESSION_RESET));
  }

  //DATA
  DEBUG_CLNT("Wait for DATA");
  result = smtp_fetch_input_line(fd, "DATA", '\0', NULL, NULL);
  if (result == READ_OK){
    DEBUG_CLNT("DATA ....");
    if(smtp_write_client_msg(fd, 354, SMTP_MSG_DATA, NULL) == FAIL){
      DEBUG_CLNT("Write Failed, Abort Session");
      ERROR_SYS("Wrie to Client");
      return SESSION_ABORT;
    }
  } else { 
    return (result == READ_ERR ? SESSION_ABORT : (result == READ_QUIT ? SESSION_QUIT : SESSION_RESET));
  }

  // Read DATA!!
  DEBUG_CLNT("Reading DATA");
  result = read_data(fd, &(data->data));
  if (result == READ_OK){
    DEBUG_CLNT("Readed Data:");
    walker = data->data;
    while (walker != NULL){
      DEBUG_CLNT_S("", walker->data);
      walker = walker->next;  
      i++;
    }
  } else {
    if(result == READ_MEM){
      if(smtp_write_client_msg(fd, 552, SMTP_MSG_MEM, NULL) == FAIL){
	DEBUG_CLNT("Write Failed, Abort Session");
	ERROR_SYS("Wrie to Client");
	return SESSION_ABORT;
      } 
      return SESSION_ABORT;
    }
  }
  return SESSION_SEND;
}

// initiate a new session
int smtp_start_session(int fd){
  int session = SESSION_RUN;
  mail_data_t *data = NULL;
  data_line_t *tmp, *walker;
  int result;

  data = malloc(sizeof(mail_data_t));

  memset(data, 0, sizeof(mail_data_t));

  // GREET
  DEBUG_CLNT("Greet Client"); 
  if(smtp_write_client_msg(fd, 220, SMTP_MSG_GREET,app->server_addr) == FAIL){
    DEBUG_CLNT("Write Failed, Abort Session");
    ERROR_SYS("Wrie to Client");
    session = SESSION_ABORT;
  }

  //HELO
  DEBUG_CLNT("Wait for HELO");
  result = smtp_fetch_input_line(fd, "HELO", ' ', check_addr, &(data->client_addr));
  if (result == READ_OK){
    DEBUG_CLNT_S("Client Helo", data->client_addr);
    if(smtp_write_client_msg(fd, 250, SMTP_MSG_HELLO, data->client_addr) == FAIL){
      DEBUG_CLNT("Write Failed, Abort Session");
      ERROR_SYS("Wrie to Client");
      session = SESSION_ABORT;
    }
  } else {
    if (result == READ_ERR){
      session = SESSION_ABORT;
    } else {
      if(result == READ_QUIT){
	session = SESSION_QUIT;
      }else {
	session = SESSION_RESET;
      }
    }
  }

  while(session == SESSION_RUN || session == SESSION_RESET){
    session = smtp_session_sequence(fd, &data);

    if(session == SESSION_SEND){
      DEBUG_CLNT("Forward Mail");
      result = forward_mail(fd, data);
      if(result == OK){      
	if(smtp_write_client_msg(fd, 250, SMTP_MSG_DATA_ACK,NULL) == FAIL){
	  DEBUG_CLNT("Write Failed, Abort Session");
	  ERROR_SYS("Wrie to Client");
	  session = SESSION_ABORT;
	}
      } else {
	if(smtp_write_client_msg(fd, 451, SMTP_MSG_DATA_FAIL,NULL) == FAIL){
	  DEBUG_CLNT("Write Failed, Abort Session");
	  ERROR_SYS("Wrie to Client");
	  session = SESSION_ABORT;
	}
      }
      session = SESSION_RUN;
    }

    if(data != NULL){ 
      if(data->sender_mail != NULL){
	free(data->sender_mail);
	data->sender_mail=NULL;
      }
      if(data->rcpt_mail != NULL){
	free(data->rcpt_mail);
	data->rcpt_mail=NULL;
      }
      if(data->data != NULL){
	walker = data->data;
	while (walker != NULL){
	  tmp = walker;
	  walker = walker->next;
	  free(tmp->data);
	  free(tmp);
	}
	data->data =  NULL;
      }
    }
  } // Session Loop end

  if(data != NULL){ 
    if(data->client_addr != NULL){
      free(data->client_addr);
    }
    free(data);
  }
  return 0;
}
