#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include <fcntl.h>

int getInput(char* buffer);
void flush();
void inputParser(char* input, char** parsedArguments);
int changeDirectory(char * path);
int ArgCount(char **parsedArguments);
int execArgs(char** parsedArguments, int *currentStatus, pid_t* childPID,size_t numberOfArgs);
int redirChecker(char ** parsedArguments, size_t numberOfArgs, pid_t pid);
int ampersandCheck(char **parsedArguments, size_t numberOfArgs);
void shiftArgs(char **parsedArguments, int index);


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
//goes through the parsed args array and counts them up and returns the index of the last arg
//useful for saving resources not looping through 512 args everytime.
int ArgCount(char **parsedArguments){
  for (int i = 0; i < 512; ++i){
    //also trims off empty args "" added to the end if there are extra spaces
    if (parsedArguments[i] == NULL || strcmp(parsedArguments[i], "") == 0) {
      return (i - 1);
      break;
    }
  }
  return 0;
}

//child forking from lectures and using execvp
int execArgs(char** parsedArguments, int *currentStatus, pid_t *childPID, size_t numberOfArgs)
{
    //forking from lecture
    pid_t pid = fork();
    *childPID = pid; 

    int ampersandBool = ampersandCheck(parsedArguments, numberOfArgs);
    if (ampersandBool == 0 && pid == 0){

      redirChecker(parsedArguments, numberOfArgs, pid);
    }
    else if (ampersandBool == 1 && pid == 0)
    {
      int null = open("/dev/null",0);
      dup2(null, STDIN_FILENO);
      dup2(null, STDOUT_FILENO);
      redirChecker(parsedArguments, numberOfArgs, pid);
    }
    flush();
    //chekcing to see if its the parent
    if (pid != 0 && pid != -1)
  {
    //checks if there's ampersands, will either wait or make it a bg process
    if (ampersandBool == 0)
    {
      waitpid(pid, currentStatus, 0);
    }
    else if (ampersandBool == 1)
    {
      waitpid(pid, currentStatus, WNOHANG);
      printf("Background process:%d has exited with a status of %d \n",pid , *currentStatus );
    }
  
     
  }

   return 0;
}

void valueShift(char** parsedArguments, size_t numberOfArgs,int index){
  
  for (int i = index; i < numberOfArgs ; ++i)
  {
    parsedArguments[i] = parsedArguments[i + 1];
  }
  parsedArguments[numberOfArgs] = NULL;
  for (int i = 0; i < numberOfArgs + 1; ++i)
  {
    puts(parsedArguments[i]);
  }

}
int redirChecker(char** parsedArguments, size_t numberOfArgs, pid_t pid){
  int redirBool = 1;

    if (pid == -1) {
        printf("\nFailed child forking.");
        flush();
        return 1;
    } 
    //if statement for detecting > and < 
    // if redir bool is not set then execute without using dup2
    else if (pid == 0) {
      for (int i = 0; i < numberOfArgs + 1; ++i){
        fflush(0);
        //for each iteration see if the command matches > or <,
        //if it does change the output/input accordingly,shift the value down and reset the counter

        if (strcmp(parsedArguments[i], ">") == 0)
        {
          //set the redir bool to let the method know we've redirected 
        
         int fd = open(parsedArguments[i+1], O_CREAT|O_WRONLY);
         //close off normal stdout
         close(STDOUT_FILENO);
         //redirect stdout into the file name of arg[i+1], since i == >
         dup2(fd, STDOUT_FILENO);
         close(fd);
         //set these null as they are no longer needed
         
         valueShift(parsedArguments, numberOfArgs, i);
         numberOfArgs --;
         valueShift(parsedArguments, numberOfArgs, i);
         numberOfArgs --;
         
        redirBool = 0;
        
        i = 0;
        }
        if (strcmp(parsedArguments[i], "<") == 0)
        {
         
         int tfd = open(parsedArguments[i + 1],O_RDONLY);
         close(STDIN_FILENO);
         dup2(tfd, STDIN_FILENO);
         close(tfd);
         //set these null as they are no longer needed
         //and shift the values down
         valueShift(parsedArguments, numberOfArgs, i);
         numberOfArgs --;
         valueShift(parsedArguments, numberOfArgs, i);
         numberOfArgs--;
         //set the redir bool flag
         redirBool = 0;
         //set i back to 0 to loop over the freshly moved vars
        i = 0;
        
      }
    
  }

  //couldnt get it working properly without seperating these two 
  if (redirBool == 0)
  {
   execvp(parsedArguments[0], parsedArguments);
  }
  if (redirBool == 1)
  {
   execvp(parsedArguments[0], parsedArguments);
  }
  //exit for the children
  exit(0);
  
  }
return 1;
}

//function to check the butt of the string for an ampersand
int ampersandCheck(char **parsedArguments, size_t numberOfArgs){
  for (int i = numberOfArgs; i < numberOfArgs + 1; ++i)
  {
    if (strcmp(parsedArguments[i], "&") == 0)
    {
      //set it to null so it doesnt get passed to exec
      parsedArguments[i] = NULL;
      return 1;
    }
  }
  return 0;
}


int main(int argc, char const *argv[]) {
  //char array for input and parsed arguments derived from input
  char inputbuffer[2048];
  char* parsedArguments[512];
  pid_t childPID = 0;
  int currentStatus = 0;

  size_t numberOfArgs;


    
  //infinite loop!
  while (1) {

      printf(": ");
      flush();
      //if the input has no errors continue on through the loop
      if (getInput(inputbuffer)) {
        continue;
      }
      inputParser(inputbuffer, parsedArguments);
      numberOfArgs = ArgCount(parsedArguments);
      //set the end of the args to null to prevent segfaults
      parsedArguments[numberOfArgs + 1] = NULL;

      // a giant if else statement to detect if we're using in house commands before moving on to 
      //creating children

      //EXIT catch```````````````````````````````````````````````````````````````
      if (strcmp(parsedArguments[0], "exit") == 0){
       
         exit(0);
       } 
      // CD catch``````````````````````````````````````````````````````
      else if (strcmp(parsedArguments[0], "cd") == 0){
        //if there is no path, pass in cd as path (gets handled in function as default action)
        if (parsedArguments[1]  == NULL){
          changeDirectory(parsedArguments[0]);
          }  
        else{
          //else just pass in the path
          changeDirectory(parsedArguments[1]);
        }
      }// STATUS catch ```````````````````````````````````````````````````
      else if (strcmp(parsedArguments[0], "status") == 0){
        printf("%d\n", currentStatus );
        flush();
      }
      else{
        //if there was an issue executing the command. set the status to 1
        if (execArgs(parsedArguments,&currentStatus,&childPID, numberOfArgs) == 1){
          currentStatus = 1;
        }
  
      }
  
    }
exit(0);
}
