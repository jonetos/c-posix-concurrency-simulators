#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <poll.h>
#include "tokenize.h"

volatile sig_atomic_t exit_flag = 0; 

struct message{
	long level;
	int priority;
	int pid;
	char string[100];
};

void sigint_handler(int sig){}

void sigusr1_handler(int sig){
	exit_flag = 1;
}

void execute(struct message msg){ 
	char **argv_exec;
	pid_t pid_executor = fork();
	if(pid_executor == 0){
		argv_exec = tokenize(msg.string);
		if(argv_exec == NULL){
			printf("Error tokenizing the process to execute. Process automatically terminated.\n");
			kill(getpid(), SIGTERM);
		}
		execvp(argv_exec[0], argv_exec);
	}								
	else{
		int status;
		waitpid(pid_executor, &status, 0);
	}
}

void execute_RR(struct message msg, int message_queue){ 
    char **argv_exec;
    int status;

    pid_t pid_executor;
    if(msg.pid != -1){ 
        pid_executor = msg.pid;
        kill(pid_executor, SIGCONT); 
    } else{ 
        pid_executor = fork();
        if(pid_executor == 0){
            argv_exec = tokenize(msg.string);
            if(argv_exec == NULL){
                printf("Error tokenizing the process to execute. Process automatically terminated.\n");
                exit(1);
            }
            execvp(argv_exec[0], argv_exec);
            exit(1); 
        }
    }

    pid_t pid_timer = fork();
    if(pid_timer == 0){
        usleep(4000000); 
        kill(pid_executor, SIGSTOP);
        exit(0);
    }

    waitpid(pid_executor, &status, WUNTRACED); 

    if(WIFSTOPPED(status)){
        msg.pid = pid_executor;
        msgsnd(message_queue, &msg, sizeof(struct message)-sizeof(long), 0);
        kill(pid_timer, SIGTERM);
        waitpid(pid_timer, NULL, 0);
    } 
    else {
        kill(pid_timer, SIGTERM);
        waitpid(pid_timer, NULL, 0);
    }
}

int main(int argc, char *argv[]){
	if(argc == 2){
		int fd = open(argv[1], O_RDONLY);
		dup2(fd, STDIN_FILENO);
		close(fd);
	}
	int message_queue = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
	
	int pid_scheduler = fork();
	
	if(pid_scheduler == 0){
		signal(SIGINT, SIG_IGN);
		struct message msg;
		
		signal(SIGUSR1, sigusr1_handler);
		
		int context_switches = 0;
		int N3 = 0, N2 = 0, N1 = 0;
		int total_processes = 0;
		
		while (!exit_flag) {
			int current_level = 1;
			
			while (current_level <= 5) {
				if (msgrcv(message_queue, &msg, sizeof(struct message)-sizeof(long), current_level, IPC_NOWAIT) != -1)
				{
					if(current_level == 1){
						if(msg.pid == -1){ 
							N1++; total_processes++; context_switches++; execute_RR(msg, message_queue);
						}else{
							context_switches++; execute_RR(msg, message_queue);
						}
					}
					else if(current_level == 2){ N2++; total_processes++; context_switches++; execute(msg);}
					else if(current_level == 3){ N2++; total_processes++; context_switches++; execute(msg);}
					else if(current_level == 4){ N2++; total_processes++; context_switches++; execute(msg);}
					else if(current_level == 5){ N3++; total_processes++; context_switches++; execute(msg);}
					current_level = 1;
				}else{
					current_level++;
				}
			}
		}
		
		printf("\n");
		printf("Total number of executed processes: %d\n", total_processes);
		printf("Total number of level 1 executed processes: %d\n", N1);
		printf("Total number of level 2 executed processes: %d\n", N2);
		printf("Total number of level 3 executed processes: %d\n", N3);
		printf("Total number of context switches: %d\n", context_switches);
		fflush(stdout);
		
		exit(0);
	}
	else{
		signal(SIGINT, sigint_handler);
		struct message inst;
		int status;
		while(scanf("%ld %d %[^\n]", &inst.level, &inst.priority, inst.string) != -1){
			inst.pid = -1;
			if(inst.level == 2 && inst.priority == 2){ inst.level = 3; }
			else if(inst.level == 2 && inst.priority == 3){ inst.level = 4; }
			else if(inst.level == 3){ inst.level = 5; }
			msgsnd(message_queue, &inst, sizeof(struct message) - sizeof(long), 0);
		}
		pause();
		
		kill(pid_scheduler, SIGUSR1);
		waitpid(pid_scheduler, &status, 0); 
		msgctl(message_queue, IPC_RMID, NULL);
		usleep(500000); /* Adjusted from sleep(0.5) */
	}
	
	return 0;
}