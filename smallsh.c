#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include<sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

int getInput(char* buffer);
void flush();
char **inputParser(char* input);
int changeDirectory(char * path);
int ArgCount(char **parsedArguments);
int execArgs(char** parsedArguments, pid_t* childPID,int *numberOfArgs, struct sigaction *sa);
int redirChecker(char ** parsedArguments, int numberOfArgs, pid_t pid);
int ampersandCheck(char **parsedArguments, int *numberOfArgs);
void shiftArgs(char **parsedArguments, int index);
void checkBg(void);
void FG_handler(int signo);
void defaultSH(int signo);


//got tired of passing even more variables to the exec args functions
int bgProcesses[100];
int bgCount = 0;
int crtlZ = -1;
size_t len;
pid_t childPID = 0;
int currentStatus = 0;

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
	  len = strlen(buffer);
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
char ** inputParser(char* input){
     //set buffer to the size of the parsedArguemnts array
     int bufSize = 512;
     char *arg;
     char **argsArray = malloc(bufSize * sizeof(char*));
     int position = 0;
     

     arg = strtok(input, " ");

     while (arg != NULL)
     {

          argsArray[position] = arg;
          position++;
          //reset token
          arg = strtok(NULL, " ");
     }
     //set end of array to null to prevent seg faults later

     argsArray[position] = NULL;
     
     return argsArray;
}
// got the generic checkign code from 
//https://stackoverflow.com/questions/26683162/execute-background-process-and-check-status-from-parent
//then just implemented that method into a for loop to check all current bg processes for exits or signals.
void checkBg() {
  int currentStat;
  for(int i = 0; i < bgCount; i++) 
  { 
    if(waitpid(bgProcesses[i], &currentStat, WNOHANG) > 0)
    {
      //check if it was signaled to exit
      if(WIFSIGNALED(currentStat)) 
      {
        printf("\nChild %d exited with status: %d\n", bgProcesses[i], WTERMSIG(currentStat));
      }
      //check if it exited on its own
      if(WIFEXITED(currentStat)) 
      { 
        printf("\nChild %d exited with status: %d\n", bgProcesses[i], WEXITSTATUS(currentStat));
      }
    }
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
int execArgs(char** parsedArguments, pid_t *childPID, int *numberOfArgs, struct sigaction *sa)
{
  //forking from lecture
  pid_t pid = fork();

  int ampersandBool = ampersandCheck(parsedArguments, numberOfArgs);

  //sigstp pathways
  if (crtlZ == -1)
  {
    //continue normally

  }
  else if (crtlZ == 1)
  {
    //always set the child mode to FG if crtlz == 1
    //and decrease # of args to prevent seg faulting due to lack of &
    --numberOfArgs;

    ampersandBool = 0;
  }
  if (ampersandBool == 0 && pid == 0)
  {
    sa->sa_handler = &FG_handler;
    redirChecker(parsedArguments, *numberOfArgs, pid);
  }
  else if (ampersandBool == 1 && pid == 0)
  {
    sa->sa_handler = &defaultSH;
    int null = open("/dev/null",0);
    //set input and output to null unless specified by the redir checker
    dup2(null, STDIN_FILENO);
    dup2(null, STDOUT_FILENO);
    redirChecker(parsedArguments, *numberOfArgs, pid);
  }
  flush();
  //checking to see if its the parent
  if (pid != 0 && pid != -1)
  {
    //checks if there's ampersands, will either wait or make it a bg process
    if (ampersandBool == 0)
    {
      *childPID = pid;

      waitpid(pid, &currentStatus, 0);
      if(WIFSIGNALED(currentStatus)) 
      {
        currentStatus = WTERMSIG(currentStatus);
        printf("\nForeground Process %d exited from signal:%d\n", pid, WTERMSIG(currentStatus));
      }

    }
    else if (ampersandBool == 1)
    {
      printf("\nBackground process PID is :%d\n", pid );
      //add bg processes to an array as suggested, to loop over and check.
      bgProcesses[bgCount] = pid;
      bgCount++;
      waitpid(pid, &currentStatus, WNOHANG);
    } 
     
  }
   return 1;
}
//basic splice function, sets the end of the party to null so, we dont run off later on
void valueShift(char** parsedArguments, int numberOfArgs,int index){
  
  for (int i = index; i < numberOfArgs ; ++i)
  {
    parsedArguments[i] = parsedArguments[i + 1];
  }
  parsedArguments[numberOfArgs] = NULL;

}
int redirChecker(char** parsedArguments, int numberOfArgs, pid_t pid){
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
          //printf("%d\n",numberOfArgs );
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
    printf("%s\n", parsedArguments[0]);
   execvp(parsedArguments[0], parsedArguments);
   currentStatus = 0;
   exit(0);
  }
  if (redirBool == 1)
  {
   execvp(parsedArguments[0], parsedArguments);
   currentStatus = 0;
   exit(0);
  }
  //exit for the children
  exit(0);
  
  }
return 1;
}

//function to check the butt of the string for an ampersand
int ampersandCheck(char **parsedArguments, int *numberOfArgs){
  for (int i = *numberOfArgs; i < *numberOfArgs + 1; ++i)
  {
    if (strcmp(parsedArguments[i], "&") == 0)
    {
      //set it to null so it doesnt get passed to exec
      parsedArguments[i] = NULL;
      --*numberOfArgs; 
      return 1;
    }
  }
  return 0;
}

void defaultSH(int signo){
  if (signo == SIGINT){
    //if theres a child process in the foreground when sigint is recieved, spit out its id and
    //the signal that killed it.
    if (childPID > 0)
    {
      currentStatus = signo;
      //printf("Foreground process: %d terminated due to signal %d\n",childPID, signo );
      childPID = 0;
    }
    //ignore sigint for the normal shell and BG processes
    
    flush();
    return;
  }
  //sets the global to its opposite value everytime a Crtl Z is sent
  //this value will be the final control of ampersand bool, which controls if a process is sent
  //to bg or fg
  if (signo == SIGTSTP)
  {
    crtlZ = crtlZ * -1;
    if (crtlZ == 1)
    {
      printf("\nActivating Foreground only mode, & will be ignored.\n");
    }
    if (crtlZ == -1)
    {
      printf("\nDeactivating Foreground only mode, & will be registered.\n");
    }
    return;
  }
  return;
}

void FG_handler(int signo)
{
  //ignore tstp if fg process
  if (signo == SIGTSTP)
  {
    return;
  }
}
void killTheYounglings(){
  //kill all bg processes 
  for (int i = 0; i < bgCount + 1; ++i)
  {
    kill(bgProcesses[i], SIGQUIT);
  }
  return;
}


int main(int argc, char const *argv[]) {
  //char array for input and parsed arguments derived from input
  char inputbuffer[2048];
  char **parsedArguments;
  

  char pid[10];
  char* parentpid = pid;
  int numberOfArgs = 0;
  
  struct sigaction sa;
  sa.sa_flags = 0;
  //error checking

  sa.sa_handler = defaultSH;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTSTP, &sa, NULL);
  

  //infinite loop!
  while (1) {
      //signal(SIGINT, SIG_IGN);
      flush();
      checkBg();
      flush();
      printf(": ");
      flush();
    
      //if the input has no errors continue on through the loop
      if (getInput(inputbuffer)) {
        continue;
      }

      parsedArguments = inputParser(inputbuffer);
      //set the end of the args to null to prevent segfaults
     
      numberOfArgs = ArgCount(parsedArguments);
      
      // a giant if else statement to detect if we're using in house commands before moving on to 
      //creating children
      //loop to check for $$

      for (int i = 0; i < numberOfArgs + 1; ++i)
      {
        //got the conversion from https://stackoverflow.com/questions/15262315/how-to-convert-pid-t-to-string
        if (strcmp(parsedArguments[i], "$$") == 0)
        {   
          snprintf(pid, 10,"%d",(int)getpid());
          parsedArguments[i] = pid;
          parentpid = pid;

        }
      }
      //EXIT catch```````````````````````````````````````````````````````````````
      if (strcmp(parsedArguments[0], "exit") == 0){
        killTheYounglings();
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

      //check for the $$ command
      else if (strcmp(parsedArguments[0], parentpid) == 0)
      {
        printf("%s\n", parentpid );
        flush();
      }
      else{

        //if there was an issue executing the command. set the status to 1
        //also executes the meat and potatoes of the shell
        if (execArgs(parsedArguments,&childPID, &numberOfArgs, &sa) == 0){
          currentStatus = 1;
        }
  
      }
    
    }
exit(0);
}
