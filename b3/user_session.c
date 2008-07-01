#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "smtprelay.h"
#include "session.h"
#include "sender.h"



int user_write_client_msg(int fd, int status, const char *msg, char *add){
  int len, ret = FAIL; 
  char *st, *buff;

  if(add == NULL){
    len = strlen(msg)+10;
  } else {
    len = strlen(msg) + strlen(add) + 20;
  }

  buff = malloc(sizeof(char) * len);
  if (status){
    st = "FAIL";
    if (status == 1){
      st = "OK";
    } 
    if(add == NULL){
      len = snprintf(buff, len - 1, msg , st);
    } else {
      len = snprintf(buff, len - 1, msg , st, add);
    }
  } else {
    if(add == NULL){
      len = snprintf(buff, len - 1, msg);
    } else {
      len = snprintf(buff, len - 1, msg, add);
    }
  }
  if(write(fd, buff, len+1) > 0){
    ret = OK;
  }

  free(buff);

  return ret;
}

// Checks if Confirm is OK
int user_check_confirm(char *buff){
  if (strcmp(buff, "send") == 0){
    return ARG_OK;
  }
  return ARG_BAD;
}

// Check if input is min2 chars
int user_check_noempty(char *buff){
  if (strlen(buff) > 1){
    return ARG_OK;
  }
  return ARG_BAD;
}

// Ask the User a Question
int user_ask_client(int fd, const char *question, const char *ok_msg, const char *fail_msg, char ** destination, int (*check_fkt)(char *), int lock_reset){
  int  len;
  char buff[1024];

  if(user_write_client_msg(fd, 0, question, NULL) == FAIL){
    DEBUG_CLNT("Write Failed, Abort Session");
    ERROR_SYS("Write to Client");
    return READ_ERR;
  }
  while (1) {
    memset(buff, 0, 1024);
    if ((len = read(fd, buff, 1024)) > 0){
      *(buff + (len-2)) = '\0';

      DEBUG_CLNT_S("Client Input", buff);

      if (strcmp(buff, "quit") == 0){
	DEBUG_CLNT("Quit Session");
	if(user_write_client_msg(fd, 1, USER_MSG_QUIT, NULL) == FAIL){
	  DEBUG_CLNT("Write Failed, Abort Session");
	  ERROR_SYS("Write to Client");
	  return READ_ERR;
	}
	return READ_QUIT;
      }
      if (!lock_reset && strcmp(buff, "reset") == 0){
	DEBUG_CLNT("Reset Session");
	if(user_write_client_msg(fd, 1, USER_MSG_RESET, NULL) == FAIL){
	  DEBUG_CLNT("Write Failed, Abort Session");
	  ERROR_SYS("Write to Client");
	  return READ_ERR;
	}
	return READ_RESET;
      }

      if (check_fkt(buff) == ARG_OK){
	if(user_write_client_msg(fd, 1, ok_msg, buff) == FAIL){
	  DEBUG_CLNT("Write Failed, Abort Session");
	  ERROR_SYS("Write to Client");
	  return READ_ERR;
	}
	if (destination != NULL){
	  *destination = malloc(sizeof(char) * (strlen(buff) + 2));
	  strcpy(*destination, buff);
	}
	DEBUG_CLNT("Read OK");
	return READ_OK;
      }

      if(user_write_client_msg(fd, 2, fail_msg, buff) == FAIL){
	DEBUG_CLNT("Write Failed, Abort Session");
	ERROR_SYS("Write to Client");
	return READ_ERR;
      }
      DEBUG_CLNT("Read FAIL .. reask");

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
}


// Clean Mail Data
void user_clean_mail_data(mail_data_t* data){
  data_line_t * walker, *tmp;

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
}

// Build a HTML Header with Subject, To and From and prepend it to the Body
data_line_t* user_build_header(char *subject, char *from, char *to, data_line_t *body){
  data_line_t *ret, *tmp;
  
  char *header_subject = malloc(sizeof(char) * 1024);
  char *header_to = malloc(sizeof(char) * 1024);
  char *header_from = malloc(sizeof(char) * 1024);

  snprintf(header_subject, 1023, "Subject: %s", subject);
  snprintf(header_to, 1023, "To: %s", to);
  snprintf(header_from, 1023, "From: %s", from);

  tmp = malloc(sizeof(data_line_t));
  tmp->data = header_from;
  ret = tmp;
  tmp->next = malloc(sizeof(data_line_t));
  tmp = tmp->next;
  tmp->data = header_to;
  tmp->next = malloc(sizeof(data_line_t));
  tmp = tmp->next;
  tmp->data = header_subject;
  tmp->next = body;
  return ret;
}

int user_start_session(int fd){
  int session = SESSION_RUN, i=0;
  mail_data_t *data = NULL;
  data_line_t *walker;
  int result;
  char buff[33];
  char *subject = NULL;

  data = malloc(sizeof(mail_data_t));
  memset(data, 0, sizeof(mail_data_t));

  // Greet
  DEBUG_CLNT("Greet Client"); 
  if(user_write_client_msg(fd, 0, USER_MSG_GREET,app->server_addr) == FAIL){
    DEBUG_CLNT("Write Failed, Abort Session");
    ERROR_SYS("Write to Client");
    session = SESSION_ABORT;
  } else {

    // Ask Who (HELO)
    DEBUG_CLNT("Ask who"); 
    result = user_ask_client(fd, USER_MSG_WHO_ASK, USER_MSG_WHO_OK, USER_MSG_WHO_FAIL, &(data->client_addr), check_addr, 1);
    if (result == READ_OK){
      DEBUG_CLNT_S("Client Helo", data->client_addr);
      if (user_write_client_msg(fd, 0, USER_MSG_HELLO, data->client_addr) == FAIL
	  || user_write_client_msg(fd, 0, USER_MSG_HELP_1, NULL) == FAIL
	  || user_write_client_msg(fd, 0, USER_MSG_HELP_2, NULL) == FAIL){
	DEBUG_CLNT("Write Failed, Abort Session");
	ERROR_SYS("Wrie to Client");
	session = SESSION_ABORT;
      }
    } else {
      session = SESSION_ABORT;
    }
  }

  while(session == SESSION_RUN || session == SESSION_RESET){

    // Sender
    result = user_ask_client(fd, USER_MSG_SENDER_ASK, USER_MSG_SENDER_OK, USER_MSG_SENDER_FAIL, &(data->sender_mail), check_mail, 0);    
    if (result == READ_OK){
      DEBUG_CLNT_S("Sender OK", data->sender_mail);
    } else {
      if (result == READ_RESET){
	session = SESSION_RESET;
	user_clean_mail_data(data);
	continue;
      } else {
	session = SESSION_ABORT;
	user_clean_mail_data(data);
	break;
      }
    }

    // RCPT
    result = user_ask_client(fd, USER_MSG_RCPT_ASK, USER_MSG_RCPT_OK, USER_MSG_RCPT_FAIL, &(data->rcpt_mail), check_mail, 0);    
    if (result == READ_OK){
      DEBUG_CLNT_S("Sender OK", data->rcpt_mail);
    } else {
      if (result == READ_RESET){
	session = SESSION_RESET;
	user_clean_mail_data(data);
	continue;
      } else {
	session = SESSION_ABORT;
	user_clean_mail_data(data);
	break;
      }
    }

    // Subject
    result = user_ask_client(fd, USER_MSG_SUBJECT_ASK, USER_MSG_SUBJECT_OK, USER_MSG_SUBJECT_FAIL, &(subject), user_check_noempty, 0);    
    if (result == READ_OK){
      DEBUG_CLNT_S("Subject OK", subject);
    } else {
      if (result == READ_RESET){
	session = SESSION_RESET;
	user_clean_mail_data(data);
	continue;
      } else {
	session = SESSION_ABORT;
	user_clean_mail_data(data);
	break;
      }
    }

    // Data
    if (user_write_client_msg(fd, 0, USER_MSG_DATA_1, NULL) == FAIL
	|| user_write_client_msg(fd, 0, USER_MSG_DATA_2, NULL) == FAIL){
      DEBUG_CLNT("Write Failed, Abort Session");
      ERROR_SYS("Wrie to Client");
      session = SESSION_ABORT;
      user_clean_mail_data(data);
      break;
    }

    // Read DATA!!
    DEBUG_CLNT("Reading DATA");
    result = read_data(fd, &(data->data));
    if (result == READ_OK){
      DEBUG_CLNT("Readed Data:");
      walker = data->data;
      i = 0;
      while (walker != NULL){
	DEBUG_CLNT_S("", walker->data);
	walker = walker->next;  
	i++;
      }
      sprintf(buff,"%d",i);
      if (user_write_client_msg(fd, 0, USER_MSG_READED, buff) == FAIL){
	DEBUG_CLNT("Write Failed, Abort Session");
	ERROR_SYS("Write to Client");
	session = SESSION_ABORT;
	user_clean_mail_data(data);
	break;
      } 
    } else {
      if(result == READ_MEM){
	if(user_write_client_msg(fd, 0, USER_MSG_MEM, NULL) == FAIL){
	  DEBUG_CLNT("Write Failed, Abort Session");
	  ERROR_SYS("Write to Client");
	} 
      }
      session = SESSION_ABORT;
      user_clean_mail_data(data);
      break;
    }

    // Build Header
    data->data = user_build_header(subject, data->sender_mail, data->rcpt_mail, data->data);
    free(subject);

    //Really Send?
    result = user_ask_client(fd, USER_MSG_CONFIRM_ASK, USER_MSG_CONFIRM_OK, USER_MSG_CONFIRM_FAIL, NULL, user_check_confirm, 0);
    if (result == READ_OK){
      DEBUG_CLNT("Confirm OK");
    } else {
      if (result == READ_RESET){
	session = SESSION_RESET;
	user_clean_mail_data(data);
	continue;
      } else {
	session = SESSION_ABORT;
	user_clean_mail_data(data);
	break;
      }
    }

    // Send Mail
    DEBUG_CLNT("Forward Mail");
    result = forward_mail(fd, data);
    if(result == OK){      
      if(user_write_client_msg(fd, 1, USER_MSG_DATA_ACK,NULL) == FAIL){
	DEBUG_CLNT("Write Failed, Abort Session");
	ERROR_SYS("Wrie to Client");
	session = SESSION_ABORT;
	user_clean_mail_data(data);
	break;
      }
    } else {
      if(user_write_client_msg(fd, 2, USER_MSG_DATA_FAIL,NULL) == FAIL){
	DEBUG_CLNT("Write Failed, Abort Session");
	ERROR_SYS("Wrie to Client");
	session = SESSION_ABORT;
	user_clean_mail_data(data);
	break;
      }
    }
    session = SESSION_RUN;

  } // Session Loop end

  // Free session resouces
  if(data != NULL){ 
    if(data->client_addr != NULL){
      free(data->client_addr);
    }
    free(data);
  }
  return 0;
}

