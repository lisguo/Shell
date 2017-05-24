#include "shell.h"
#include "debug.h"
#include <errno.h>

int stdin_fd = STDIN_FILENO;
int stdout_fd = STDOUT_FILENO;

void changeDirectory(char* cmd){
	char** commands = trimSpaces(cmd);

	//Get input dir
    char* input_dir = commands[1];
    if (strcmp(commands[0], "cd") == 0 && input_dir == NULL){
     	//Go to home
    	if(chdir(getenv("HOME")) != 0){
    		error("%s\n", "Unable to change directory to HOME");
    	}
    }
    else if(strcmp(input_dir,"-") == 0){
    	info("PREVDIR is %s\n", getenv("PREVDIR"));
        //Go to prevdir
        if(chdir(getenv("PREVDIR")) != 0){
    		error("Unable to change directory to PREVDIR: %s\n", getenv("PREVDIR"));
    	}
    }
    else{
        //Go to input_dir
        if(chdir(input_dir) != 0){
        	fprintf(stderr,"Unable to change directory to %s\n", input_dir);
        }
    }
    setPrevDir();
    freeStringArr(commands);
    free(commands);
}

void setPrevDir(){
	//Set PREVDIR
	if(setenv("PREVDIR", getenv("PWD"), 1) != 0){
    	error("%s\n", "Unable to set prevdir");
	}
}

void setWorkingDir(){
	size_t dir_size = 0;
	char* buffer = malloc(sizeof(char*));
	//Increase size if it's not enough
	while(getcwd(buffer, dir_size) == NULL){
		dir_size++;
    	buffer = realloc(buffer, dir_size);
	}

    //Set pwd
    if(setenv("PWD", buffer, 1) != 0){
    	error("%s\n", "Unable to set PWD");
    }

    free(buffer);

    info("PWD is now %s\n", getenv("PWD"));
}

void printWorkingDirectory(char* buffer){
	char* dir = getenv("PWD");
	pid_t child_pid = 0;
    int child_status;

    //Set signal
    struct sigaction s_action;
	memset (&s_action, '\0', sizeof(s_action));

	s_action.sa_sigaction = &handle_child;
	s_action.sa_flags = SA_SIGINFO;

	if (sigaction(SIGCHLD, &s_action, NULL) < 0) {
		perror ("SIGCHLD ERROR");
	}


	if((child_pid = fork()) == 0){
        info("Child process: %d\n", getpid());
        printf("%s\n",dir);
    	exit(EXIT_SUCCESS);
    }
    else{
        waitpid(child_pid, &child_status, 0);
        if(WIFEXITED(child_status)){
            //info("Child %d exited with status %d\n", child_pid, WEXITSTATUS(child_status));
        }
    }
}

void executeProgram(char** commands){
	char** args = commands;
	int index = indexOf(commands, "<");
	if(index == -1){
		index = indexOf(commands, ">");
	}
	if(index == -1){
		index = indexOf(commands, ">>");
	}
	info("Index is %d\n", index);
    stdout_fd = dup(STDOUT_FILENO); //FD for stdout
    stdin_fd = dup(STDIN_FILENO); //FD for stdin

    int out_fd = -1;
    int in_fd = -1;

    if(getCount(commands, "|") == 1){ //Check for piping
    	singlePipe(commands, stdin_fd, stdout_fd);

    }
    else if(getCount(commands, "|") == 2){
    	doublePipe(commands, stdin_fd, stdout_fd);
    }
    else{	//No piping, just redirection
    	if(indexOf(args, ">") != -1 && indexOf(args, "<") !=  -1){ //Redirect input to output
	    	int out_index = indexOf(args, ">");
	    	int in_index = indexOf(args, "<");

	    	out_fd = open(commands[out_index + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
	    	in_fd = open(commands[in_index + 1], O_RDONLY, S_IRWXU);
	    	info("Out FD: %d\nIn FD: %d\n", out_fd, in_fd);
	    	redirect(in_fd, STDIN_FILENO); //Redirect in to stdin
	    	redirect(out_fd, STDOUT_FILENO); //Redirect stdout to out
	    }
	    else if(indexOf(args, ">") != -1){ //Redirect stdin to output
	    	int out_index = indexOf(args, ">");
	    	out_fd = open(commands[out_index + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);

	    	in_fd = STDOUT_FILENO;
	    	redirect(out_fd, in_fd);
	    }
	    else if(indexOf(args, ">>") != -1){
	    	info("%s\n", "Append!");
	    	int out_index = indexOf(args, ">>");
	    	out_fd = open(commands[out_index + 1], O_WRONLY | O_CREAT | O_APPEND, S_IRWXU);

	    	in_fd = STDOUT_FILENO;
	    	redirect(out_fd, in_fd);
	    }
	    else if(indexOf(args, "<") !=  -1){ //Redirect output to stdin
	    	int in_index = indexOf(args, "<");
	    	in_fd = open(commands[in_index + 1], O_RDONLY, S_IRWXU);

	    	out_fd = STDIN_FILENO;
	    	redirect(in_fd, out_fd);
	    }
	    else if(getFDRedirection(args) != -1){ //Check for special redirection
	    	int fd = getFDRedirection(args);
	    	char buff[5];
			sprintf(buff,"%d%s",fd,">");
			int out_index = indexOf(commands, buff);
	    	out_fd = open(commands[out_index + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
	    	in_fd = fd;
	    	redirect(out_fd, in_fd);
	    	index = out_index;
	    }
	    if(index != -1){
			args = malloc(sizeof(char*) * index + 1);
			//Copy args 0 to index
			for (int i = 0; i < index; i++){
				args[i] = malloc((strlen(commands[i]) + 1) * sizeof(char));
				strcpy(args[i], commands[i]);

				info("Copying %s to %s \n",commands[i], args[i]);
			}
			args[index] = NULL;
		}

		//Get program name
	    char* name = args[0];
	    //Look for name
	    char* prog_path = getProgramPath(getenv("PATH"), name);

	    info("Path of program: %s\n", prog_path);

	    if(prog_path == NULL){
	        fprintf(stderr, "%s\n", "Incorrect name! Program not found.\n");
	        if(index != -1){
		    	freeStringArr(args);
		    	free(args);

		    	info("%s\n", "Free'd args.");
		    }
	        return;
	    }

	    //Execute
	    forkAndExecv(prog_path, args);

	    if(in_fd >= 0 && out_fd >= 0){
	    	info("%s\n", "Closing File Descriptor and Setting STDOUT/STDIN back");
	    	//Close fd
	    	close(in_fd);
	    	close(out_fd);

    	dup2(stdout_fd, 1);
    	dup2(stdin_fd, 0);
	    }

	    //If we had to make a path for the program, then we need to free it
	    if(strcmp(name, prog_path) != 0){
	        free(prog_path);
	    }
	    if(index != -1){
	    	freeStringArr(args);
	    	free(args);

	    	info("%s\n", "Free'd args.");
	    }
    }
}

void singlePipe(char** commands, int stdin_fd, int stdout_fd){
	info("%s\n", "There is one pipe");

	//Get args of first program
	int pipe_index = indexOf(commands, "|");

	//Check for invalid prog names
	if(commands[0] == NULL || commands[pipe_index+1] == NULL){
		fprintf(stderr, "Invalid program name.\n");
		return;
	}

	char** arg_1 = malloc(sizeof(char*) * pipe_index + 1);
	for(int i = 0; i < pipe_index; i++){
		arg_1[i] = malloc(strlen(commands[i]));
		strcpy(arg_1[i] , commands[i]);
	}
	arg_1[pipe_index] = NULL;

	//Get args of second program
	pipe_index++;
	int count = 1;
	while(commands[pipe_index] != NULL){ //Get count
		count++;
		pipe_index++;
	}
	char** arg_2 = malloc(sizeof(char*) * count);
	pipe_index = indexOf(commands, "|") + 1;
	count = 0;
	while(commands[pipe_index] != NULL){
		arg_2[count] = malloc(strlen(commands[pipe_index]) + 1);
		strcpy(arg_2[count], commands[pipe_index]);
		count++;
		pipe_index++;
	}
	arg_2[count] = NULL;

	char* path_1 = getProgramPath(getenv("PATH"), arg_1[0]);
	char* path_2 = getProgramPath(getenv("PATH"), arg_2[0]);

	if(path_1 == NULL || path_2 == NULL){
		fprintf(stderr, "Invalid program name.\n");
		return;
	}

	int fd[2];
	if(pipe(fd) == -1){
		fprintf(stderr, "Error while piping.");
		return;
	}

	//Fork
	pid_t prog1_pid;
	pid_t prog2_pid;
	int child_status;

	prog1_pid = fork();
	if(prog1_pid == 0){
		info("Redirecting output pipe : %d to %d\n", fd[1], STDOUT_FILENO);
		redirect(fd[1], STDOUT_FILENO);
		close(fd[0]);
		info("Executing %s\n", path_1);
        execv(path_1, arg_1);
    	exit(EXIT_SUCCESS);
    }
    else{
        waitpid(prog1_pid, &child_status, 0);
        if(WIFEXITED(child_status)){
        	close(fd[1]);
            info("Child %d exited with status %d\n", prog1_pid, WEXITSTATUS(child_status));
        }
    }

    prog2_pid = fork();
    if(prog2_pid == 0){
    	info("Redirecting input pipe : %d to %d\n", fd[0], STDIN_FILENO);
		redirect(fd[0], STDIN_FILENO);
		close(fd[1]);
		info("Executing %s\n", path_2);
        execv(path_2, arg_2);
    	exit(EXIT_SUCCESS);
    }
    else{
        waitpid(prog2_pid, &child_status, 0);
        if(WIFEXITED(child_status)){
        	close(fd[0]);
            info("Child %d exited with status %d\n", prog2_pid, WEXITSTATUS(child_status));
        }
    }
    info("%s\n", "Closing File Descriptors and Freeing Memory");
    //close and put fds back
    close(fd[0]);
    close(fd[1]);
    dup2(stdout_fd, 1);
    dup2(stdin_fd, 0);

    freeStringArr(arg_1);
    free(arg_1);
    freeStringArr(arg_2);
    free(arg_2);
}

void doublePipe(char** commands, int stdin_fd, int stdout_fd){
	info("%s\n", "There are two pipes");

	//Get args of first program
	int pipe_index = indexOf(commands, "|");
	char** arg_1 = malloc(sizeof(char*) * pipe_index + 1);
	for(int i = 0; i < pipe_index; i++){
		arg_1[i] = malloc(strlen(commands[i]));
		strcpy(arg_1[i] , commands[i]);
	}
	arg_1[pipe_index] = NULL;

	//Get args of second program
	pipe_index++;
	int count = 1;
	while(commands[pipe_index] != NULL){ //Get count
		count++;
		pipe_index++;
	}
	char** arg_2 = malloc(sizeof(char*) * count);
	pipe_index = indexOf(commands, "|") + 1;
	count = 0;
	while(strcmp(commands[pipe_index], "|") != 0){
		arg_2[count] = malloc(strlen(commands[pipe_index]) + 1);
		strcpy(arg_2[count], commands[pipe_index]);
		count++;
		pipe_index++;
	}
	arg_2[count] = NULL;

	//Get args of third program
	pipe_index++;
	int arg_start = pipe_index;
	count = 1;
	while(commands[pipe_index] != NULL){ //Get count
		count++;
		pipe_index++;
	}
	char** arg_3 = malloc(sizeof(char*) * count);
	count = 0;
	while(commands[arg_start] != NULL){
		arg_3[count] = malloc(strlen(commands[arg_start]) + 1);
		strcpy(arg_3[count], commands[arg_start]);
		count++;
		arg_start++;
		info("Arg_start is %d\n", arg_start);
	}
	arg_3[count] = NULL;

	char* path_1 = getProgramPath(getenv("PATH"), arg_1[0]);
	char* path_2 = getProgramPath(getenv("PATH"), arg_2[0]);
	char* path_3 = getProgramPath(getenv("PATH"), arg_3[0]);

	if(path_1 == NULL || path_2 == NULL || path_3 == NULL){
		fprintf(stderr, "Invalid program name.\n");
	    return;
	}

	int fd[2];
	if(pipe(fd) == -1){
		fprintf(stderr, "Error while piping.");
		return;
	}

	//Fork
	pid_t prog1_pid;
	pid_t prog2_pid;
	pid_t prog3_pid;
	int child_status;

	prog1_pid = fork();
	if(prog1_pid == 0){
		info("Redirecting output pipe : %d to %d\n", fd[1], STDOUT_FILENO);
		redirect(fd[1], STDOUT_FILENO);
		close(fd[0]);
		info("Executing %s\n", path_1);
        execv(path_1, arg_1);
    	exit(EXIT_SUCCESS);
    }
    else{
        waitpid(prog1_pid, &child_status, 0);
        if(WIFEXITED(child_status)){
        	close(fd[1]);
            info("Child %d exited with status %d\n", prog1_pid, WEXITSTATUS(child_status));
        }
    }

    int fd2[2];
    if(pipe(fd2) == -1){
		fprintf(stderr, "Error while piping.");
		return;
	}

	prog2_pid = fork();
    if(prog2_pid == 0){
    	info("Redirecting input pipe : %d to %d\n", fd[0], fd[1]);
		redirect(fd[0], STDIN_FILENO);
		redirect(fd2[1], STDOUT_FILENO);
		close(fd[1]);
		close(fd2[0]);
		info("Executing %s\n", path_2);
        execv(path_2, arg_2);
    	exit(EXIT_SUCCESS);
    }
    else{
        waitpid(prog2_pid, &child_status, 0);
        if(WIFEXITED(child_status)){
        	close(fd[0]);
        	close(fd2[1]);
            info("Child %d exited with status %d\n", prog2_pid, WEXITSTATUS(child_status));
        }
    }

    prog3_pid = fork();
    if(prog3_pid == 0){
    	info("Redirecting input pipe : %d to %d\n", fd2[0], STDIN_FILENO);
		redirect(fd2[0], STDIN_FILENO);
		close(fd2[1]);
		info("Executing %s\n", path_3);
        execv(path_3, arg_3);
    	exit(EXIT_SUCCESS);
    }
    else{
        waitpid(prog3_pid, &child_status, 0);
        if(WIFEXITED(child_status)){
        	close(fd2[0]);
        	close(fd2[1]);
            info("Child %d exited with status %d\n", prog2_pid, WEXITSTATUS(child_status));
        }
    }

    info("%s\n", "Closing File Descriptors and Freeing Memory");
    //close and put fds back
    close(fd[0]);
    close(fd[1]);
    close(fd2[0]);
    close(fd2[1]);

    dup2(stdout_fd, 1);
    dup2(stdin_fd, 0);

    freeStringArr(arg_1);
    free(arg_1);
    freeStringArr(arg_2);
    free(arg_2);
    freeStringArr(arg_3);
    free(arg_3);
}
void redirect(int in_fd, int out_fd){
	if(in_fd >= 0 && out_fd >= 0){
		if(dup2(in_fd, out_fd) != -1){
			success("%s\n", "Redirection successful.");
		}
		else{
			fprintf(stderr, "Error while duplicating file descriptor.");
		}
	}
	else{
		fprintf(stderr, "Error opening file descriptor.");
	}
}

int indexOf(char** arr, char* str){
	char* curr = *arr;
	int index = 0;
	while(curr != NULL){
		if(strcmp(curr, str) == 0){
			return index;
		}
		//Increment
		index++;
		curr = *(arr + index);
	}
	return -1;
}

int getFDRedirection(char** arr){
	char* curr = *arr;
	int index = 0;
	while(curr != NULL){
		//Check if last char is >
		if(curr[strlen(curr)-1] == '>'){
			//Get FD
			 return atoi(curr);
		}
		//Increment
		index++;
		curr = *(arr + index);
	}
	return -1;
}

int getCount(char** arr, char* str){
	int count = 0;
	int i = 0;
	char* curr = arr[i];
	while(curr != NULL){
		if(strcmp(curr, str) ==0){
			count++;
		}
		i++;
		curr = arr[i];
	}
	return count;
}

char* createPrompt(char* netid, char* dir, char* terminal){
	char* prompt = malloc(strlen(netid) + strlen(dir) + strlen(terminal) + 1);
	memset(prompt, 0, strlen(prompt)); //Clear prompt and add contents
    strcpy(prompt, netid);
    strcat(prompt, dir);
    strcat(prompt, terminal);
    return prompt;
}

char* getProgramPath(char* paths, char* prog_name){
	if(prog_name == NULL){
		return NULL;
	}
	//Check for /
	if(strchr(prog_name, '/') != NULL){
		//Does prog_name exist?
		struct stat* file_info = malloc(sizeof(struct stat));
		if(stat(prog_name, file_info) == 0){
			//Found path
			success("Program path found! : %s\n", prog_name);
			free(file_info);
			return prog_name;
		}
		free(file_info);
		return NULL;
	}
	//Create a copy of paths to use strtok()
	char* paths_cpy = malloc(strlen(paths) + 1);
	strcpy(paths_cpy, paths);

	char* curr_path = strtok(paths_cpy, ":");
	while(curr_path != NULL){
		//Check if next_path has executable
		char* prog_path = malloc(strlen(curr_path)+ 1 + strlen(prog_name) + 1);
		strcpy(prog_path, curr_path);
		strcat(prog_path, "/");
		strcat(prog_path, prog_name);

		struct stat* file_info = malloc(sizeof(struct stat));
		if(stat(prog_path, file_info) == 0){
			//Found path
			success("Program path found! : %s\n", prog_path);
			free(file_info);
			free(paths_cpy);
			return prog_path;
		}
		free(file_info);
		//Get next path
		curr_path = strtok(NULL, ":");
	}
	free(paths_cpy);
	return NULL;
}

void forkAndExecv(char* path, char** args){
	if(args == NULL || args[0] == NULL){
		fprintf(stderr, "Invalid program arguments.\n");
		return;
	}
	//Set signal
    struct sigaction s_action;
	memset (&s_action, '\0', sizeof(s_action));

	s_action.sa_sigaction = &handle_child;
	s_action.sa_flags = SA_SIGINFO;

	if (sigaction(SIGCHLD, &s_action, NULL) < 0) {
		char* error_msg = "ERROR SETTING SIGCHLD";
		write(STDERR_FILENO, error_msg, strlen(error_msg));
	}

	pid_t child_pid = 0;
    int child_status;

	if((child_pid = fork()) == 0){
		info("Executing %s\n", path);
		//Redirect based redirection code
        execv(path, args);
    	exit(EXIT_SUCCESS);
    }
    else{
        waitpid(child_pid, &child_status, 0);
    }
}

char** trimSpaces(char* str){
	int index = 0;
	int count = 1;

	char* str2 = str;
	while(*str2 != '\0' && str2 != NULL){
		//Find first letter
		while(isspace(*str2)){
			str2++;
		}
		if(*str2 == '\0'){ //All space
			break;
		}
		char* substr = str2;
		while(!isspace(*substr) && *substr != '\0'){
			substr++;
		}
		str2 = substr;
		count++;
	}
	info("Count is %d\n", count);

	char** buffer = malloc(sizeof(char*) * count);

	char* trimStr = str;
	while(*trimStr != '\0' && trimStr != NULL){
		//Find first letter
		while(isspace(*trimStr)){
			trimStr++;
		}
		if(*trimStr == '\0'){ //All space
			return buffer;
		}
		char* substr = trimStr;
		while(!isspace(*substr) && *substr != '\0'){
			substr++;
		}

		buffer[index] = malloc(sizeof(char) * (substr - trimStr));

		strncpy(buffer[index], trimStr, substr - trimStr);
		(buffer[index])[substr - trimStr] = '\0';
		info("Arg: %s\n", buffer[index]);
		index++;
		trimStr = substr;
	}

	info("Count is %d\n", count);
	buffer[count-1] = NULL;
	return buffer;
}

void freeStringArr(char** list){
	while(*list != NULL){
		info("Freeing: %s\n", *list);
		free(*list);
		list++;
	}
}

/* When child dies, display PID and CPU utilization */
void handle_child(int sig, siginfo_t *siginfo, void *context){
    dup2(stdout_fd, 1);
    dup2(stdin_fd, 0);
    pid_t pid = siginfo -> si_pid;
    double total_time =  (double) (siginfo -> si_stime + siginfo -> si_utime) / (sysconf(_SC_CLK_TCK) / 1000.0);
    char buff[74];
    sprintf(buff, "Child with PID %lu has died. It spent %g milliseconds utilizing the CPU\n",(long)pid, total_time);
    write(1, buff, 74);
}

