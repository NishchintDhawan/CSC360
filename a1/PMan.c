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
    Node* next;
} node;

node* head = NULL;

void printlist(node* head);

int main(){
		char *cmd;
		char *temp;
		char *cmd_type ="bg" ;
		char *token;

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
				//s	printf("%s  \n",argv[i]);
					i++;
					token = strtok(NULL, " ");
   				}
				cmd_type=argv[0];
                if (strcmp(cmd_type, CMD_BG)==0){
                      bg_entry(&argv[1]);
            	// printf("Hello");
                }

                else if(strcmp(cmd_type, CMD_BGLIST)==0){
					printlist(head);
                }

                // else if(cmd_type == CMD_BGKILL || cmd_type == CMD_BGSTOP || cmd_type == CMD_BGCONT){
                //      bgsig_entry(pid, cmd_type);

                // }
                // else if(cmd_type == CMD_PSTAT){
                //      pstat_entry(pid);
                // }
                // else {
                //      usage_pman();
                // }
       			 //      check_zombieProcess();

				// char *release = argv[0];
				// for(i=0; release;i++){
				// 	free(argv[i]);
				// 	release=argv[i+1];
				// }
        }
        return 0;
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
		n1->command = argv[0];
		n1->next=NULL;

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
		//printlist(head);
    }
     else {
             perror("fork failed");
             exit(EXIT_FAILURE);
     }
}

void bglist_entry(){

}

void usage_pman(){
	printf("invalid command\n");
}
//prints the list of processes.
void printlist(node *head){

	int i=0;
	node *curr=head;
	printf("======\n");
	while(curr!=NULL){
		printf("%d: %s\n", curr->pid, curr->command);
		curr=curr->next;
		i++;
	}
	printf("total: %d", i);
	printf("\n======\n");
}

// void check_zombieProcess(void){


//      //delete zombie process from our data structure.
//      int status;
//      int retVal = 0;

//      while(1) {
//              usleep(1000);
//              if(headPnode == NULL){
//                      return ;
//              }
//              retVal = waitpid(-1, &status, WNOHANG);
//              if(retVal > 0) {
//                      //remove the background process from your data structure
//              }
//              else if(retVal == 0){
//                      break;
//              }
//              else{
//                      perror("waitpid failed");
//                      exit(EXIT_FAILURE);
//              }
//      }
//      return ;
// }