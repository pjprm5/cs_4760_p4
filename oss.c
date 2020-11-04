// Paul Passiglia
// cs_4760
// Project 4
// 10/29/20
// oss.c



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "sharedinfo.h"

unsigned int randomTime()
{
  unsigned int randomNum = ((rand() % (1500000000 - 1000000 + 1)) +1);
  return randomNum;
}

// Global variables.
SharedInfo *sharedInfo;
int infoID;
int msqID;

// Message queue struct
struct MessageQueue {
  long mtype;
  char messBuff[1];
};

#define maxTimeBetweenNewProcsNS 500000000
#define maxTimeBetweenNewProcsSecs 1


int main (int argc, char *argv[])
{

  printf("oss.c begins....\n");

  int proc_count = 0;
  
  // Timespec struct for nanosleep.
  struct timespec tim1, tim2;
  tim1.tv_sec = 0;
  tim1.tv_nsec = 10000L;

  // Allocate message queue -------------------------------------------
  struct MessageQueue messageQ;
  int msqID;
  key_t msqKey = ftok("user_proc.c", 666);
  msqID = msgget(msqKey, 0644 | IPC_CREAT);
  if (msqKey == -1)
  {
    perror("OSS: Error: ftok failure msqKey");
    exit(-1);
  }

  messageQ.mtype = 1; // Declare initial mtype as 1 for ./oss

  strcpy(messageQ.messBuff, "0"); // Put msg in buffer
  
  if(msgsnd(msqID, &messageQ, sizeof(messageQ.messBuff), 0) == -1)
  {
    perror("OSS: Error: msgsnd ");
    exit(-1);
  }
  
  // Allocate shared memory information -------------------------------
  key_t infoKey = ftok("makefile", 123); 
  if (infoKey == -1)
  {
    perror("OSS: Error: ftok failure");
    exit(-1);
  }
  
  // Create shared memory ID.
  infoID = shmget(infoKey, sizeof(SharedInfo), 0600 | IPC_CREAT);
  if (infoID == -1)
  {
    perror("OSS: Error: shmget failure");
    exit(-1);
  }

  // Attach to shared memory.
  sharedInfo = (SharedInfo*)shmat(infoID, (void *)0, 0);
  if (sharedInfo == (void*)-1)
  {
    perror("OSS: Error: shmat failure");
    exit(-1);
  }

  // Begin launching child processes -----------------------------------  
  // Set shared clock to zero
  sharedInfo->secs = 0;
  sharedInfo->nanosecs = 0;

  // Seed shared clock to a random time for scheduling first process
  srand(time(0)); // seed random time
  unsigned int randomTimeToSchedule = randomTime();
  printf("Random time: %u \n", randomTimeToSchedule);
  

  sharedInfo->secs = sharedInfo->secs + (randomTimeToSchedule/1000000000);
  if (randomTimeToSchedule >= 1000000000)
  {
    sharedInfo->nanosecs = randomTimeToSchedule % 1000000000;
  }
  else
  {
    sharedInfo->nanosecs = randomTimeToSchedule;
  }

  printf("Time 1st child launched --> %u:%u \n", sharedInfo->secs, sharedInfo->nanosecs);

  // Launch first child -----------------------------------------------
  pid_t childPid; 
  int i;
  for (i = 0; i < 1; i++)
  {
    proc_count++;
    childPid = fork();
    if (childPid == 0)
    {
      char *args[] = {"./user_proc", NULL};
      execvp(args[0], args);
    }
    else if (childPid < 0)
    {
      perror("Error: Fork() failed");
    }
    else if ((childPid > 0) && (i >= 0))
    {
      FILE* fptr = (fopen("output", "a"));
      if (fptr == NULL)
      {
        perror("OSS: Error: ftpr error");
      }
      fprintf(fptr, "OSS(%d): Creating child: %d at time: %u:%u \n", getpid(), childPid, sharedInfo->secs, sharedInfo->nanosecs);
      printf("OSS(%d): Creating child: %d at time: %u:%u \n", getpid(), childPid, sharedInfo->secs, sharedInfo->nanosecs);
      fclose(fptr);
    }
  }

  //sharedInfo->arrayPCB[0].totalCPUtime = 100;
  //printf("Total CPU Time: %u \n", sharedInfo->arrayPCB[0].totalCPUtime);
  
  // Parent/Main loop
  while (1)
  {
    printf("OSS: Inside main loop. \n");
    
    // If terminated 
    msgrcv(msqID, &messageQ, sizeof(messageQ.messBuff), 2, 0);
    if (sharedInfo->nanosecs >= 1000000000) // Fix the clock
    {
      sharedInfo->secs = sharedInfo->secs + sharedInfo->nanosecs/1000000000; // Add seconds to nanosecs after nanosecs gets integer division.
      sharedInfo->nanosecs = sharedInfo->nanosecs % 1000000000; // Modulo operation to get nanosecs.
    }
    if (strcmp(messageQ.messBuff, "1") == 0)
    {
      // Process terminated, wait for process, then log what was terminated and launch a new process.
      printf("OSS: Process Terminated. \n");
      printf("Process time = %u \n", sharedInfo->arrayPCB[0].totalCPUtime);
      wait(NULL);
      break;
    }
    
    
    // If ran entire quantum.
    //msgrcv(msqID, &messageQ, sizeof(messageQ.messBuff), 3, IPC_NOWAIT);
    if (strcmp(messageQ.messBuff, "2") == 0)
    {
      wait(NULL);
      printf("OSS: Process Ran Entire Quantum. \n");
      sharedInfo->nanosecs = sharedInfo->nanosecs + 10000000; // Update nanosecs.
      sharedInfo->arrayPCB[0].totalCPUtime = sharedInfo->arrayPCB[0].totalCPUtime + 10000000; // Update total CPU time.
      sharedInfo->arrayPCB[0].timeInSystem = 10000000;
      sharedInfo->arrayPCB[0].timeLastBurst = 10000000;
      
      // Send out message for user_proc to receive
      //messageQ.mtype = 1;
      //msgsnd(msqID, &messageQ, sizeof(messageQ.messBuff), 0);
      
      childPid = fork();
      if (childPid == 0)
      {
        char *args[] = {"./user_proc", NULL};
        execvp(args[0], args);

      }
      else if (childPid < 0)
      {
        while(nanosleep(&tim1, &tim2));
        //perror("OSS: Error: Fork() failed inside main loop. \n");
      }
      else if (childPid > 0)
      {
        FILE* fptr = (fopen("output", "a"));
        if (fptr == NULL)
        {
          perror("OSS: Error: fptr error. \n");
        }
        fprintf(fptr, "OSS(%d): Creating Child: %d at time: %u:%u \n", getpid(), childPid, sharedInfo->secs, sharedInfo->nanosecs);
        printf("OSS(%d): Creating Child: %d at time: %u:%u \n", getpid(), childPid, sharedInfo->secs, sharedInfo->nanosecs);
        fclose(fptr);

        //messageQ.mtype = 1;
        //msgsnd(msqID, &messageQ, sizeof(messageQ.messBuff), 0);
      }
      //messageQ.mtype = 1;
      //msgsnd(msqID, &messageQ, sizeof(messageQ.messBuff), 0);
    }
     
    
    // If interrupted.
    //msgrcv(msqID, &messageQ, sizeof(messageQ.messBuff), 4, IPC_NOWAIT);
    if (strcmp(messageQ.messBuff, "3") == 0)
    {
      wait(NULL);
      printf("OSS: Process was interrupted. \n");
      // Process was interrupted, put into a blocked queue and schedule another process.

      // Send out message for user_proc to receive
      //messageQ.mtype = 1;
      //msgsnd(msqID, &messageQ, sizeof(messageQ.messBuff), 0);
      childPid = fork();
      if (childPid == 0)
      {
        char *args[] = {"./user_proc", NULL};
        execvp(args[0], args);
        
      }
      else if (childPid < 0)
      {
        while(nanosleep(&tim1, &tim2));
        //perror("OSS: Error: Fork() failed inside main loop. \n");
      }
      else if (childPid > 0)
      {
        FILE* fptr = (fopen("output", "a"));
        if (fptr == NULL)
        {
          perror("OSS: Error: fptr error. \n");
        }
        fprintf(fptr, "OSS(%d): Creating Child: %d at time: %u:%u \n", getpid(), childPid, sharedInfo->secs, sharedInfo->nanosecs);
        printf("OSS(%d): Creating Child: %d at time: %u:%u \n", getpid(), childPid, sharedInfo->secs, sharedInfo->nanosecs);
        fclose(fptr);

        //messageQ.mtype = 1;
        //msgsnd(msqID, &messageQ, sizeof(messageQ.messBuff), 0);
      }

    }
    messageQ.mtype = 1;
    msgsnd(msqID, &messageQ, sizeof(messageQ.messBuff), 0);        
  }


 






  shmdt(sharedInfo); // Detach shared memory.
  shmctl(infoID, IPC_RMID, NULL); // Destroy shared memory.
  msgctl(msqID, IPC_RMID, NULL); // Destroy message queue.
  return 0;
}
