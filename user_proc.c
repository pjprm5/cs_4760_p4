// Paul Passiglia
// cs_4760
// Project 4
// 10/29/2020
// user_proc.c



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

// Global variables.
SharedInfo *sharedInfo;
int infoID;

// Message queue struct 
struct MessageQueue {
  long mtype;
  char messBuff[15];
};

int main (int argc, char *argv[])
{
  printf("user_proc.c begins... \n");

  // Allocate message queue ---------------------------------------
  struct MessageQueue messageQ;
  int msqID;
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


  





  shmdt(sharedInfo); // Detach shared memory.
  //shmctl(infoID, IPC_RMID, NULL); // Destory shared memory.

  return 0;
}
