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
	if(argc != 3){
        dprintf(3,"[FLOWMETER - %d] Execution format: [./flowmeter] [key] [threshold]\n", getpid());
        exit(1);
    }
	
	int threshold = atoi(argv[2]);
	
	key_t key = ftok("/tmp", argv[1][0]);
    dprintf(3,"[FLOWMETER - %d] key created: %d\n", getpid(), key);
    
	int messageQueueID;
    messageQueueID = msgget(key, 0660);
    if(messageQueueID == -1){
        perror("[FLOWMETER] Error with msgget()\n");
        exit(-1);
    }
    dprintf(3,"[FLOWMETER - %d] message queue created: %d\n", getpid(), messageQueueID);
    
	struct fluid fl;
	struct message msg;
	msg.type = 2;
	msg.pid = getpid();
	snprintf(msg.text, sizeof(msg.text), "Excessive flow problem in %d", getpid());
	
	while(1){
		if(read(0, &fl, sizeof(struct fluid)) == sizeof(fl)){
			sleep(1);
			
			dprintf(3,"[FLOWMETER - %d] Flow received from the pipe = %d\n", getpid(), fl.flow_rate);
			
			if(write(1, &fl, sizeof(struct fluid)) != sizeof(fl)){
				perror("[FLOWMETER] Error in write()\n");
				exit(1);
			}
			
			if(fl.flow_rate > threshold){
				msgsnd(messageQueueID, &msg, sizeof(struct message)-sizeof(long), 0);
			}	
		} else {
			perror("[FLOWMETER] Error with read() from stdin\n");
			exit(1);
		}
	}
	return 0;
}