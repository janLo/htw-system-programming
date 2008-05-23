extern char* server_addr_string;

int create_remote_conn(char *addr, char *port);
void quit_remote_conn(int fd);
int create_conn(char *addr, char *port);
void remote_sock(char* addr, char *port);
void quit_conn(int fd);
void quit_remote_conn(int fd);
void sig_abrt_conn(int signr);
