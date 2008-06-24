
int smtp_start_session(int fd);
int smtp_write_client_msg(int fd, int status, const char *msg, char *add);
