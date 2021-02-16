/*Staffordshire University - School of Computing and Digital Technologies Department of Computing.
COCS50633
Systems Programming with C++
Task 1
C Programming Assignment
Author: Daniel Truman - 04908117
Filename: producer.c
Date: 18th December 2019*/


#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <wait.h>

/*Structure prototype*/
struct printJob{
	pid_t printNumber; /*Holds job number from child pid*/
	int printPriority; /*Holds randomly assigned number indicating priority*/
	int jobFlag; /*Used to indicate job status between programmes over shared memory*/
	int cycleFlag; /*Used to indicate cycle status between programmes over shared memory*/
};

/*Function prototypes*/
int writeLog(int number, int cycle, int priority);
void readLog();
int error (int errorCode);


int main(int argc, char *argv[]) 
{

/*Variables*/
key_t key;
size_t SHMSZ;
time_t t;
int shmem, i, jobNo, loopCount;
char prompt;
struct printJob *jobBatch;

	/*Error checking command line argument entry*/
	if(argc != 2)
	{
		error(1);
		return 0;
	}

	if(atoi(argv[1]) > 5)
	{
		error(2);
		return 0;
	}
    
	if(atoi(argv[1]) < 1)
	{
		error(2);
		return 0;
	}

	jobNo = atoi(argv[1]); /*Put argument value into variable jobNo once checked*/

	/*Begin creating shared memory*/
	key = ftok("sharedmemfile", 77); /*Unique key generated*/
	SHMSZ = sizeof(jobBatch); /*Shared mem size defined by struct*/ 
	/*printf("Size should be %ld\n\r", SHMSZ);*/ /*Will print defined size for debug purposes if uncommented*/
	shmem = shmget(key, SHMSZ, 0666 | IPC_CREAT); /*Get shared memeory*/
	
	if((jobBatch = shmat(shmem, NULL, 0)) == (void *) -1) /*Attach pointer to shared memory with error checking*/
	{
		perror("shmat");
		exit(1);
	}
	
	(*jobBatch).cycleFlag = 1; /*Set cycle flag as ready to cycle*/

	while((*jobBatch).cycleFlag != 0) /*Break out loop*/
	{
		printf("\n\rPrint producer - Ready to proceed.\n\r");
	    
		for(loopCount = 0; loopCount < 5; loopCount++) /*Loops five times before allowing option to end programme or loop again*/
		{
			
			(*jobBatch).jobFlag = 2; /*Sets job flag for consumer to proceed with delay to ensure completion*/
			sleep(1);
			printf("Proceeding.\n\n\r");
			(*jobBatch).jobFlag = 0; /*Set job flag as ready to send*/
			
		    for(i = 0; i < jobNo; i++) /*Does number of jobs based on command line input*/
		    {
				
				while((*jobBatch).jobFlag != 0); /*Wait here for flag*/
				
				if (fork() == 0) /*Begin fork and check to make sure child is used to write to shared memory*/
				{
					printf("Cycle: %d - Job No. %d is being generated.\n\r", loopCount + 1, i + 1);
					while((*jobBatch).jobFlag != 0);
					srand(time(&t) ^ (getpid() << 16)); /*seed rand() based on XOR of left shifted child pid for better chance of unique seed*/
					(*jobBatch).printNumber = (int)getpid(); /*Assign child pid to structure variable*/
					(*jobBatch).printPriority = rand() % (8 + 1 - 1) + 1; /*Random number between 1-8*/
					printf("Cycle: %d - Job ID: %d, priority: %d - Ready for consumer.\n\n\r", loopCount + 1, (*jobBatch).printNumber, (*jobBatch).printPriority);
					writeLog((*jobBatch).printNumber, loopCount + 1, (*jobBatch).printPriority); /*Create log entry for successful job*/
					(*jobBatch).jobFlag = 1; /*Set flag for consumer to collect*/
					exit(1);
				}
				
			    wait(NULL); /*Wait for any child process to fully complete*/
		    }
		    
		    while((*jobBatch).jobFlag != 3); /*Wait for flag change*/
		    sleep(2); /*Delay before looping*/
		}
		
		printf("Current cycles completed.\n\r");
		
		/*Check input is valid*/
		do
		{
			printf("\n\rPlease enter 1 to repeat another cycle or 2 to close.\n\r");
			prompt = getchar();
		}while(atoi(&prompt) != 1 && atoi(&prompt) != 2);
		
		/*Cycle again if 2 selected, else begin shutdown*/
		if(atoi(&prompt) == 2)
		{
			(*jobBatch).cycleFlag = 0;
			(*jobBatch).jobFlag = 2;
		}
			
		sleep(2); /*Ensure time for processes to complete before changing flag*/
	}
	
	/*Wait for consumer to signal shutdown*/
    printf("\n\rAll job cycles completed. Termination request sent to consumer.\n\r");
    while((*jobBatch).jobFlag != 4) /*Wait for consumer to change flag signalling shutdown*/
	sleep(1);   

	printf("Consumer successfully disconnected. Shutting down producer.\n\r");
    shmdt(jobBatch); /*Detach from shared memory*/
	shmctl(shmem,IPC_RMID,NULL); /*Destroy the shared memory*/

	/*Print log to screen and close*/
  	printf("\n\rShutdown completed. Displaying current logfile:\n\n\r");
  	readLog();
	return 0;
}

/*Function writes entries to file for log purposes*/
int writeLog(int number, int cycle, int priority)
{
	FILE *fptw;

	fptw = fopen("producerlog.txt","a"); /*filename define here, create and append*/

	/*Exit with error if cannot create or write to file*/	
	if (fptw == NULL)
	{
		fclose(fptw);
		error(3);
	}

	fprintf(fptw,"%d %d %d %lu\n\r", cycle, number, priority, (unsigned long)time(NULL)); /*Print to file with timestamp*/
	fclose(fptw);
	return(1);
}

/*Function prints current log to screen with relevant details*/
void readLog()
{
	FILE *fptr;
	int cycle, jobID, jobPriority;
	long unsigned int tStamp;

	/*Exit with error if cannot find file*/
	fptr = fopen("producerlog.txt","r"); /*filename defined here, read only*/
	if (fptr == NULL)
	{
		fclose(fptr);
		error(3);
	}
	
	/*Read through file until end of file and print to screen with descriptions*/
	while(fscanf(fptr, "%d %d %d %lu", &cycle, &jobID, &jobPriority, &tStamp) != EOF)
	{
		printf("Cycle:%2d,  Job ID:%5d,  Priority:%2d - Succesfully generated and sent at timestamp: %lu\n\r", cycle, jobID, jobPriority, tStamp);
	}
	
	fclose(fptr);
}

/*Function handles error scenarios with switch case*/
int error (int errorCode)
{
	switch(errorCode)
	{
		case 0: printf("\n\rMajor systems error.\n\n\r");
			exit(0);
			break;
		case 1: printf("Unknown argument. Try entering a value between 1-5 after executable.\n\r");
			break;
		case 2: printf("Please enter an amount of jobs between 1-5.\n\r");
			break;
		case 3: printf("\n\rCannot find file producerlog.txt.\n\r");
			exit(0);
			break;
	}
	return 0;
}
