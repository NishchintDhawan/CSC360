// Nishchint Dhawan
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

#define MAX_COUNT  200

void bg_entry(char **argv);
void bglist_entry();
void check_zombieProcess(void);
void bgsig_entry(int, char *);
void pstat_entry(char *);
void usage_pman();
typedef struct Node Node;

typedef struct Node {
	pid_t pid;
	char *command;
    Node *next;
} node;

node* head = NULL;

void printlist(node* head);

int main(){
		char *cmd;
		char *temp;
		char *cmd_type;
		char *token;
		pid_t pid;

        while(1){
        		cmd = readline("PMan: > ");
				
				if(strcmp(cmd,"")==0){
					continue;
				}

				//count spaces and create argv array.
				temp = (char *)malloc(strlen(cmd)*sizeof(char));
				strcpy(temp,cmd);
				int len=0;
				token=strtok(temp, " ");
				
				while(token !=NULL){
					len++;
					token=strtok(NULL," ");
				}

				free(temp);
				char *argv[len];
				/* parse the input cmd (e.g., with strtok)*/

				/* get the first token */
				token = strtok(cmd, " ");
				char *value;
				int i=0;
				
				/* walk through other tokens */
				while( token != NULL ) {					
					value = (char *)malloc(strlen(token)*sizeof(char));
					strcpy(value,token);

					argv[i] = value;
					i++;
					token = strtok(NULL, " ");
   				}
				

				argv[i]=NULL;
				cmd_type=argv[0];
                if (strcmp(cmd_type, CMD_BG)==0){
					if(argv[1]==NULL){
						printf("missing 1 argument with bg\n");
						continue;
					}
                      bg_entry(&argv[1]);
                }

                else if(strcmp(cmd_type, CMD_BGLIST)==0){
					bglist_entry(head);
                }

                else if( strcmp(cmd_type,CMD_BGKILL)==0 || strcmp(cmd_type, CMD_BGSTOP)==0 || strcmp(cmd_type, CMD_BGCONT)==0 ){
                   	if(argv[1]==NULL){
						printf("missing 1 argument with %s\n", cmd_type);
						continue;
					}

				 // printf("%s %s", argv[0], argv[1]);
				//	pid = (inargv[1];
				   
				    pid = (pid_t)strtol(argv[1], NULL, 10);
				//	printf("pid : %d ", pid);
					bgsig_entry(pid, cmd_type);
                }

                // else if(cmd_type == CMD_PSTAT){
                //      pstat_entry(pid);
                // }
                else {
                     usage_pman();
                }
       			
			    check_zombieProcess();

				for(int i=0;i<len;i++){
					free(argv[i]);
				}			
        }
        return 0;
}

void removeNode(pid_t pid_remove){

	node *curr = head;
	node *prev = NULL;
//	printf("Before if\n");
//	printf("curr: %p\n", curr);

	if(curr!=NULL && curr->pid==pid_remove){
		
		head = curr->next;
	//	printf("Deleting head node\n");
		free(curr);
		return;
	}

	while(curr!=NULL && curr->pid!=pid_remove){

		prev=curr;
		curr=curr->next;
	}

	if(curr==NULL){
		//printf("Node not found");
		return;
	}

//	printf("Deleting the node : %d %s\n", curr->pid, curr->command);
	prev->next = curr->next;
	free(curr);
	return;
}

void bg_entry(char **argv){
	
    pid_t pid;
    pid = fork();
    if(pid == 0){
		//child process.
		//	printf("pid is 0\n");
		if(execvp(argv[0], argv) < 0){
				perror("Error on execvp");
		}
		exit(EXIT_SUCCESS);
	}

	else if(pid > 0) {
		//parent process.
		//printf("pid > 0\n");	
		node* n1 = (node *)malloc(sizeof(node));

		n1->pid = (pid_t)pid;
		n1->next=NULL;

		char *temp = (char *)malloc(strlen(argv[0])*sizeof(char));
		strcpy(temp,argv[0]);
		n1->command = temp;

		//insert into list
		if(head==NULL){
			head=n1;
		}
		else{
			node *curr = head; 
			while(curr->next!=NULL){
				curr=curr->next;
			}
			curr->next=n1;
		}
    }
     else {
             perror("fork failed");
             exit(EXIT_FAILURE);
     }
	 
}



void bgsig_entry(pid_t pid, char* cmd){

	if(strcmp(cmd,CMD_BGSTOP)==0){
		kill(pid, SIGSTOP);
	}

	if(strcmp(cmd,CMD_BGCONT)==0){
		kill(pid, SIGCONT);
	}

	if(strcmp(cmd,CMD_BGKILL)==0){
		kill(pid, SIGTERM);
		removeNode(pid);
	}
}

void usage_pman(){
	printf("invalid command\n");
}

//prints the list of processes.
void bglist_entry(node *head){

	int i=0;
	node *curr=head;
	printf("======\n");
	while(curr!=NULL){
		printf("%d  ", curr->pid);
		printf("%s \n", curr->command);
		//removeNode(curr->pid);
		curr=curr->next;
		i++;
	}
	printf("total: %d", i);
	printf("\n======\n");
	
}



void check_zombieProcess(void){


     //delete zombie process from our data structure.
     int status;
     int retVal = 0;

     while(1) {
             usleep(1000);
             if(head == NULL){
				 //    printf("head is null\n");
                     return ;
             }
             retVal = waitpid(-1, &status, WNOHANG);
		//	 printf("retVal: %d\n", retVal);
             if(retVal > 0) {
                     //remove the background process from your data structure
					 removeNode(retVal);
             }
             else if(retVal == 0){
                     break;
             }
             else{
                     perror("waitpid failed");
                     exit(EXIT_FAILURE);
             }
     }
     return ;
}