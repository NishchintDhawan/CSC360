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

#define MAX_COUNT  800

void bg_entry(char **argv);
void bglist_entry();
void check_zombieProcess(void);
void bgsig_entry(int, char *);
void pstat_entry(pid_t);
void usage_pman();
int check_pid(pid_t pid);

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
				   
				    pid = (pid_t)strtol(argv[1], NULL, 10);
					bgsig_entry(pid, cmd_type);
                }

                else if(strcmp(cmd_type, CMD_PSTAT)==0){
					 pid = (pid_t)strtol(argv[1], NULL, 10);
                     pstat_entry(pid);
                }

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
		return;
	}

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
		printf("Running process %d\n", pid);
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

	if(check_pid(pid)==1){
		printf("Process %d does not exist\n", pid);
		return;
	}

	if(strcmp(cmd,CMD_BGSTOP)==0){
		if(kill(pid, SIGSTOP)==1){
			printf("Error stopping process: %d\n", pid);
			return;
		}
		printf("Stopped process: %d\n\n", pid);
	}

	if(strcmp(cmd,CMD_BGCONT)==0){
		if(kill(pid, SIGCONT)==1){
			printf("Error continuing process %d\n", pid);
			return;
		}
		printf("Resumed process: %d\n", pid);

	}

	if(strcmp(cmd,CMD_BGKILL)==0){
		if(kill(pid, SIGTERM)==1){
			printf("Error killing process: %d\n", pid);
			return;
		}
	}

}

void usage_pman(){
	printf("invalid command\n");
}

//prints the list of processes.
void bglist_entry(node *head){

	int i=0;
	node *curr=head;

	while(curr!=NULL){
		printf("%d  ", curr->pid);
		printf("%s \n", curr->command);
		curr=curr->next;
		i++;
	}
	printf("Total background jobs: %d\n", i);
	
}

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

void statData(char *path){

	//open stat file and read input.
	char *statData[MAX_COUNT]; 			//store stat file data.
	char *data[512];
	FILE *fp = fopen(path, "r");
	int i = 0;
	char str[512] ; 
	int len =0;


	if(fp!=NULL){
		while(fgets(str, 512, fp) != NULL){
				data[i] = str;
				i++;
		}
	}
	else{
		printf("no stat file found for this pid\n");
		return;
	}

	char *token;
	len = i;
	i=0;
	token=strtok(data[0], " ");
	while(token !=NULL){
		statData[i] = token;
		i++;
		token=strtok(NULL," ");
	}

	printf("comm:%s \nstate:%s \nutime:%.6f \nstime:%.6f \nrss:%s \n", statData[1], statData[2], (float)(atoi(statData[13])/sysconf(_SC_CLK_TCK)), (float)(atoi(statData[14])/sysconf(_SC_CLK_TCK)), statData[24]);
	
}


void statusData(char *path){
	
	char *statusData[MAX_COUNT]; 			//store status file data.

	FILE *fp = fopen(path, "r");
	char str[MAX_COUNT];

	char *parameter;

	int i=0;

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
						printf("%s", line);
					}
					token=strtok(NULL," ");
				} 
				i++;
		}
	}
	else{
		printf("no status file found for this pid\n");
		return;
	}

}

void pstat_entry(pid_t pid){

	//check if pid is valid.
	if(check_pid(pid)==1){
		printf("Error: Process %d does not exist\n", pid);
		return;	
	}

	//create path to stat file.
	char *path1 = malloc(200+sizeof(pid));
	sprintf(path1, "/proc/%d/stat", pid);
	char *path2 = malloc(200+sizeof(pid));
	sprintf(path2, "/proc/%d/status", pid);

	statData(path1);
	statusData(path2);

}

void check_zombieProcess(void){

     int status;
     int retVal = 0;

     while(1) {
             usleep(1500);
             if(head == NULL){
                     return ;
             }
             retVal = waitpid(-1, &status, WNOHANG);
             if(retVal > 0) {
                    //remove the background process from your data structure
					
					if(WIFEXITED(status)){
						printf("Process %d terminated\n", retVal);
					}
					
					if(WIFSIGNALED(status)){
						printf("Process %d was killed\n", retVal);
					}

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


/*
Running process with pid: <pid> DDNE

run process with '..'

Contents for : 
README ?
MAKE FILE ?


-9 -> send SIGTERM signal.

print when process killed/stopped/resumed/created. DONE


utime and stime should be in float and in seconds 6 floating points.

check if pid exists in the bg list DONE



*/