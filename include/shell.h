#ifndef SHELL_H
#define SHELL_H
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


void changeDirectory(char* cmd);
void printWorkingDirectory();


void executeProgram(char** commands);
void singlePipe(char** commands, int stdin_fd, int stdout_fd);
void doublePipe(char** commands, int stdin_fd, int stdout_fd);
void redirect(int in_fd, int out_fd);

//Creates a prompt that is put in the buffer
char* createPrompt(char* netid, char* dir, char* terminal);

void setPrevDir();
void setWorkingDir();

//Paths is a string of directories separated by colons (:)
char* getProgramPath(char* paths, char* prog_name);
void forkAndExecv(char* path, char** args);

char** trimSpaces(char* str);
void freeStringArr(char** list);
int indexOf(char** arr, char* str);
int getCount(char** arr, char* str);
int getFDRedirection(char** arr);

void handle_alarm(int sig, siginfo_t *siginfo, void *context);
void handle_child(int sig, siginfo_t *siginfo, void *context);
void handle_sigusr(int sig, siginfo_t *siginfo, void *context);
#endif
