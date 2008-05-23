#define MSG_GREET 	"%d %s SMTP Relay by Jan Losinski\r\n"
#define MSG_RESET 	"%d RESET Accepted, Resetted\r\n"
#define MSG_NOOP 	"%d NOOP Ok, I'm here\r\n"
#define MSG_BYE 	"%d Bye Bye.\r\n"
#define MSG_QUIT 	"%d Ok, I try to forward the Message:\r\n"
#define MSG_NOT_IMPL 	"%d %s Command not implemented\r\n"
#define MSG_SYNTAX 	"%d Syntax error or command unrecognized\r\n"
#define MSG_SYNTAX_ARG 	"%d syntax error in parameters or arguments\r\n"
#define MSG_SENDER 	"%d Sender %s OK\r\n"
#define MSG_HELLO 	"%d Hello %s, How are you?\r\n"
#define MSG_RCPT 	"%d RCPT %s seems to be OK\r\n"
#define MSG_DATA 	"%d Waiting for Data, End with <CR><LF>.<CR><LF>\r\n"
#define MSG_DATA_ACK 	"%d Message Accepted and forwarded\r\n"
#define MSG_DATA_FAIL 	"%d Message Accepted but forward failed\r\n"
#define MSG_MEM		"%d Requested mail action aborted: exceeded storage allocation\r\n"
#define MSG_SEQ		"%d Bad Sequence of Commands\r\n"
#define MSG_PROTO       "%d-Poto: %s\r\n"



typedef struct data_line{
  char *data;
  struct data_line *next;
} data_line_t;

typedef struct mail_data{
  char* client_addr;
  char* sender_mail;
  char* rcpt_mail;
  data_line_t * data;
} mail_data_t;

int write_client_msg(int fd, int status, const char *msg, char *add);
int start_session(int fd);
void put_forward_proto(int status, int fd, data_line_t *proto);
