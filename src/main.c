#include "shell.h"
#include "debug.h"

volatile sig_atomic_t alarm_sig = 0;
int main(int argc, char const *argv[], char* envp[]){
    rl_catch_signals = 0; // This is disable readline's default signal handlers.

    //Block SIGTSTP
    sigset_t mask, prev_mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTSTP); //Add SIGTSTP to mask for blocking
    sigprocmask(SIG_BLOCK, &mask, &prev_mask); //block sigs in mask and place prev sigset in prevmask

    char* cmd;

    char* user = "lisguo : ";
    char* dir = getenv("PWD");
    char* terminal = " $ ";
    char* prompt = createPrompt(user, dir, terminal);

    setPrevDir();

    //GET pwd
    while((cmd = readline(prompt)) != NULL) {
        info("Command: %s\n", cmd);
        char** commands = trimSpaces(cmd);
        if(strcmp(cmd, "") == 0){
            //Do nothing
            info("%s\n", "Nothing entered.");
        }
        else if (strcmp(commands[0], "exit") == 0 && commands[1] == NULL)
            exit(EXIT_SUCCESS);
        else if (strcmp(commands[0], "help") == 0 && commands[1] == NULL){
            printf("%s\n%s\n%s\n%s\n%s\n","help","exit","cd [ - | . | .. | directory ]","pwd","alarm [n]");
        }
        else if (strncmp(commands[0], "cd", 2) == 0){
           changeDirectory(cmd);
        }
        else if (strcmp(commands[0],"pwd") == 0 && commands[1] == NULL){
            printWorkingDirectory(dir);
        }
        else if(strcmp(commands[0], "alarm") == 0 && commands[1] != NULL && commands[2] == NULL){
            int n = atoi(commands[1]);
            if(n == 0){
                fprintf(stderr, "Not a valid argument.\n");
            }
            else{
                //Set signal
                struct sigaction s_action;

                s_action.sa_sigaction = &handle_alarm;
                s_action.sa_flags = SA_SIGINFO;

                if (sigaction(SIGALRM, &s_action, NULL) < 0) {
                    fprintf(stderr, "ERROR SETTING SIGALRM");
                }
                alarm(n);
                alarm_sig = n;
            }
        }
        else{
            info("%s\n", "Executing program");
            executeProgram(commands);
            success("%s\n", "Program properly executed.");
        }
        info("%s\n", "Freeing commands");
        freeStringArr(commands);
        free(commands);

        //Remake prompt at the end of each iteration
        setWorkingDir();
        char* dir = getenv("PWD");

        info("%s\n", "Creating prompt...");
        free(prompt);
        prompt = createPrompt(user, dir, terminal);
    }

    free(cmd);

    //Restore previous mask
    sigprocmask(SIG_SETMASK, &prev_mask, NULL);
    return EXIT_SUCCESS;
}

//alarm handler
void handle_alarm(int sig, siginfo_t *siginfo, void *context){
    char buff[36];
    sprintf(buff, "\nYour %d second timer has finished!",alarm_sig);
    write(1, buff, 36);
    //printf("\nYour %d second timer has finished!\n", alarm_sig);
}
