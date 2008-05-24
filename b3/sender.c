#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "connection.h"
#include "session.h"
#include "smtprelay.h"
#include "sender.h"

enum command{
  COMMAND_OK,
  COMMAND_WRITE,
  COMMAND_READ,
  COMMAND_FAIL
};

data_line_t *new_proto_entry(char *data){
  data_line_t *new = malloc(sizeof(data_line_t));
  char *pos;

  new->next = NULL;
  new->data = malloc(sizeof(char) * (strlen(data)+10) );
  strcpy(new->data, data);

  if((pos = strchr(new->data, '\n')) != NULL){
    *pos = '\0';
  }
  if((pos = strchr(new->data, '\r')) != NULL){
    *pos = '\0';
  }
  
  return new;
}

data_line_t *wind_proto(data_line_t *proto_end){
  data_line_t *tmp1, *tmp2;

  tmp1 = proto_end;
  while(tmp1 != NULL){
    tmp2 = tmp1;
    tmp1 = tmp1->next;
  }
  return tmp2;
}

int read_remote(int fd, char *buff, size_t size, data_line_t * protocol_end){
  int ret = 0;

  if((ret = read(fd, buff, size)) > 0){
    protocol_end->next = new_proto_entry(buff);
    return ret;
  } else {
    ERROR_SYS("Reading from Remote Socket");
    DEBUG_CLNT("Error while Reading from remote socket");
    return ret;
  }
}

int write_remote(int fd, char* buff, size_t size, data_line_t * protocol_end){
  int ret = 0;

  if((ret = write(fd, buff, size)) > 0){
    protocol_end->next = new_proto_entry(buff);
    return ret;
  } else {
    ERROR_SYS("Writing on Remote Socket");
    DEBUG_CLNT("Error while Writing on remote socket");
    return ret;
  }
}

void free_protocol(data_line_t * protocol){
  data_line_t * tmp;
  while(protocol != NULL){
    tmp = protocol;
    protocol = protocol->next;
    free(tmp->data);
    free(tmp);
  }
}

int extract_status(char* buff){
  int ret = 0;
  char *pos;
  if((pos = strchr(buff, ' ')) != NULL){
    *pos = '\0';
    ret = atoi(buff);
    *pos = ' ';
  }
  return ret;
}

int try_command(int remote_fd, char *command, int expected_return, data_line_t *protocol_end){
  int i;
  data_line_t *protocol = protocol_end;
  int status;
  char buff[1024];
  char *pos;
  DEBUG_CLNT_S("Try Remote Command", command);
  for(i = 0; i < SEND_MAXTRY; i++){
    snprintf(buff, 1024, "%s\r\n", command);
    if (write_remote(remote_fd, buff, strlen(buff), protocol) <= 0){
    //if (write(remote_fd, buff, strlen(buff)) <= 0){
      ERROR_SYS("Writing on Remote Socket");
      DEBUG_CLNT("Error while Writing on remote socket");
      return COMMAND_WRITE;
    }
    DEBUG_CLNT("Written .. Read Reply");
    protocol = protocol->next;
    if(read_remote(remote_fd, buff, 1024, protocol) <= 0){
      return COMMAND_READ;
    }
    if((pos = strchr(buff, '\n')) != NULL){
      *pos = '\0';
    }
    if((pos = strchr(buff, '\r')) != NULL){
      *pos = '\0';
    }
    DEBUG_CLNT_S("From Remote", buff);
    status = extract_status(buff);
    if(status == expected_return){
      DEBUG_CLNT("Reply from Remote as expected");
      return COMMAND_OK;
    }
    if(status > 499 || status < 400){
      DEBUG_CLNT("Reply from Remote not as expected");
      return COMMAND_FAIL;
    }
    DEBUG_CLNT("Retry Remote Command");
  }
  return COMMAND_FAIL;
}
      


int send_mail(int client_fd, int remote_fd, mail_data_t *data, data_line_t **protocol){
  char buff[1024];
  int status;
  data_line_t *line = NULL;
  data_line_t *protocol_end;

  protocol_end = new_proto_entry("Begin Forward .. Protocoll:");
  *protocol = protocol_end;

  protocol_end->next = new_proto_entry("-----------------------------------------");
  protocol_end = protocol_end->next;


  if(read_remote(remote_fd, buff, 1024, protocol_end) <= 0){
    return FAIL;
  }
  DEBUG_CLNT_S("From Remote", buff);
  protocol_end = protocol_end->next;
  status = extract_status(buff);
  if(status != 220){
    return FAIL;
  }
  DEBUG_CLNT("Remote Connection Seems to be OK, Send Commands");

  snprintf(buff, 1024, "HELO %s", server_addr_string);
  if(try_command(remote_fd, buff, 250, protocol_end) != COMMAND_OK){
    return FAIL;
  }
  protocol_end = wind_proto(protocol_end);
  
  snprintf(buff, 1024, "MAIL FROM:%s", data->sender_mail);
  if(try_command(remote_fd, buff, 250, protocol_end) != COMMAND_OK){
    return FAIL;
  }
  protocol_end = wind_proto(protocol_end);

  snprintf(buff, 1024, "RCPT TO:%s", data->rcpt_mail);
  if(try_command(remote_fd, buff, 250,protocol_end) != COMMAND_OK){
    return FAIL;
  }
  protocol_end = wind_proto(protocol_end);

  snprintf(buff, 1024, "DATA");
  if(try_command(remote_fd, buff, 354, protocol_end) != COMMAND_OK){
    return FAIL;
  }
  protocol_end = wind_proto(protocol_end);

  line = data->data;
  while(line != NULL){
    DEBUG_CLNT_S("Write Data to Remote", line->data);
    snprintf(buff, 1024, "%s\r\n", line->data);
    if (write_remote(remote_fd, buff, strlen(buff), protocol_end) <= 0){
      ERROR_SYS("Writing on Remote Socket");
      DEBUG_CLNT("Error while Writing on remote socket");
      return COMMAND_WRITE;
    }
    line = line->next;
  }
  protocol_end = wind_proto(protocol_end);
   
  snprintf(buff, 1024, ".");
  if(try_command(remote_fd, buff, 250, protocol_end) != COMMAND_OK){
    return FAIL;
  }
  protocol_end = wind_proto(protocol_end);


  snprintf(buff, 1024, "QUIT");
  if(try_command(remote_fd, buff, 221, protocol_end) != COMMAND_OK){
    return FAIL;
  }
  protocol_end = wind_proto(protocol_end);
  
  protocol_end->next = new_proto_entry("-----------------------------------------");
  protocol_end = protocol_end->next;

  return OK;
}

int forward_mail(int fd, mail_data_t* data){
  int remote_fd;
  data_line_t *protocol;

      remote_fd = create_remote_conn(app->remote_addr, app->remote_port);
      if(send_mail(fd, remote_fd, data, &protocol) == OK){
	put_forward_proto(fd, 250, protocol);
      } else {
        put_forward_proto(fd, 451, protocol);
      }

      quit_remote_conn(remote_fd);
      free_protocol(protocol);
      return OK;
}
