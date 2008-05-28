/* Systemprogrammierung
 * Beleg 2, Aufgabe 1
 * "Listen aller Inodes eines Verzeichnises"
 *
 * von: Jan Losinski
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

// lists the inode (called recursive)
void listInode(char *file, int level){
  struct stat s,t;
  DIR *dir;
  struct dirent *de;
  char *space;
  int i, m = 0, j;
  char buffer[512];
  char space2[512];

  // stats the dir file
  if (stat(file, &s)){
    fprintf(stderr, "Fehler bei Datei '%s': ", file);
    perror("");
    return;
  }

  // tests if is a directory
  if (!S_ISDIR(s.st_mode)){
    fprintf(stderr, "Fehler, Datei '%s' ist kein Verzeichnis!\n", file);
    return;
  }

  // opens dir
  if((dir = opendir(file)) == NULL){
    fprintf(stderr, "Fehler, Verzeichnis '%s' ist nicht lesbar\n", file);
    return;
  }

  // alloc memory for the printout
  space = malloc(sizeof(char)*((level*4)+1));

  // build the tree view
  for (i = 0; i < level; i++){
    space[ i*4] = ' ';
    space[(i*4)+1] = '|';
    space[(i*4)+2] = ' ';
    space[(i*4)+3] = ' ';
  }
  if (level > 0){
    space[(level*4)-2] = '_';
    space[(level*4)-3] = '\\';
  }
  space[level*4] = '\0';
  
  // copy the filename in the printout
  i = strlen(file);
  strcpy(buffer, file);
  buffer[i] = '/';

  // build a long "....." string
  for (j = 0; j < 100; j++){
    space2[j] = '.';
  }

  // reads the dirents
  while((de = readdir(dir)) != NULL){

    // prevent processing of "." ans ".."
    if(strcmp(de->d_name, ".") && strcmp(de->d_name, "..")){
      
      // cut ehe "..." string sown to the needed size
      space2[m] = '.';
      m = 60 - (strlen(space) + strlen(de->d_name));
      space2[m] = '\0';

      // print the filename and the inode
      printf("%s%s: %s %d\n",space, de->d_name, space2,de->d_ino);

      // tests if the current file is a dir and call listInode recursive if so
      strcpy(&buffer[i+1], de->d_name);
      if(lstat(buffer, &t) == 0){
	//  printf("test\n");
	if (S_ISDIR(t.st_mode)){
	  listInode(buffer,level + 1);
	}
      }
    }
  }
  // close the dir
  closedir(dir);

  // free the mem
  free(space);
}

int main(int argc, char *argv[]){
  int i = 0;
  struct stat s;
  char buff[257];

  // take current working dir
  if (argc < 2){
    getcwd(buff, 256);
    if(lstat(buff, &s) == 0){
      printf("Verz '%s'\n", buff);
      listInode(buff, 1);
      printf("\n");
    } else {
      fprintf(stderr, "Fehler bei Datei '%s': ", buff);
      perror("");
    }
  }

  // iterate over files from cmd
  for(i = 1; i < argc; i++){
    if(lstat(argv[i], &s) == 0){
      printf("Verz '%s'\n", argv[i]); 
      listInode(argv[i], 1);
      printf("\n");
    } else {
      fprintf(stderr, "Fehler bei Datei '%s': ", argv[i]);
      perror("");
    }
  }
  exit(0);
}
