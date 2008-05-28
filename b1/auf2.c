/* Systemprogrammierung
 * Beleg 1, Aufgabe 2
 * "Kopieren einer Datei"
 *
 * von: Jan Losinski
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


#define BUFFLEN 1024

// prints a errot to stderr and quit programm
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

  // Test argc
  if (argc != 3){
    fprintf(stderr, "Syntax: auf2 Quelldatei Zieldatei\n");
    exit(1);
  }

  // stat source file
  if(stat(argv[1], &s_s) != 0){
    ext_fail("Quell", argv[1]);
  }

  // open sourde file
  if((fd_s = open(argv[1], O_RDONLY)) < 0){
    ext_fail("Quell", argv[1]);
  }

  // stat destination file and ask to overwrite if exist
  if(stat(argv[2], &s_d) == 0){
     printf("Datei '%s' existiert bereits .. überschreiben? (y/N) ", argv[2]);
     if(getchar() != 'y'){
       printf("Überschreibe NICHT!\nBeende..\n");
       exit(0);
     }
     printf("ÜBERSCHREIBE!\n");
  }

  // open destination file
  if((fd_d = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, s_s.st_mode)) < 0){
    ext_fail("Ziel", argv[2]);
  }
  
  // read bytes from source and write them do destination
  while((i = read(fd_s, buff, BUFFLEN)) > 0){
    if(write(fd_d, buff, i) < 0){
      ext_fail("Ziel", argv[2]);
    }
  }

  // close both files
  close(fd_s);
  close(fd_d);
      
  // put a status text to the tty
  if(i == 0){
    printf("Datei '%s' erfolgreich nach '%s' kopiert\n", argv[1], argv[2]);
  } else {
    ext_fail("Quell", argv[1]);
  }

  exit(0);
}

