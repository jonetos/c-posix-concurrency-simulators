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
#include "structs.h"
#include <sys/sem.h>

int main(int argc, char *argv[]){
    if(argc != 4){
        dprintf(3,"[PUMP - %d] Execution format: [./pump] [key] [period] [volume]\n", getpid());
        exit(1);
    }

    int period = atoi(argv[2]);
    int volume = atoi(argv[3]);

    key_t key = ftok("/tmp", argv[1][0]);
    dprintf(3,"[PUMP - %d] key created: %d\n", getpid(), key);
    
	int semaphoreGroupID;
    semaphoreGroupID = semget(key, 2, 0660);
    if(semaphoreGroupID == -1){
		perror("[PUMP] Error with semget()\n");
		exit(1);
	}
	dprintf(3,"[PUMP - %d] Semaphore group ID obtained: %d\n", getpid(), semaphoreGroupID);
	
	int sharedMemoryID;
	sharedMemoryID = shmget(key, sizeof(int), 0660);
	if (sharedMemoryID == -1) {
		perror("[PUMP] Error with shmget()");
		exit(1);
	}
	dprintf(3,"[PUMP - %d] Shared memory ID obtained: %d\n", getpid(), sharedMemoryID);
	
	int* pumpCapacity;
	pumpCapacity = (int*)shmat(sharedMemoryID, NULL, 0);
	if (pumpCapacity == (void*) -1) {
		perror("[PUMP] Error in shmat()");
		exit(1);
	}

	int messageQueueID;
    messageQueueID = msgget(key, 0660);
    if(messageQueueID == -1){
        perror("[PUMP] Error with msgget()\n");
        exit(-1);
    }
    dprintf(3,"[PUMP - %d] message queue created: %d\n", getpid(), messageQueueID);
    
	struct sembuf op; 
	
	struct fluid flu;
	flu.flow_rate = volume;
	flu.counter = 0;
	
	struct message msg;
	msg.pid = getpid();
	msg.type = 1;
	snprintf(msg.text, sizeof(msg.text), "Supply problem in pump %d, insufficient flow", getpid());
        
    while(1){
        sleep(period);
        
        op.sem_num = 0; 
		op.sem_op = -volume;
		op.sem_flg = IPC_NOWAIT;

        if (semop(semaphoreGroupID, &op, 1) == -1){
			if (errno == EAGAIN) {
				dprintf(3,"[PUMP - %d] It was not possible to add the volume to the pump capacity\n", getpid());
				msgsnd(messageQueueID, &msg, sizeof(struct message)-sizeof(long), 0);
			} else {
				perror("[PUMP] Error with semop() on semaphore 0\n");
				exit(1);
			}
		} else {
			flu.counter++;
			if(write(1, &flu, sizeof(struct fluid)) == -1){
				perror("[PUMP] Error with write to standard output\n");
				exit(1);
			}
			
			op.sem_num = 1; 
			op.sem_op = -1;
			op.sem_flg = 0;

			if (semop(semaphoreGroupID, &op, 1) == -1) {
				perror("[PUMP] Error locking semaphore 1\n");
				exit(1);
			}
			
			*pumpCapacity += flu.flow_rate;
			dprintf(3,"[PUMP - %d] Capacity now = %d\n", getpid(), *pumpCapacity);
			
			op.sem_op = +1;
			
			if (semop(semaphoreGroupID, &op, 1) == -1) {
				perror("[PUMP] Error unlocking semaphore 1\n");
				exit(1);
			}
		}
    }
    return 0;
}