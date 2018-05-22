#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>

int getInput(char* buffer);
void flush();
void inputParser(char* input, char** parsedArguments);
int changeDirectory(char * path);

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
  if (fgets(buffer, sizeof buffer, stdin) != NULL) {
  	//shaving off excess newline and replacing for a endline char /0
	  size_t len = strlen(buffer);
	  if (len > 0 && buffer[len-1] == '\n') {
	    buffer[--len] = '\0';
	  }
	}
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
//parse the input we got seperating by spaces
void inputParser(char* input, char** parsedArguments){
  int i;
  for (i = 0; i < 512; i++) {
    parsedArguments[i] = strsep(&input, " ");
  }
}

int changeDirectory(char *pth){
  char path[2048];
    strcpy(path,pth);

    char cwd[2048];
   
    //if cd was passed in as the path variable then go to the home directory instead
    if (strcmp(path, "cd") == 0)
    {
      //https://stackoverflow.com/questions/9493234/chdir-to-home-directory
      chdir(getenv("HOME"));
    }
    //https://lonelycoding.com/implementing-cd-system-call-using-c-if-condition/
    //i used this resource to help navigate using strcat and cwd
    else if(pth[0] != '/')
    {// true for the dir in cwd
        getcwd(cwd,sizeof(cwd));
        strcat(cwd,"/");
        strcat(cwd,path);
        chdir(cwd);
    }else{//true for dir w.r.t. /
        chdir(pth);
    }

    return 0;
}

int main(int argc, char const *argv[]) {
  //char array for input and parsed arguments derived from input
  char inputbuffer[2048];
  char* parsedArguments[512];
  int currentStatus;
    

  currentStatus = 0;
  //infinite loop!
    while (1) {

      printf(":");
      flush();
      //if the input has no errors continue on through the loop
      if (getInput(inputbuffer)) {
        continue;
      }
      inputParser(inputbuffer, parsedArguments);
      printf("%s\n", parsedArguments[0]);
      // a giant if else statement to detect if we're using in house commands before moving on to 
      //creating children

      //EXIT catch```````````````````````````````````````````````````````````````
      if (strcmp(parsedArguments[0], "exit") == 0){
       	
        exit(0);
       } 
      // CD catch``````````````````````````````````````````````````````
      else if (strcmp(parsedArguments[0], "cd") == 0){
        //if there is no path, pass in cd as path
        if (parsedArguments[1]  == NULL){
          changeDirectory(parsedArguments[0]);
          }  
        else{
          //else just pass in the path
          changeDirectory(parsedArguments[1]);
        }
      }// STATUS catch ```````````````````````````````````````````````````
      else if (strcmp(parsedArguments[0], "status") == 0){
      	
        printf("%d \n", currentStatus );
      }
      else{

      }


    }
  exit(0);
}
