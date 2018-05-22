#include <stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>

int getInput(char* buffer);
void flush();
void inputParser(char* input, char** parsedArguments);
//flushin all around
void flush(){
  fflush(stdin);
  fflush(stdout);
}
//input getter
int getInput(char* string){
  flush();
  char buffer [2048];
  //fill buffer up to its brim with input
  fgets(buffer, 2048, stdin);
  //if input is less than/ equal to 0 return an error
  //additionally if the first char is a comment throw dont allow passage
  //into the rest of the program
  //https://stackoverflow.com/questions/3845600/how-to-access-the-first-character-of-a-character-array?utm_medium=organic&utm_source=google_rich_qa&utm_campaign=google_rich_qa
  if (strlen(buffer) > 0 && buffer[0] != '#') {
    flush();
    strcpy(string, buffer);
    return 0;
  }
  flush();
  return 1;
}
//parse the input we got
void inputParser(char* input, char** parsedArguments){
  int i;
  for (i = 0; i < 512; i++) {
    parsedArguments[i] = strsep(&input, " ");
  }
}

int main(int argc, char const *argv[]) {
  //char array for input and parsed arguments derived from input
  char inputbuffer[2048];
  char* parsedArguments[512];
  //infinite loop!
    while (1) {
      printf(":");
      flush();
      if (getInput(inputbuffer)) {
        continue;
      }
      inputParser(inputbuffer, parsedArguments)

    }
  exit;
}
