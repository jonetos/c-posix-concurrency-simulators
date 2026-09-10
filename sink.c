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

int main(int argc, char *argv[]){
	
	if (argc != 3) {
        printf("[SINK - %d] Execution format: [./sink] [key] [monitor_pid]\n", getpid());
        exit(1);
    }
	
	key_t key = ftok("/tmp", argv[1][0]);
	printf("[SINK - %d] key created: %d\n", getpid(), key);
	
	int sharedMemoryID;
	sharedMemoryID = shmget(key, sizeof(int), 0660);
	if (sharedMemoryID == -1) {
		perror("[SINK] Error with shmget()\n");
		exit(1);
	}
	printf("[SINK - %d] Shared memory ID obtained: %d\n", getpid(), sharedMemoryID);

	int* pumpCapacity;
	pumpCapacity = (int*)shmat(sharedMemoryID, NULL, 0);
	if (pumpCapacity == (void*) -1) {
		perror("[SINK] Error in shmat()\n");
		exit(1);
	}
	 
	int semaphoreGroupID;
    semaphoreGroupID = semget(key, 2, 0660);
    if(semaphoreGroupID == -1){
		perror("[SINK] Error with semget()\n");
		exit(1);
	}
	printf("[SINK - %d] Semaphore group ID obtained: %d\n", getpid(), semaphoreGroupID);
	
	struct sembuf op;
	op.sem_num = 1;   // semaphore 1 of the group
	op.sem_flg = 0;
	
    struct fluid fl;
	pid_t monitor_pid = atoi(argv[2]);
	
	while(1){
		if(read(0, &fl, sizeof(fl)) == sizeof(fl)){
			sleep(2);
			
			op.sem_op = -1;   // DECREMENT -> block if 0
			if (semop(semaphoreGroupID, &op, 1) == -1) {
				perror("[SINK] Error locking semaphore 1\n");
				exit(1);
			}
			
			*pumpCapacity -= fl.flow_rate;
			printf("[SINK - %d] Capacity now = %d\n", getpid(), *pumpCapacity);
			
			op.sem_op = +1;   // INCREMENT -> unblock setting to 1
			if (semop(semaphoreGroupID, &op, 1) == -1) {
				perror("[SINK] Error unlocking semaphore 1\n");
				exit(1);
			}
			
			kill(monitor_pid, SIGUSR1);
		} else{
			perror("[SINK] Error with read() from stdin\n");
			exit(1);
		}
	}
	return 0;
}