#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


#define BUFFLEN 1024

void ext_fail(char *type, char *name){
  fprintf(stderr, "Fehler bei %sdatei '%s': ", type, name);
  perror("");
  exit(1);
}

int main(int argc, char *argv[]){
  int i = 0;
  struct stat s_s, s_d;
  int fd_s, fd_d;
  char buff[BUFFLEN];
  if (argc != 3){
    fprintf(stderr, "Syntax: auf2 Quelldatei Zieldatei\n");
    exit(1);
  }
  if(stat(argv[1], &s_s) != 0){
    ext_fail("Quell", argv[1]);
  }
  if((fd_s = open(argv[1], O_RDONLY)) < 0){
    ext_fail("Quell", argv[1]);
  }


  if(stat(argv[2], &s_d) == 0){
     printf("Datei '%s' existiert bereits .. überschreiben? (y/N) ", argv[2]);
     if(getchar() != 'y'){
       printf("Überschreibe NICHT!\nBeende..\n");
       exit(0);
     }
     printf("ÜBERSCHREIBE!\n");
  }
  if((fd_d = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, s_s.st_mode)) < 0){
    ext_fail("Ziel", argv[2]);
  }
  
  while((i = read(fd_s, buff, BUFFLEN)) > 0){
    if(write(fd_d, buff, i) < 0){
      ext_fail("Ziel", argv[2]);
    }
  }
  close(fd_s);
  close(fd_d);
      
  if(i == 0){
    printf("Datei '%s' erfolgreich nach '%s' kopiert\n", argv[1], argv[2]);
  } else {
    ext_fail("Quell", argv[1]);
  }

  exit(0);
}

