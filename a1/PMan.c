int main(){

	while(1){	
		cmd = readline("PMan: > ");
		
		/* parse the input cmd (e.g., with strtok)
		 */
		
		if (cmd_type == CMD_BG){
			bg_entry(argv);
		}
		else if(cmd_type == CMD_BGLIST){
			bglist_entry();
		}
		else if(cmd_type == CMD_BGKILL || cmd_type == CMD_BGSTOP || cmd_type == CMD_BGCONT){
			bgsig_entry(pid, cmd_type);
		}
		else if(cmd_type == CMD_PSTAT){
			pstat_entry(pid);
		}
		else {
			usage_pman();
		}
		check_zombieProcess();
	}
	return 0;
}

void bg_entry(char **argv){
	
	pid_t pid;
	pid = fork();
	if(pid == 0){
		if(execvp(argv[0], argv) < 0){
			perror("Error on execvp");
		}
		exit(EXIT_SUCCESS);
	}
	else if(pid > 0) {
		// store information of the background child process in your data structures
	}
	else {
		perror("fork failed");
		exit(EXIT_FAILURE);
	}
}

void check_zombieProcess(void){
	int status;
	int retVal = 0;
	
	while(1) {
		usleep(1000);
		if(headPnode == NULL){
			return ;
		}
		retVal = waitpid(-1, &status, WNOHANG);
		if(retVal > 0) {
			//remove the background process from your data structure
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