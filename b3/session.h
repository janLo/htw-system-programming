#define SMTP_MSG_GREET 		"%d %s SMTP Relay by Jan Losinski\r\n"
#define SMTP_MSG_RESET 		"%d RESET Accepted, Resetted\r\n"
#define SMTP_MSG_NOOP 		"%d NOOP Ok, I'm here\r\n"
#define SMTP_MSG_BYE 		"%d Bye Bye.\r\n"
#define SMTP_MSG_QUIT 		"%d Ok, I try to forward the Message:\r\n"
#define SMTP_MSG_NOT_IMPL 	"%d %s Command not implemented\r\n"
#define SMTP_MSG_SYNTAX 	"%d Syntax error or command unrecognized\r\n"
#define SMTP_MSG_SYNTAX_ARG 	"%d syntax error in parameters or arguments\r\n"
#define SMTP_MSG_SENDER 	"%d Sender %s OK\r\n"
#define SMTP_MSG_HELLO 		"%d Hello %s, How are you?\r\n"
#define SMTP_MSG_RCPT 		"%d RCPT %s seems to be OK\r\n"
#define SMTP_MSG_DATA 		"%d Waiting for Data, End with <CR><LF>.<CR><LF>\r\n"
#define SMTP_MSG_DATA_ACK 	"%d Message Accepted and forwarded\r\n"
#define SMTP_MSG_DATA_FAIL 	"%d Message Accepted but forward failed\r\n"
#define SMTP_MSG_MEM		"%d Requested mail action aborted: exceeded storage allocation\r\n"
#define SMTP_MSG_SEQ		"%d Bad Sequence of Commands\r\n"
#define SMTP_MSG_PROTO 		"%d-Poto: %s\r\n"

#define USER_MSG_GREET 		"SMTP Relay by Jan Losinski on %s\r\n"
#define USER_MSG_PROTO 		"Potokoll %s: %s\r\n"
#define USER_MSG_WHO_ASK	"Who are You? Tell me your hostname or type quit!\r\n"
#define USER_MSG_WHO_OK		"%s, %s seems to be a good hostname, I accept it.\r\n"
#define USER_MSG_WHO_FAIL	"%s, %s is not a giid Hostname in my opinion, please insert again or type quit\r\n"
#define USER_MSG_HELLO		"Hello %s, nice to see you\r\n"
#define USER_MSG_HELP_1		"Please input the Values I ask for and send with <CR><LF>\r\n"
#define USER_MSG_HELP_2		"You can Quit with \"quit\" or reset your input with \"reset\"\r\n"
#define USER_MSG_SENDER_ASK	"Please specify the sender of the Mail\r\n"
#define USER_MSG_SENDER_OK	"%s, %s seems to be a good sender\r\n"
#define USER_MSG_SENDER_FAIL	"%s, %s is not ok, please try again\r\n"
#define USER_MSG_RCPT_ASK	"Please specify the recipient of the Mail\r\n"
#define USER_MSG_RCPT_OK	"%s, %s seems ro be a good recipient\r\n"
#define USER_MSG_RCPT_FAIL	"%s, %s is bad, please try again\r\n"
#define USER_MSG_DATA_1		"Now input the Data of the Mail. This is a Block with multipple Lines\r\n"
#define USER_MSG_DATA_2		"Terminate the Data Block with <CR><LF>.<CR><LF>\r\n"
#define USER_MSG_READED		"%s lines Successfully readed\r\n"
#define USER_MSG_MEM		"hmn, not enough memory\r\n"
#define USER_MSG_CONFIRM_ASK	"Should I really send this Mail? If jey type \"send\", if not type \"reset\" or \"quit\"\r\n"
#define USER_MSG_CONFIRM_OK	"%s, %s readed\r\n"
#define USER_MSG_CONFIRM_FAIL	"%s, %s is not a valid input, try again\r\n"
#define USER_MSG_DATA_ACK	"%s, Message Acceptet\r\n"
#define USER_MSG_DATA_FAIL	"%s, Something went wrong while sending\r\n"
#define USER_MSG_QUIT		"Quit requested, Bye Bye.\r\n"
#define USER_MSG_RESET		"Reset all Session Inputs\r\n"



// State Vals for the Session
enum session {
  SESSION_RUN,
  SESSION_SEND,
  SESSION_QUIT,
  SESSION_RESET,
  SESSION_ABORT
};

// Return Vals for fetch_input_line
enum read {
  READ_OK,
  READ_RESET,
  READ_QUIT,
  READ_ERR,
  READ_MEM
};

// Return Vals for check_input
enum check {
  CHECK_OK,
  CHECK_ARG,
  CHECK_ARG_MSG,
  CHECK_DELIM,
  CHECK_PREF,
};

// Return Vals for the Argument checkers
enum check_arg{
  ARG_OK,
  ARG_BAD,
  ARG_BAD_MSG
};

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

int start_session(int fd);
void put_forward_proto(int status, int fd, data_line_t *proto);


int check_addr(char * addr);
int check_mail(char * addr);
int read_data(int fd, data_line_t **data_head);
