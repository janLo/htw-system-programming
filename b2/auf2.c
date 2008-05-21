#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <pwd.h>
#include <asm/param.h>
#include <time.h>
#include <sys/sysmacros.h>


#ifdef USE_PS
  #define COMMAND "ps ho pid,tty,user,comm,%cpu,lstart,args -C "
#endif

typedef struct pinfo_list {
  int pid;
  char *tty;
  char* executable;
  char *user;
  char *cpu;
  char *time;
  char* params;
  struct pinfo_list *next;
} pinfo_list_t;

#ifndef USE_PS
unsigned long get_hz(){
  unsigned long Hertz = 0;
#ifdef HZ
  Hertz = (unsigned long long) HZ;
#else
  // Assume Hertz is 100, if not: shit happens .. stupid api
  Hertz = 100;
#endif
  return Hertz;
}

// tty-translation tables stolen from libproc, because
// of less documentation about all the different terminal-types

// major 204 is a mess -- "Low-density serial ports"
static const char low_density_names[][4] = {
"LU0",  "LU1",  "LU2",  "LU3",
"FB0",
"SA0",  "SA1",  "SA2",
"SC0",  "SC1",  "SC2",  "SC3",
"FW0",  "FW1",  "FW2",  "FW3",
"AM0",  "AM1",  "AM2",  "AM3",  "AM4",  "AM5",  "AM6",  "AM7",
"AM8",  "AM9",  "AM10", "AM11", "AM12", "AM13", "AM14", "AM15",
"DB0",  "DB1",  "DB2",  "DB3",  "DB4",  "DB5",  "DB6",  "DB7",
"SG0",
"SMX0",  "SMX1",  "SMX2",
"MM0",  "MM1",
};

int translate_tty(long long dev, char *buf){
  
  int maj = major(dev);
  int min = minor(dev);
  struct stat sbuf;
  int t0, t1;
  unsigned tmpmin = min;

  switch(maj){
  case   4:
    if(min<64){
      sprintf(buf, "/dev/tty%d", min);
      break;
    }
    if(min<128){  /* to 255 on newer systems */
      sprintf(buf, "/dev/ttyS%d", min-64);
      break;
    }
    tmpmin = min & 0x3f;  /* FALL THROUGH */
  case   3:      /* /dev/[pt]ty[p-za-o][0-9a-z] is 936 */
    if(tmpmin > 255) return 0;   // should never happen; array index protection
    t0 = "pqrstuvwxyzabcde"[tmpmin>>4];
    t1 = "0123456789abcdef"[tmpmin&0x0f];
    sprintf(buf, "/dev/tty%c%c", t0, t1);
    break;
  case  11:  sprintf(buf, "/dev/ttyB%d",  min); break;
  case  17:  sprintf(buf, "/dev/ttyH%d",  min); break;
  case  19:  sprintf(buf, "/dev/ttyC%d",  min); break;
  case  22:  sprintf(buf, "/dev/ttyD%d",  min); break; /* devices.txt */
  case  23:  sprintf(buf, "/dev/ttyD%d",  min); break; /* driver code */
  case  24:  sprintf(buf, "/dev/ttyE%d",  min); break;
  case  32:  sprintf(buf, "/dev/ttyX%d",  min); break;
  case  43:  sprintf(buf, "/dev/ttyI%d",  min); break;
  case  46:  sprintf(buf, "/dev/ttyR%d",  min); break;
  case  48:  sprintf(buf, "/dev/ttyL%d",  min); break;
  case  57:  sprintf(buf, "/dev/ttyP%d",  min); break;
  case  71:  sprintf(buf, "/dev/ttyF%d",  min); break;
  case  75:  sprintf(buf, "/dev/ttyW%d",  min); break;
  case  78:  sprintf(buf, "/dev/ttyM%d",  min); break; /* conflict */
  case 105:  sprintf(buf, "/dev/ttyV%d",  min); break;
  case 112:  sprintf(buf, "/dev/ttyM%d",  min); break; /* conflict */
  /* 136 ... 143 are /dev/pts/0, /dev/pts/1, /dev/pts/2 ... */
  case 136 ... 143:  sprintf(buf, "/dev/pts/%d",  min+(maj-136)*256); break;
  case 148:  sprintf(buf, "/dev/ttyT%d",  min); break;
  case 154:  sprintf(buf, "/dev/ttySR%d", min); break;
  case 156:  sprintf(buf, "/dev/ttySR%d", min+256); break;
  case 164:  sprintf(buf, "/dev/ttyCH%d",  min); break;
  case 166:  sprintf(buf, "/dev/ttyACM%d", min); break; /* bummer, 9-char */
  case 172:  sprintf(buf, "/dev/ttyMX%d",  min); break;
  case 174:  sprintf(buf, "/dev/ttySI%d",  min); break;
  case 188:  sprintf(buf, "/dev/ttyUSB%d", min); break; /* bummer, 9-char */
  case 204:
    if(min >= sizeof low_density_names / sizeof low_density_names[0]) return 0;
    sprintf(buf, "/dev/tty%s",  low_density_names[min]);
    break;
  case 208:  sprintf(buf, "/dev/ttyU%d",  min); break;
  case 216:  sprintf(buf, "/dev/ttyUB%d",  min); break;
  case 224:  sprintf(buf, "/dev/ttyY%d",  min); break;
  case 227:  sprintf(buf, "/dev/3270/tty%d", min); break; /* bummer, HUGE */
  case 229:  sprintf(buf, "/dev/iseries/vtty%d",  min); break; /* bummer, HUGE */
  default: return 0;
  }
  if(stat(buf, &sbuf) < 0) return 0;
  if(min != minor(sbuf.st_rdev)) return 0;
  if(maj != major(sbuf.st_rdev)) return 0;
  return 1;

}


pinfo_list_t *get_process_info(const char* process_name){

  struct stat s1, s2;
  char dir_buff[256];
  char stat_buff[256];
  char cmdline_buff[256];
  char buff[1024];
  int pid, i, jiffies;
  time_t tim, uptime, since_boot;
  char * pos1, * pos2, *walk, *cmd, *end;
  pinfo_list_t * ret = NULL, *tmp2 = NULL, *tmp1;
  DIR * dir;
  struct dirent *de;
  int stat_fd, cmd_fd, up_fd;
  struct passwd *pwd;
  unsigned long Hertz = get_hz();

  strcpy(dir_buff, "/proc/");
  strcpy(stat_buff, "/proc/");
  strcpy(cmdline_buff, "/proc/");

  // open /proc
  if((dir = opendir("/proc/")) == NULL){
    fprintf(stderr, "Fehler, /proc nicht lesbar\n");
    exit(0);
  }

  //iterate oder dirents in /proc
  while((de = readdir(dir)) != NULL){

    //build full dirent-path
    strcpy(&dir_buff[6], de->d_name); 
    
    //stat dirent
    if(lstat(dir_buff, &s1) == 0){

      //test if dirent is a dir and not . or ..
      if (S_ISDIR(s1.st_mode) && (strcmp(de->d_name, ".") != 0) && (strcmp(de->d_name, "..") != 0)){

	//build paths for /proc/PID/stat and /proc/PID/cmdline
	strcpy(&stat_buff[6]   , de->d_name); 
	strcpy(&cmdline_buff[6], de->d_name); 

	pos1 = strchr(stat_buff,'\0');
	*pos1 = '/';
	pos1++;
	pos2 = strchr(cmdline_buff, '\0');
	*pos2 = '/';
	pos2++;

	strcpy(pos1, "stat");
	strcpy(pos2, "cmdline");

	//test if dirent contain stat and cmdline and if so, assume that its a process-dir
	if(lstat(stat_buff, &s1) == 0 && lstat(cmdline_buff,&s2) == 0){

	  //get the pid from the dir name
	  pid = atoi(de->d_name);

	  //open /proc/PID/stat
	  if((stat_fd = open(stat_buff, O_RDONLY)) < 0){
	    continue;
	  }

	  //read /proc/PID/stat
	  if((i = read(stat_fd, buff, 1024)) < 1){
	    close(stat_fd);
	    continue;
	  }
	  buff[i-1] = '\0';

	  //search for process-name
	  walk = strchr(buff, '(');
	  walk++;
	  cmd = walk;
	  walk = strchr(cmd, ')');
	  *walk = '\0';
	  walk++; walk++;

	  //test if process-name matches the given search string
	  if (strncmp(cmd, process_name, 16) == 0){

	    //allocate a new pinfo_list_t object
	    tmp1 = malloc(sizeof(pinfo_list_t));

	    //set its pid
            tmp1->pid = pid;
	    
	    //set the process-name
	    tmp1->executable = malloc(sizeof(char)*(strlen(cmd)+2));
	    strcmp(tmp1->executable, cmd);

	    //search for tty-num
	    for(i = 0; i < 4; i++){
	      walk = strchr(walk, ' ');
	      walk++;
	    }
            cmd = walk;
	    walk = strchr(walk, ' ');
	    *walk = '\0';
	    walk++;
	    tmp1->tty = malloc(sizeof(char)*(strlen(cmd)+10));
	    i = atoi(cmd);
	    if(translate_tty(i, tmp1->tty) == 0){
	      strcpy(tmp1->tty, "?");
	    }
	    //printf("%d: %d, %d\n", tmp1->pid, major(i), minor(i));
            
	    
            //get the username from the owner pf the process-dir
	    if ((pwd = getpwuid(s1.st_uid)) != NULL){
	      tmp1->user = malloc(sizeof(char)*(strlen(pwd->pw_name)+2));
	      strcpy(tmp1->user, pwd->pw_name);
	    }

	    //get used jiffies (tics) usesd by the process
	    for(i = 0; i < 6; i++){
	      walk = strchr(walk, ' ');
	      walk++;
	    }
	    cmd = walk;
	    walk = strchr(walk, ' ');
	    *walk = '\0';
	    walk++;
	    jiffies = atoi(cmd);

	    cmd = walk;
	    walk = strchr(walk, ' ');
	    *walk = '\0';
	    walk++;
	    jiffies += atoi(cmd);

            //get time the process started since boot-time 
	    for(i = 0; i < 6; i++){
	      walk = strchr(walk, ' ');
	      walk++;
	    }

	    cmd = walk;
	    walk = strchr(walk, ' ');
	    *walk = '\0';
	    walk++;

	    since_boot = atoll(cmd);

	    //read uptime from /proc/uptime
            if((up_fd = open("/proc/uptime", O_RDONLY)) > 0){
	      if((i = read(up_fd, buff, 1024)) > 0){
		*(strchr(buff, '.')) = ' ';
		uptime=atoll(buff);
	      }
	    }

	    //calc time (unix epoch) the process started
	    tim = time(NULL) - (uptime -  ( (since_boot - (since_boot%Hertz) ) / Hertz));

	    //calc % of CPU the process used recently (whatever recently means)
	    i = (jiffies * 1000ULL / Hertz) / (uptime - ( (since_boot - (since_boot%Hertz) ) / Hertz) );
	    if (i > 999){
	      i = 999;
	    }
	    
	    //build and set %cpu - string
	    tmp1->cpu = malloc(sizeof(char)*6);
            snprintf(tmp1->cpu, 5, "%d.%d", i/10, i%10);
	    
	    //build and set start-time-string
	    cmd = ctime(&tim);
	    if((walk = strchr(cmd, '\n')) != NULL){
	      *walk = '\0';
	    }
	    tmp1->time = malloc(sizeof(char) * (strlen(cmd) + 2));
	    strcpy(tmp1->time, cmd);

	    //read /proc/PID/cmdline
	    if((cmd_fd = open(cmdline_buff, O_RDONLY)) > 0){
	      if((i = read(cmd_fd, buff, 1024)) > 0){
		
		//replace '\0' through ' '
                end = buff+(i-1);
	        while((walk = strchr(buff, '\0')) != NULL){
		  if (walk != end){
		    *walk = ' ';
		  } else {
		    break;
		  }
		}

		//set the String
		tmp1->params = malloc(sizeof(char)* (strlen(buff) + 1));
		strcpy(tmp1->params, buff);
	      } else {
		
		//if /proc/PID/cmdline is empty set the process-name
                tmp1->params = tmp1->executable;
	      }
	      close(cmd_fd);
	    } else {

	      //if /proc/PID/cmdline is not readable set the process-name
              tmp1->params = tmp1->executable;
	    }
	    tmp1->next = NULL;

	    //build the list
	    if(tmp2 != NULL){
	      tmp2->next = tmp1;
	    }
	    tmp2 = tmp1;
	    if(ret == NULL){
	      ret = tmp1;
	    }

	  }
	  close(stat_fd);

	}
      }
    }
  }
  
  return ret;
  
}
#endif


#ifdef USE_PS
pinfo_list_t *get_process_info(const char* process_name){
  pinfo_list_t *ret = NULL, *tmp1 = NULL, *tmp2;
  FILE *pipe;
  char buff[1024], *tok, *buff2;;
  int i;
  char command[512];

  //Build the ps command
  strcpy(command, COMMAND);
  strcpy(&command[strlen(command)], process_name);
  
  //execute the command and open a pipe to it
  if((pipe = popen(command, "r")) == NULL){
    fprintf(stderr, "Fehler bei Aufruf: %s", command);
    perror("");
    exit(1);
  }

  //iterate over lines from pipe
  while (fgets(buff, 1024, pipe) != NULL){

    //write the readed buffer on a new place in memory, so we
    //don't have to malloc and strcpy all the time
    buff2 = malloc(sizeof(char)*(strlen(buff)+2));
    strcpy(buff2, buff);

    //Allocate a new pinfo_list_t object
    tmp2 = malloc(sizeof(pinfo_list_t));
    
    //replace newline with string-end
    *(strchr(buff2,'\n')) = '\0';
    
    //get pid token
    tok = strtok(buff2, " ");
    tmp2->pid = atoi(tok);

    //get tty token
    tok = strtok(NULL, " ");
    tmp2->tty = tok;

    //get user token
    tok = strtok(NULL, " ");
    tmp2->user = tok;

    //get exec token
    tok = strtok(NULL, " ");
    tmp2->executable = tok;

    //get %cpu token
    tok = strtok(NULL, " ");
    tmp2->cpu = tok;

    //get time tokens (more than one spaces in it
    tok = strtok(NULL, " ");
    tmp2->time = tok;
    for(i = 0; i < 4; i ++){
      *(tok+strlen(tok)) = ' ';
      tok = strtok(NULL, " ");
    }
    
    //get params tookens (also more than one spaces)
    tok = strtok(NULL, " ");
    tmp2->params = tok;

    do {
      *(tok+strlen(tok)) = ' ';
    } while (tok = strtok(NULL, " "));
    
    tmp2->next = NULL;

    //build the list
    if(tmp1 != NULL){
      tmp1->next = tmp2;
    }
    tmp1 = tmp2;
    if(ret == NULL){
      ret = tmp2;
    }
  }
  return ret;
}
#endif

int select_pid(pinfo_list_t *processes, const char* search){
  int i = 0, sel, j;
  pinfo_list_t * walker = processes;

  //print each process details
  if (walker != NULL){
    printf("Gefundene Prozese:\n");
    do {
      printf("%d:  %s\n",i, walker->params);
      printf("     \\_ PID   = %d\n", walker->pid);
      printf("     \\_ START = %s\n", walker->time);
      printf("     \\_ %%CPU  = %s\n", walker->cpu);
      printf("     \\_ TTY   = %s\n", walker->tty);
      printf("     \\_ USER  = %s\n", walker->user);
      printf("\n");
      i++;
    } while ((walker = walker->next) != NULL);
    printf("Bitte einen Prozess (0-%d) auswählen: ", i-1);

    //select process to kill
    scanf("%d", &sel);
    if(sel < 0 || sel >= i){
      printf("%d ist eine ungültige Eingabe, Beende.\n", sel);
      exit(1);
    }

    //get process-id to kill
    walker = processes;
    for (j = 0; j < sel; j++){
      walker = walker->next;
    }
    printf("Beende Prozess Nr. %d\n", walker->pid);
    return walker->pid;

  } else {
    printf("Kein Prozess '%s' gefunden\n", search);
    exit(0);
  }
}


int main(int argc, char *argv[]){

  int signal = 15, i, pid;
  pinfo_list_t * list;

#ifdef USE_PS
  printf("Beleg 2 Aufgabe 2a: 'zap' mit ps\n");
#else
    printf("Beleg 2 Aufgabe 2b: 'zap' mit /proc\n");
#endif
  
  if (argc < 2 || argc > 3){
    fprintf(stderr, "Usage: %s processname! [signal]\n", argv[0]);
    exit(1);
  }
  if (argc == 3){
    i = atoi(argv[2]);
    if (i > -1 && i < 16 ){
      signal = i;
    }
  }

  //get processes
  list = get_process_info(argv[1]);

  //select process
  pid = select_pid(list, argv[1]);

  //kill process
  kill(pid, signal);
  exit(0);
}
