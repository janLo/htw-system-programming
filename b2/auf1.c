#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

void listInode(char *file, int level){
  struct stat s,t;
  DIR *dir;
  struct dirent *de;
  char *space;
  int i, m = 0, j;
  char buffer[512];
  char space2[512];

  if (stat(file, &s)){
    fprintf(stderr, "Fehler bei Datei '%s': ", file);
    perror("");
    return;
  }
  if (!S_ISDIR(s.st_mode)){
    fprintf(stderr, "Fehler, Datei '%s' ist kein Verzeichnis!\n", file);
    return;
  }
  if((dir = opendir(file)) == NULL){
    fprintf(stderr, "Fehler, Verzeichnis '%s' ist nicht lesbar\n", file);
    return;
  }

  space = malloc(sizeof(char)*((level*4)+1));

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
  
  i = strlen(file);
  strcpy(buffer, file);
  buffer[i] = '/';

  for (j = 0; j < 100; j++){
    space2[j] = '.';
  }

  while((de = readdir(dir)) != NULL){
    if(strcmp(de->d_name, ".") && strcmp(de->d_name, "..")){
      space2[m] = '.';
      m = 60 - (strlen(space) + strlen(de->d_name));
      space2[m] = '\0';
      printf("%s%s: %s %d\n",space, de->d_name, space2,de->d_ino);//
      strcpy(&buffer[i+1], de->d_name);
      if(lstat(buffer, &t) == 0){
	//  printf("test\n");
	if (S_ISDIR(t.st_mode)){
	  listInode(buffer,level + 1);
	}
      }
    }
  }
  closedir(dir);
  free(space);
}

int main(int argc, char *argv[]){
  int i = 0;
  struct stat s;
  if (argc < 2){

  }
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
