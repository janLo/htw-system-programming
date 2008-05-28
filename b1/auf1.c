/* Systemprogrammierung
 * Beleg 1, Aufgabe 1
 * "Ausgabe der Dateigröße einer Datei"
 *
 * von: Jan Losinski
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* Reads a Filename from the commandline and,
 * stat() it and print the site in Bytes */
int main(int argc, char *argv[]){
  int i = 0;
  struct stat s;
  if (argc < 2){
    fprintf(stderr, "Keine Datei(en) angegeben!\n");
    exit(1);
  }
  for(i = 1; i < argc; i++){
    if(stat(argv[i], &s) == 0){
      printf("Datei '%s' \t.. Größe: %d Bytes\n", argv[i], s.st_size); 
    } else {
      fprintf(stderr, "Fehler bei Datei '%s': ", argv[i]);
      perror("");
    }
  }
  exit(0);
}
