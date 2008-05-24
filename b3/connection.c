#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "connection.h"
#include "session.h"
#include "smtprelay.h"

#define MAX_WAIT_CONN 25

typedef struct client_thread_data {
  int client_sock;
  struct sockaddr_in client_addr;
} client_thread_data_t;




struct sockaddr_in serv_addr;  // Server und Clientaddresse
int serv_sock = -1;
char * server_addr_string = NULL;


static int create_socket(){
  int serv_sock;

  if((serv_sock = socket(PF_INET, SOCK_STREAM, 0)) == -1){
    ERROR_SYS("socket creation");
    return -1;
  }
  DEBUG_N("Socket Created. fd", serv_sock);

  if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
    ERROR_SYS("socket binding");
    quit_conn(serv_sock);
    return -1;
  }
  DEBUG("Socket bound");

  if(listen(serv_sock, MAX_WAIT_CONN) == -1){
    ERROR_SYS("socket listening");
    quit_conn(serv_sock);
    return -1;
  }
  DEBUG("Listen on Socket");

  return serv_sock;
}

static void* process_client(void *client_data){
  client_thread_data_t * data = (client_thread_data_t*) client_data;
  int sock = data->client_sock;
  
  DEBUG_CLNT("Thread created");

  start_session(sock);

  shutdown(sock,2);
  quit_conn(sock);
  DEBUG_CLNT("Client Socket Closed");

  free(data);

  DEBUG_CLNT("Quit Thread");
  return NULL;
}

static int wait_connect(int server_sock){
  int client_sock, addr_len;
  struct sockaddr_in client_addr;
  client_thread_data_t *client_data = NULL;
  pthread_t tid;

  while(1){

    addr_len = sizeof(client_addr);

    DEBUG("Wait for Client");

    if((client_sock = accept(server_sock, (struct sockaddr*)&client_addr, (unsigned int*)&addr_len)) == -1){
      ERROR_SYS("socket accept");
    }
    DEBUG("Client Accepted");

    if((client_data = (client_thread_data_t*)malloc(sizeof(client_thread_data_t))) == NULL){
      ERROR_SYS("alloc mem for client data");
      return 0;
    }
    client_data->client_sock = client_sock;
    client_data->client_addr = client_addr;

    if(pthread_create(&tid, NULL, process_client, client_data) != 0){
      ERROR_SYS("create thread");
      quit_conn(client_sock);
    }
    pthread_detach(tid);
  }
}


int create_conn(char *addr, char *port){


  server_addr_string = addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_port = htons(atoi(port));
  serv_addr.sin_addr.s_addr = inet_addr(addr);
  serv_addr.sin_family = AF_INET;

  if ((serv_sock = create_socket()) == -1){
    return 1;
  }

  wait_connect(serv_sock); 

  quit_conn(serv_sock);

  return 0;

}

void quit_conn(int fd){
  if(close(fd) != 0){
    ERROR_SYS("Close Conn");
    DEBUG_CLNT("Close Connection");
  }
}

int create_remote_conn(char *addr, char *port){
  int remote_sock;

  struct sockaddr_in remote_addr;

  memset(&remote_addr, 0, sizeof(remote_addr));
  remote_addr.sin_port = htons(atoi(port));
  remote_addr.sin_addr.s_addr = inet_addr(addr);
  remote_addr.sin_family = AF_INET;

  if((remote_sock = socket(PF_INET, SOCK_STREAM, 0)) == -1){
    ERROR_SYS("remote socket creation");
    return -1;
  }
  DEBUG_CLNT_N("Remote Socket Created. fd", remote_sock);

  if(connect(remote_sock, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) == -1){
    ERROR_SYS("Remote sockte Connecting");
    close(remote_sock);
    return -1;
  }
  DEBUG_CLNT("Remote Socket connected");

  return remote_sock;
}

void quit_remote_conn(int fd){
  if(close(fd) != 0){
    ERROR_SYS("Close remote Conn");
    DEBUG_CLNT("Close Remote Connection");
  }
}

void sig_abrt_conn(int signr){
#ifdef IS_DEBUG
  switch (signr){
    case SIGABRT:
      DEBUG("Got SIGABRT Interupt!\n");
    case SIGTERM:
      DEBUG("Got SIGTERM Interupt!\n");
    case SIGQUIT:
      DEBUG("Got SIGQUIT Interupt!\n");
    case SIGINT:
      DEBUG("Got SIGINT Interupt\n");
  }
#endif
  if(serv_sock != -1){
//    shutdown(serv_sock,2);
    quit_conn(serv_sock);
  }
#ifdef USE_DMALLOC
  dmalloc_shutdown();
#endif
  exit(0);
}


