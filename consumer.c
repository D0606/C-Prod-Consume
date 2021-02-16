/*Staffordshire University - School of Computing and Digital Technologies Department of Computing.
COCS50633
Systems Programming with C++
Task 1
C Programming Assignment
Author: Daniel Truman - 04908117
Filename: consumer.c
Date: 18th December 2019*/

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define SHMSZ 8 /*Size of shared memory for structure should be 8 as checked by debug line from producer.c*/

/*Structure prototype*/
struct printJob{
	pid_t printNumber; /*Holds job number from child pid*/
	int printPriority; /*Holds randomly assigned number indicating priority*/
	int jobFlag; /*Used to indicate job status between programmes over shared memory*/
	int cycleFlag; /*Used to indicate cycle status between programmes over shared memory*/
};

/*Function prototypes*/
int bubbleSort(int count, struct printJob *jobList);
int writeLog(int count, int cycle, struct printJob *jobList);
int deleteJob(int count, int cycle, struct printJob *jobList);
void readLog();
int error (int errorCode);


int main(int argc, char *argv[]) 
{ 

/*Variables*/
size_t size;
key_t key;
int shmem, i, jobNo, loopCount, *num;
struct printJob *jobBatch;
struct printJob processJob[5];
	
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
	shmem = shmget(key, SHMSZ, 0666 | IPC_CREAT); /*Get shared memeory*/

	if((jobBatch = shmat(shmem, NULL, 0)) == (void *) -1) /*Attach pointer to shared memory with error checking*/
	{
		perror("shmat");
		exit(1);
	}

	(*jobBatch).cycleFlag = 1; /*Set cycle flag as ready to cycle*/

	loopCount = 1; /*Set Cycle counter before loop*/
	while((*jobBatch).cycleFlag != 0) /*Break out loop*/
	{
			printf("\n\rPrint consumer - Ready to proceed.\n\r");
		    while((*jobBatch).jobFlag != 1); /*Wait for job flag status*/
			printf("Proceeding.\n\n\r");
		    for(i = 0; i < jobNo; i++)
			    {
					if ((*jobBatch).jobFlag == 2) /*Break out of loop on job flag*/
					{
						break;
					}
					printf("Cycle: %d - Job No. %d is being retrieved.\n\r", loopCount, i + 1);
					while((*jobBatch).jobFlag != 1) /*Wait for job flag status to indicate ready to receive*/
					sleep(1);
					processJob[i].printNumber = (*jobBatch).printNumber;
					processJob[i].printPriority = (*jobBatch).printPriority;
					printf("Cycle: %d - Job ID: %d, priority: %d - Succesfully retrieved.\n\n\r", loopCount, processJob[i].printNumber, processJob[i].printPriority);
					(*jobBatch).jobFlag = 0; /*Set job flag status to indicate done*/
			    } 
						
			bubbleSort(jobNo, processJob); /*Sort retrieved jobs from array*/
	
			writeLog(jobNo, loopCount, processJob); /*Write to logs once sorted.*/
			
			deleteJob(jobNo, loopCount, processJob); /*Delete jobs once logged.*/

			/*Track cycle number*/
			if(loopCount == 5)
			{
				loopCount = 0;
			}
			loopCount++;

			printf("\n\rAwaiting instructions from producer.\n\r");
			(*jobBatch).jobFlag = 3;
		    while((*jobBatch).jobFlag != 2);
		
	}

	printf("\n\rConnection termination request received from producer. Closing connection.\n\r");
	(*jobBatch).jobFlag = 4; /*Set flag to indicate detaching*/
	shmdt(jobBatch); /*Detach from shared memory*/
	
	/*Print log to screen and close*/
	printf("\n\rConnection closed. Displaying current logfile:\n\n\r");
	readLog();
	return 0; 
}

/*Function sorts through array and organises by greater to lesser priority*/
int bubbleSort(int count, struct printJob *jobList)
{
	int tempPriority, tempJob, swapDone, i;

		while(1)
	    {
	        swapDone = 0;
	        for(i=0; i < count - 1; i++)
	        {
	            if(jobList[i].printPriority < jobList[i + 1].printPriority)
	            {
	                /*Sorting priority values*/ 
	                tempPriority = jobList[i].printPriority;
	                jobList[i].printPriority = jobList[i + 1].printPriority;
	                jobList[i + 1].printPriority = tempPriority;
	
	                /*Sorting Job ID values*/
	                tempJob = jobList[i].printNumber;
	                jobList[i].printNumber = jobList[i + 1].printNumber;
	                jobList[i + 1].printNumber = tempJob;
	                
	                swapDone = 1;
	            }
	        }
	
	        if(swapDone == 0)
	        {
	            break;
	        }
	    }
}

/*Function writes entries to file for log purposes*/
int writeLog(int count, int cycle, struct printJob *jobList)
{
	FILE *fptw;
	int i;

		fptw = fopen("consumerlog.txt","a"); /*filename define here, create and append*/

	/*Exit with error if cannot create or write to file*/	
	if (fptw == NULL)
	{
		fclose(fptw);
		error(3);
	}
	
	/*Print array to file with timestamp*/
	for(i=0; i < count; i++)
	{
		fprintf(fptw,"%d %d %d %lu\n\r", cycle, jobList[i].printNumber, jobList[i].printPriority, (unsigned long)time(NULL));
	}

	fclose(fptw);
	return(1);
}


/*Function overwrites jobs to remove them once retrieved*/
int deleteJob(int count, int cycle, struct printJob *jobList)
{
	for(int i = 0; i < count; i++)
	{
		printf("Cycle: %d - Job ID: %d, priority: %d - Succesfully deleted.\n\r", cycle, jobList[i].printNumber, jobList[i].printPriority);
		jobList[i].printNumber = 0;
		jobList[i].printPriority = 0;
	}
	return(1);
}


/*Function prints current log to screen with relevant details*/
void readLog()
{
	FILE *fptr;
	int cycle, jobID, jobPriority;
	long unsigned int tStamp;

	/*Exit with error if cannot find file*/
	fptr = fopen("consumerlog.txt","r"); /*filename defined here, read only*/
	if (fptr == NULL)
	{
		fclose(fptr);
		error(3);
	}
	
	/*Read through file until end of file and print to screen with descriptions*/
	while(fscanf(fptr, "%d %d %d %lu", &cycle, &jobID, &jobPriority, &tStamp) != EOF)
	{
		printf("Cycle:%2d,  Job ID:%5d,  Priority:%2d - Succesfully deleted at timestamp: %lu\n\r", cycle, jobID, jobPriority, tStamp);
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
		case 3: printf("\n\rCannot find file consumerlog.txt.\n\r");
			exit(0);
			break;
	}
	return 0;
}
