/*
Name: Nishchint Dhawan
ID: V00882939
Course: CSC360 
Assignment: P1
*/

/* Imports */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

#define CMD_BG "bg"
#define CMD_BGLIST "bglist"
#define CMD_BGKILL "bgkill"
#define CMD_BGSTOP "bgstop"
#define CMD_BGCONT "bgstart"
#define CMD_PSTAT "pstat"

#define MAX_COUNT  800

/* Create function prototypes. */
void bg_entry(char **argv);
void bglist_entry();
void check_zombieProcess(void);
void bgsig_entry(int, char *);
void pstat_entry(pid_t);
void usage_pman(char *cmd_type);
int check_pid(pid_t pid);
void removeNode(pid_t pid_remove);
void statData(char *path);
void statusData(char *path);


/* Create list using nodes. */
typedef struct Node Node;

typedef struct Node {
	pid_t pid;
	char *command;
    Node *next;
} node;

/* List head pointer. */
node* head = NULL;		

int main(){
		char *cmd;
		char *temp;
		char *cmd_type;
		char *token;
		pid_t pid;

        while(1){
        		cmd = readline("PMan: > ");		//take user input.
				
				if(strcmp(cmd,"")==0){			//return nothing if no input is entered.
					continue;
				}

				/* Count spaces and create argv array. */
				temp = (char *)malloc(strlen(cmd)*sizeof(char));
				strcpy(temp,cmd);
				int len=0;
				
				token=strtok(temp, " ");			//count size of input and store in 'len'.
				while(token !=NULL){
					len++;
					token=strtok(NULL," ");
				}

				free(temp);
				char *argv[len];
				/* parse the input cmd (e.g., with strtok) */

				/* get the first token */
				token = strtok(cmd, " ");
				char *value;
				int i=0;
				
				/* walk through other tokens */
				while( token != NULL ) {					
					value = (char *)malloc(strlen(token)*sizeof(char));
					strcpy(value,token);

					argv[i] = value;				/*store the tokens in argv array.*/
					i++;
					token = strtok(NULL, " ");
   				}

				argv[i]=NULL;						/*last token should be NULL.*/

				cmd_type=argv[0];					/* get type of command.*/

				/* Check if command entered is 'bg' */
                if (strcmp(cmd_type, CMD_BG)==0){
					if(argv[1]==NULL){
						printf("missing 1 argument with bg\n\n");
						continue;
					}
                      bg_entry(&argv[1]);
                }

				/* Check if command entered is 'bglist' */
                else if(strcmp(cmd_type, CMD_BGLIST)==0){
					if(argv[1]!=NULL){
						printf("too many arguments for bglist. Needs 0 arguments.\n\n");
						continue;
					}
					bglist_entry(head);
                }

				/* Check if command entered is 'bgkill' or 'bgstop' or 'bgstart' */
                else if( strcmp(cmd_type,CMD_BGKILL)==0 || strcmp(cmd_type, CMD_BGSTOP)==0 || strcmp(cmd_type, CMD_BGCONT)==0 ){
                   	if(argv[1]==NULL){
						printf("missing 1 argument with %s\n\n", cmd_type);
						continue;
					}
				   
				    pid = (pid_t)strtol(argv[1], NULL, 10);
					bgsig_entry(pid, cmd_type);
                }

				/* Check if command entered is 'pstat' */
                else if(strcmp(cmd_type, CMD_PSTAT)==0){
					if(argv[1]==NULL){
						printf("missing 1 argument with %s\n\n", cmd_type);
						continue;
					}

					 pid = (pid_t)strtol(argv[1], NULL, 10);
                     pstat_entry(pid);
                }

				/* Otherwise command is not valid. */
                else {
                     usage_pman(cmd_type);
                }
       			
			    check_zombieProcess();			/* Check for zombie processes in background. */

				/* Free up space taken by the argv in memory for the new command. This prevents stack overflow.*/
				for(int i=0;i<len;i++){
					free(argv[i]);
				}			
        }
        return 0;
}

/* Remove a node from the list. */
void removeNode(pid_t pid_remove){

	node *curr = head;
	node *prev = NULL;

	/* If we want to remove the first element. */
	if(curr!=NULL && curr->pid==pid_remove){		
		head = curr->next;
		free(curr);
		return;
	}

	/* Walk through the list to find the node to be removed. */
	while(curr!=NULL && curr->pid!=pid_remove){		
		prev=curr;
		curr=curr->next;
	}

	/* Return if node is not found in the list. */
	if(curr==NULL){									
		return;
	}

	/* Remove the node and free up memory space. */
	prev->next = curr->next;						
	free(curr);
	return;
}

/* Add new process to the list.*/
void bg_entry(char **argv){
	
    pid_t pid;
    pid = fork();
    if(pid == 0){
		//child process.
		/* execute the command using execvp. */
		if(execvp(argv[0], argv) < 0){			
				perror("Error on execvp");
		}
		exit(EXIT_SUCCESS);
	}

	else if(pid > 0) {
		//parent process.

		/* Create new node for the list.*/
		node* n1 = (node *)malloc(sizeof(node));
		printf("Running process %d\n\n", pid);
		n1->pid = (pid_t)pid;
		n1->next=NULL;

		/* store the command string in the command field. */
		char *temp = (char *)malloc(strlen(argv[0])*sizeof(char));
		strcpy(temp,argv[0]);
		n1->command = temp;

		/* insert into list. */
		if(head==NULL){
			head=n1;
		}
		else{
			node *curr = head; 
			while(curr->next!=NULL){	/* Move to the end of the list. */
				curr=curr->next;
			}
			curr->next=n1;				/* insert node. */
		}
    }
     else {
             perror("fork failed");
             exit(EXIT_FAILURE);
     }
	 
}

/* Kill, Stop or Start a process.  */
void bgsig_entry(pid_t pid, char* cmd){

	/* Check if pid is valid or not. */
	if(check_pid(pid)==1){
		printf("Process %d does not exist\n\n", pid);
		return;
	}

	/* Stop a process. */
	if(strcmp(cmd,CMD_BGSTOP)==0){
		if(kill(pid, SIGSTOP)==1){
			printf("Error stopping process: %d\n\n", pid);
			return;
		}
		printf("Stopped process: %d\n\n", pid);
	}
	
	/* Continue a process. */
	if(strcmp(cmd,CMD_BGCONT)==0){
		if(kill(pid, SIGCONT)==1){
			printf("Error continuing process %d\n\n", pid);
			return;
		}
		printf("Resumed process: %d\n\n", pid);

	}

	/* Kill a process. */
	if(strcmp(cmd,CMD_BGKILL)==0){
		if(kill(pid, SIGTERM)==1){
			printf("Error killing process: %d\n\n", pid);
			return;
		}
	}

}

/* Runs if the command entered is not valid. */
void usage_pman(char *cmd_type){
	printf("%s : command not found\n\n", cmd_type);
}

/* Prints the list of processes. */
void bglist_entry(node *head){

	int i=0;
	node *curr=head;

	/* Walk through list and print. */
	while(curr!=NULL){
		printf("\n%d: ", curr->pid);
		printf("%s \n", curr->command);
		curr=curr->next;
		i++;
	}
	printf("Total background jobs: %d\n\n", i);
	
}

/* Check if a given pid is valid or not (exists in the list). */
int check_pid(pid_t pid){

	node *curr = head;

	while(curr!=NULL){
		if(curr->pid==pid){
			return 0;
		}
		curr=curr->next;
	}
	return 1;
}

/* Print the contents of the stat file. */
void statData(char *path){

	//open stat file and read input.
	char *statData[MAX_COUNT]; 			//store stat file data.
	char *data[512];
	FILE *fp = fopen(path, "r");
	int i = 0;
	char str[1024] ; 

	/* Read the stat file and store the data. */
	if(fp!=NULL){
		while(fgets(str, 1024, fp) != NULL){
				data[i] = str;
				i++;
		}
	}
	else{
		printf("no stat file found for this pid\n\n");   /* If the file is not found. */
		return;
	}

	/* Extract the data and display nicely. */
	char *token;
	i=0;

	token=strtok(data[0], " ");
	while(token !=NULL){
		statData[i] = token;		/* Store each value in the statData array. */
		i++;
		token=strtok(NULL," ");
	}

	printf("\ncomm:\t\t%s \nstate:\t\t%s \nutime:\t\t%.6f \nstime:\t\t%.6f \nrss:\t\t%s \n", statData[1], statData[2], (float)(atoi(statData[13])/sysconf(_SC_CLK_TCK)), (float)(atoi(statData[14])/sysconf(_SC_CLK_TCK)), statData[24]);
	
}

/* Print the contents of the status file. */
void statusData(char *path){

	//open the file and read data.
	FILE *fp = fopen(path, "r");
	char str[MAX_COUNT];

	int i=0;

	/* Read the status file and print data. */
	if(fp!=NULL){
		while(fgets(str, 4096, fp) != NULL){

				char line[MAX_COUNT];
				strcpy(line,str);
				char *token;
				token = strtok(str,":");
				while(token !=NULL){
					if(strcmp(token,"voluntary_ctxt_switches")==0){
						printf("%s", line);
					}
					else if(strcmp(token,"nonvoluntary_ctxt_switches")==0){
						printf("%s\n", line);
					}
					token=strtok(NULL," ");
				} 
				i++;
		}
	}
	else{
		printf("no status file found for this pid\n\n");		/* If the file is not found. */
		return;
	}

}

/* Prints information for the process. */
void pstat_entry(pid_t pid){

	//check if pid is valid.
	if(check_pid(pid)==1){
		printf("Error: Process %d does not exist\n\n", pid);
		return;	
	}

	/* Create path to stat file. */
	char *path1 = malloc(200+sizeof(pid));
	sprintf(path1, "/proc/%d/stat", pid);

	/* Create path to status file. */
	char *path2 = malloc(200+sizeof(pid));
	sprintf(path2, "/proc/%d/status", pid);

	/* Print data for stat and status. */
	statData(path1);
	statusData(path2);

}

/* Checks for any zombie processes (terminated or killed) and removes them from our list. */
void check_zombieProcess(void){

     int status;
     int retVal = 0;

     while(1) {
             usleep(1500);
			 
             if(head == NULL){	/* if our list is empty, we return. */
                     return ;
             }

             retVal = waitpid(-1, &status, WNOHANG);
             if(retVal > 0) {
                    //remove the background process from your data structure
					
					/* Check if the process was terminated normally. */
					if(WIFEXITED(status)){		
						printf("Process %d was terminated\n\n", retVal);
					}
					
					/* Check if the process wsa killed. */
					if(WIFSIGNALED(status)){
						printf("Process %d was killed\n\n", retVal);
					}
					
					/* Remove the process from our list. */
					removeNode(retVal);
             }

            else if(retVal == 0){		/* If no zombie process is detected, we return. */
                    break;
            }
            else{
                    perror("waitpid failed");
                    exit(EXIT_FAILURE);
            }
    }
    return ;
}