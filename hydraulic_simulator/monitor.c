#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/sem.h>
#include "structs.h"

void SIGUSR1_handler(int dummy){}

int main(int argc, char *argv[]){
	if(argc != 2){
		printf("[MONITOR - %d] Execution format: [./monitor] [key]\n", getpid());
        exit(1);
    }
    
    key_t key = ftok("/tmp", argv[1][0]);
	printf("[MONITOR - %d] key created: %d\n", getpid(), key);
	
    pid_t monitorB = fork();
    
    if(monitorB == 0){ // Monitor B child
		int messageQueueID;
		messageQueueID = msgget(key, 0660);
		if(messageQueueID == -1){
			perror("[MONITOR-B] Error with msgget()\n");
			exit(-1);
		}
		printf("[MONITOR-B - %d] Message queue ID obtained: %d\n", getpid(), messageQueueID);
		
		struct message msg;
		while (1) {
			if (msgrcv(messageQueueID, &msg, sizeof(struct message)-sizeof(long), -2, 0) != -1){
				printf("[MONITOR-B - %d] Message %s from process %d\n", getpid(), msg.text, msg.pid);
			}
		}
	} else { // Monitor A parent
		signal(SIGUSR1, SIGUSR1_handler);
		
		int semaphoreGroupID;
		semaphoreGroupID = semget(key, 2, 0660);
		if(semaphoreGroupID == -1){
			perror("[MONITOR-A] Error with semget()\n");
			exit(1);
		}
		printf("[MONITOR-A - %d] Semaphore group ID obtained: %d\n", getpid(), semaphoreGroupID);
		
		int sharedMemoryID;
		sharedMemoryID = shmget(key, sizeof(int), 0660);
		if (sharedMemoryID == -1) {
			perror("[MONITOR-A] Error with shmget()\n");
			exit(1);
		}
		printf("[MONITOR-A - %d] Shared memory ID obtained: %d\n", getpid(), sharedMemoryID);

		int* pumpCapacity;
		pumpCapacity = (int*)shmat(sharedMemoryID, NULL, 0);
		if (pumpCapacity == (void*) -1) {
			perror("[MONITOR-A] Error in shmat()\n");
			exit(1);
		}
		
		struct sembuf op;
		op.sem_num = 1;
		op.sem_flg = 0;
			
		while(1){
			pause();
			op.sem_op = -1;
			if (semop(semaphoreGroupID, &op, 1) == -1) {
				perror("[MONITOR-A] Error locking semaphore 1\n");
				exit(1);
			}
			
			printf("[MONITOR-A - %d] %d liters present in the pump\n", getpid(), *pumpCapacity);
			
			op.sem_op = +1;
			if (semop(semaphoreGroupID, &op, 1) == -1) {
				perror("[MONITOR-A] Error unlocking semaphore 1\n");
				exit(1);
			}
		}
	}
	return 0;
}