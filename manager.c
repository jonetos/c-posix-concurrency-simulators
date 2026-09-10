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

void sigintHandler(int signum){
    printf("[MANAGER - %d] SIGINT received\n", getpid());
}

int main(int argc, char *argv[]){
    if(argc != 5){
        printf("[MANAGER - %d] Execution format: [./manager] [key] [period] [volume] [threshold]\n", getpid());
        exit(1);
    }

    signal(SIGINT, sigintHandler);
    printf("[MANAGER - %d] created\n", getpid());

    key_t key;
    key = ftok("/tmp", argv[1][0]);
    printf("[MANAGER - %d] key created: %d\n", getpid(), key);
    
    int messageQueueID;
    messageQueueID = msgget(key, IPC_CREAT | 0660);
    if(messageQueueID == -1){
        perror("[MANAGER] Error with msgget()\n");
        exit(-1);
    }
    printf("[MANAGER - %d] message queue created: %d\n", getpid(), messageQueueID);

    int sharedMemoryID;
    sharedMemoryID = shmget(key, sizeof(int), IPC_CREAT | 0660);
    if(sharedMemoryID == -1){
        perror("[MANAGER] Error with shmget()\n");
        exit(-1);
    }
    printf("[MANAGER - %d] shared memory created: %d\n", getpid(), sharedMemoryID);

    int* pumpCapacity; // To point to the shared memory
    pumpCapacity = (int*)shmat(sharedMemoryID, NULL, 0);
    if(pumpCapacity == (void*)-1){ 
        perror("[MANAGER] Error linking with shmat()\n");
        exit(-1);
    }
    printf("[MANAGER - %d] shared memory linked: %d\n", getpid(), sharedMemoryID);

    int semaphoreGroupID;
    semaphoreGroupID = semget(key, 2, IPC_CREAT | 0660);   
    if(semaphoreGroupID == -1){
        perror("[MANAGER] Error with semget()\n");
        exit(-1);
    }
    printf("[MANAGER - %d] semaphore group created: %d\n", getpid(), semaphoreGroupID);

	union semun {
		int val;
		struct semid_ds *buf; 
		unsigned short *array; 
	};
	
	union semun arg;
	arg.val = 1;
	
    int semaphoreValue;
    semaphoreValue = 0;
    if(semctl(semaphoreGroupID, 0, SETVAL, semaphoreValue) == -1){
        perror("[MANAGER] Error initializing provider tank semaphore with semctl()\n");
        exit(-1);
    }
    semaphoreValue = 1;
    if(semctl(semaphoreGroupID, 1, SETVAL, semaphoreValue) == -1){
        perror("[MANAGER] Error initializing shared memory access semaphore with semctl()\n");
        exit(-1);
    }
    printf("[MANAGER - %d] semaphore group initialized: %d\n", getpid(), semaphoreGroupID);

    *pumpCapacity = 0;
    printf("[MANAGER - %d] pumpCapacity initialized to %d\n", getpid(), *pumpCapacity);

    printf("[MANAGER - %d] required resources created.\n", getpid());

    // PIPE CREATION 
    int pipePumpFlowmeter[2];
    if(pipe(pipePumpFlowmeter) == -1){
        perror("[MANAGER] Error with pipe() PumpFlowmeter.\n");
        exit(-1);
    }
    printf("[MANAGER - %d] pipe Pump->Flowmeter created.\n", getpid());

    int pipeFlowmeterSink[2];
    if(pipe(pipeFlowmeterSink) == -1){
        perror("[MANAGER] Error with pipe() FlowmeterSink.\n");
        exit(-1);
    }
    printf("[MANAGER - %d] pipe Flowmeter->Sink created.\n", getpid());

	// PROCESS CREATION
	pid_t pump_pid, flowmeter_pid, sink_pid, monitor_pid;
	int standard_stdout = dup(STDOUT_FILENO); // Save a terminal output descriptor before dup2
	
	pump_pid = fork();
	if(pump_pid == 0){
		close(pipeFlowmeterSink[0]);
		close(pipeFlowmeterSink[1]);
		
		close(pipePumpFlowmeter[0]);
		dup2(pipePumpFlowmeter[1], STDOUT_FILENO);
		close(pipePumpFlowmeter[1]);
		
		dup2(standard_stdout, 3);
		close(standard_stdout);
		
		execl("./pump", "./pump", argv[1], argv[2], argv[3], NULL);
		perror("[MANAGER] Error executing pump child\n");
		exit(1);
	}
	
	flowmeter_pid = fork();
	if(flowmeter_pid == 0){
		close(pipePumpFlowmeter[1]);
		dup2(pipePumpFlowmeter[0], STDIN_FILENO);
		close(pipePumpFlowmeter[0]);
		
		close(pipeFlowmeterSink[0]);
		dup2(pipeFlowmeterSink[1], STDOUT_FILENO);
		close(pipeFlowmeterSink[1]);
		
		dup2(standard_stdout, 3);
		close(standard_stdout);
		
		execl("./flowmeter", "./flowmeter", argv[1], argv[4], NULL);
		perror("[MANAGER] Error executing flowmeter child\n");
		exit(1);
	}
	
	monitor_pid = fork();
	if(monitor_pid == 0){
		close(pipeFlowmeterSink[0]);
		close(pipeFlowmeterSink[1]);
		close(pipePumpFlowmeter[0]);
		close(pipePumpFlowmeter[1]);
		close(standard_stdout);
	
		execl("./monitor", "./monitor", argv[1], NULL);
		perror("[MANAGER] Error executing monitor child\n");
		exit(1);
	}
	
	sink_pid = fork();
	if(sink_pid == 0){
		close(pipePumpFlowmeter[0]);
		close(pipePumpFlowmeter[1]);
		close(standard_stdout);
		
		close(pipeFlowmeterSink[1]);
		dup2(pipeFlowmeterSink[0], STDIN_FILENO);
		close(pipeFlowmeterSink[0]);
		
		char pid_monitor_str[16];
		snprintf(pid_monitor_str, sizeof(pid_monitor_str), "%d", (int)monitor_pid);
		
		execl("./sink", "./sink", argv[1], pid_monitor_str, NULL);
		perror("[MANAGER] Error executing sink child\n");
		exit(1);
	}
	
	close(pipeFlowmeterSink[0]);
	close(pipeFlowmeterSink[1]);
	close(pipePumpFlowmeter[0]);
	close(pipePumpFlowmeter[1]);
	close(standard_stdout);
	
	pause();
	
	kill(pump_pid, SIGTERM);
	waitpid(pump_pid, NULL, 0);
	printf("[MANAGER - %d] Pump process with pid %d terminated\n", getpid(), pump_pid);
	
	kill(flowmeter_pid, SIGTERM);
	waitpid(flowmeter_pid, NULL, 0);
	printf("[MANAGER - %d] Flowmeter process with pid %d terminated\n", getpid(), flowmeter_pid);
	
	kill(sink_pid, SIGTERM);
	waitpid(sink_pid, NULL, 0);
	printf("[MANAGER - %d] Sink process with pid %d terminated\n", getpid(), sink_pid);
	
	kill(monitor_pid, SIGTERM);
	waitpid(monitor_pid, NULL, 0);
	printf("[MANAGER - %d] Monitor process with pid %d terminated\n", getpid(), monitor_pid);
	
	shmdt(pumpCapacity);
	shmctl(sharedMemoryID, IPC_RMID, NULL);
	printf("[MANAGER - %d] Shared memory destroyed\n", getpid());

	semctl(semaphoreGroupID, 0, IPC_RMID);
	printf("[MANAGER - %d] Semaphore group destroyed\n", getpid());

	msgctl(messageQueueID, IPC_RMID, NULL);
	printf("[MANAGER - %d] Message queue destroyed\n", getpid());

	return 0;
}