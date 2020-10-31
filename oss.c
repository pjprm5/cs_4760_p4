// Paul Passiglia
// cs_4760
// Project 4
// 10/29/20
// oss.c



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
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
  char messBuff[15];
};

#define maxTimeBetweenNewProcsNS 500000000
#define maxTimeBetweenNewProcsSecs 1


int main (int argc, char *argv[])
{

  printf("oss.c begins....\n");

  int proc_count = 0;

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

  sharedInfo->arrayPCB[0].totalCPUtime = 100;
  printf("Total CPU Time: %u \n", sharedInfo->arrayPCB[0].totalCPUtime);









  shmdt(sharedInfo); // Detach shared memory.
  shmctl(infoID, IPC_RMID, NULL); // Destroy shared memory.
  msgctl(msqID, IPC_RMID, NULL); // Destroy message queue.
  return 0;
}
