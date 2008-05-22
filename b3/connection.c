#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "smtprelay.h"
#include "connection.h"

#define MAX_WAIT_CONN 25

typedef struct client_thread_data {
  int client_sock;
  struct sockaddr_in client_addr;
} client_thread_data_t;




struct sockaddr_in serv_addr;  // Server und Clientaddresse
int serv_sock = -1;


static int create_socket(){
  int serv_sock;

  if((serv_sock = socket(PF_INET, SOCK_STREAM, 0)) == -1){
    put_err("socket creation");
    return -1;
  }
  DEBUG_N("Socket Created. fd", serv_sock);

  if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
    put_err("socket binding");
    close(serv_sock);
    return -1;
  }
  DEBUG("Socket bound");

  if(listen(serv_sock, MAX_WAIT_CONN) == -1){
    put_err("socket listening");
    close(serv_sock);
    return -1;
  }
  DEBUG("Listen on Socket");

  return serv_sock;
}

static void* process_client(void *client_data){
  int sock = ((client_thread_data_t*)client_data)->client_sock;
  
  DEBUG_CLNT("Thread created");

  //write(
  //TODO Hier geht es morgen weiter!
  sleep(10);

  close(sock);
  DEBUG_CLNT("Client Socket Closed");

  free(client_data);

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
      put_err("socket accept");
    }
    DEBUG("Client Accepted");

    if((client_data = malloc(sizeof(client_thread_data_t))) == NULL){
      put_err("alloc mem for client data");
    }
    client_data->client_sock = client_sock;
    client_data->client_addr = client_addr;

    if(pthread_create(&tid, NULL, process_client, client_data) == -1){
      close(client_sock);
      free(client_data);
    }

  }
}


int create_conn(char *addr, char *port){

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_port = htons(atoi(port));
  serv_addr.sin_addr.s_addr = inet_addr(addr);
  serv_addr.sin_family = AF_INET;

  if ((serv_sock = create_socket()) == -1){
    return 1;
  }

  wait_connect(serv_sock); 

  close(serv_sock);

  return 0;

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
    close(serv_sock);
  }
  exit(0);
}
