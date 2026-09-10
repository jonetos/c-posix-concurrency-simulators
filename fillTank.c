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

int main(int argc, char *argv[]){
	if(argc != 4){
		printf("[FILLTANK - %d] Execution format: [./fillTank] [key] [time] [volume]\n", getpid());
        exit(1);
    }
	
	int waitTime = atoi(argv[2]);
	int volume = atoi(argv[3]);
	
	key_t key = ftok("/tmp", argv[1][0]);
	printf("[FILLTANK - %d] key created: %d\n", getpid(), key);
	
	int semaphoreGroupID;
	semaphoreGroupID = semget(key, 2, 0660);
    if(semaphoreGroupID == -1){
		perror("[FILLTANK] Error with semget()\n");
		exit(1);
	}
	printf("[FILLTANK - %d] Semaphore group ID obtained: %d\n", getpid(), semaphoreGroupID);
	
	struct sembuf op;
	op.sem_num = 0;
	op.sem_op = +volume;
	op.sem_flg = 0;
	
	sleep(waitTime);
	
	if (semop(semaphoreGroupID, &op, 1) == -1) {
		perror("[FILLTANK] Error increasing volume on semaphore 0\n");
		exit(1);
	}
	
	printf("[FILLTANK - %d] Semaphore 0 refilled with volume \n", getpid());
	
	return 0;
}