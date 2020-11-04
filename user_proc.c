// Paul Passiglia
// cs_4760
// Project 4
// 10/29/2020
// user_proc.c



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
  unsigned int randomNum = ((rand() % (100 - 1 + 1)) + 1);
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

int main (int argc, char *argv[])
{
  printf("user_proc.c begins... \n");
  srand(time(0) ^ getpid()); // Seed random time.
  
  // Allocate message queue ---------------------------------------
  struct MessageQueue messageQ;
  key_t msqKey = ftok("user_proc.c", 666);
  msqID = msgget(msqKey, 0644 | IPC_CREAT);
  if (msqKey == -1)
  {
    perror("USER: Error: ftok failure msqKey");
    exit(-1);
  }

  // Allocate shared memory information ---------------------------
  key_t infoKey = ftok("makefile", 123);
  if (infoKey == -1)
  {
    perror("USER: Error: ftok failure");
    exit(-1);
  }

  // Create shared memory ID.
  infoID = shmget(infoKey, sizeof(SharedInfo), 0600 | IPC_CREAT);
  if (infoID == -1)
  {
    perror("USER: Error: shmget failure");
    exit(-1);
  }

  // Attach to shared memory.
  sharedInfo = (SharedInfo*)shmat(infoID, (void *)0, 0);
  if (sharedInfo == (void*)-1)
  {
    perror("USER: Error: shmat failure");
    exit(-1);
  }

  while (1)
  {
    if (msgrcv(msqID, &messageQ, sizeof(messageQ.messBuff), 1, 0) == 1) // Wait to receive message of type 1 for now.
    {
      printf("USER: Inside main loop. \n");
      unsigned int chanceForOption = randomTime(); // Populate random number to decide what happens to process below.
      printf("USER: chanceForOption = %u \n", chanceForOption);

      if (chanceForOption > 0  && chanceForOption <= 5) // 1. Process has determinted to terminate. 
      {
        printf("USER: Process is terminating. \n");
        strcpy(messageQ.messBuff, "1");
        messageQ.mtype = 2;
        msgsnd(msqID, &messageQ, sizeof(messageQ.messBuff), 0); // Send msg back of type 2 indicating the termination..
        shmdt(sharedInfo);
        return 0;
      }

      if (chanceForOption > 5 && chanceForOption <= 70) // 2. Process has determined to run it's entire quantum.
      {
        printf("USER: Process has run its quantum. \n");
        strcpy(messageQ.messBuff, "2");
        messageQ.mtype = 2;
        msgsnd(msqID, &messageQ, sizeof(messageQ.messBuff), 0); // Send msg back of type 2 indicating process has run it's entire quantum.
        shmdt(sharedInfo);
        return 0;
      }

      if (chanceForOption > 70 && chanceForOption <= 100);
      {
        printf("USER: Process has been I/O. \n");
        strcpy(messageQ.messBuff, "3");
        messageQ.mtype = 2;
        msgsnd(msqID, &messageQ, sizeof(messageQ.messBuff), 0); // Send msg back of type 2 indicating process has been interrupted.
        shmdt(sharedInfo);
        return 0;
      }
    }
    //messageQ.mtype = 2;
    //msgsnd(msqID, &messageQ, sizeof(messageQ.messBuff), 0);
  }

      // 1. small chance to terminate, report back to oss with small amount of time used and that we are terminating
      // 2. run for assigned time quantum, report back to oss with full time used 
      // 3. get an I/O interuppt in the middle of the time quantum
      // msqgrcv could wait on different mtypes depending on these options



 
  //shmdt(sharedInfo); // Detach shared memory.
  //shmctl(infoID, IPC_RMID, NULL); // Destory shared memory.

  return 0;
}
