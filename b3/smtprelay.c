#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>

#define IS_DEBUG

#define DEFAULT_BINDADDR "127.0.0.1"
#define DEFAULT_BINDPORT "2345"
#define MAX_WAIT_CONN 25

#ifdef IS_DEBUG
  #define DEBUG(x) fprintf(stderr, "DEBUG: %s\n", x)
  #define DEBUG_N(x,y) fprintf(stderr, "DEBUG: %s=%d\n", x, y)
  #define DEBUG_S(x,y) fprintf(stderr, "DEBUG: %s=%s\n", x, y)
#else
  #define DEBUG
  #define DEBUG_N(x,y) 
  #define DEBUG_S(x,y)
#endif

typedef struct client_thread_data {
  int client_sock;
  struct sockaddr_in client_addr;
} client_thread_data_t;




struct sockaddr_in serv_addr;  // Server und Clientaddresse
int serv_sock = -1;


void put_err(char *src){
  if (src == NULL){
    fprintf(stderr, "ERROR: ");
  } else {
    fprintf(stderr, "ERROR: %s - ", src);
  }
  perror("");
}


int process_opt(int argc, char *argv[]){
  memset(&serv_addr, 0, sizeof(serv_addr));
  if (argc > 1) {
    //TODO Beispiel Seite 695
  } else {
    serv_addr.sin_port = htons(atoi(DEFAULT_BINDPORT));
    serv_addr.sin_addr.s_addr = inet_addr(DEFAULT_BINDADDR);
  }
  serv_addr.sin_family = AF_INET;
  return 0;
}

int create_socket(){
  int serv_sock, client_sock;

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

void* process_client(void *client_data){
  int sock = ((client_thread_data_t*)client_data)->client_sock;
  
  DEBUG("Process created");

  //TODO Hier geht es morgen weiter!
  sleep(10);

  close(sock);
  DEBUG("Client Socket Closed");

  free(client_data);
}

int wait_connect(int server_sock){
  int client_sock, addr_len;
  struct sockaddr_in client_addr;
  client_thread_data_t *client_data = NULL;
  pthread_t tid;

  while(1){

    addr_len = sizeof(client_addr);

    DEBUG("Wait for Client");

    if((client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len)) == -1){
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


static void exit_sig_handler(int signr){
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
    close(serv_sock);
  }
  exit(0);
}

int main(int argc, char *argv[]){
  if(signal(SIGABRT, exit_sig_handler) == SIG_ERR){
    put_err("Sighandler 'SIGABRT'");
  }
  if(signal(SIGTERM, exit_sig_handler) == SIG_ERR){
    put_err("Sighandler 'SIGTERM'");
  }
  if(signal(SIGQUIT, exit_sig_handler) == SIG_ERR){
    put_err("Sighandler 'SIGQUIT'");
  }
  if(signal(SIGINT, exit_sig_handler) == SIG_ERR){
    put_err("Sighandler 'SIGINT'");
  }
  
  if (process_opt(argc, argv)){
    exit(1);
  }
  if ((serv_sock = create_socket()) == -1){
    exit(1);
  }

  wait_connect(serv_sock); 

  close(serv_sock);
  return 0;
}


