#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_LOOP 100000
#define NUM_YIELD 20000
#define NUM_SLEEP 1000

#define NUM_THREAD 4
#define MAX_LEVEL 5

int parent;

int fork_children()
{
  int i, p;
  for (i = 0; i < NUM_THREAD; i++)
    if ((p = fork()) == 0)
    {
      sleep(10);
      return getpid();
    }
  return parent;
}

int fork_children2()
{
  int i, p;
  for (i = 0; i < NUM_THREAD; i++)
  {
    if ((p = fork()) == 0)
    {
      sleep(300);
      return getpid();
    }
    else
    {
      setPriority(p, i);
      printf(1, "setpriority success, pid : %d, pr : %d\n", p,i);
    }
  }
  return parent;
}

int max_level;

int fork_children3()
{
  int i, p;
  for (i = 0; i < NUM_THREAD; i++)
  {
    if ((p = fork()) == 0)
    {
      sleep(10);
      max_level = i;
      return getpid();
    }
  }
  return parent;
}
void exit_children()
{
  if (getpid() != parent)
    exit();
  while (wait() != -1);
}

int main(int argc, char* argv[]){
  if (argc == 1) {
    int i, pid;
    int count[MAX_LEVEL] = {0};
  //  int child;

    parent = getpid();

    printf(1, "MLFQ test start\n");
    printf(1, "[Test 1] default\n");
    pid = fork_children();

    if (pid != parent)
    {
      for (i = 0; i < NUM_LOOP; i++)
      {
        int x = getLevel();
        if (x < 0 || x > 4)
        {
          printf(1, "Wrong level: %d\n", x);
          exit();
        }
        count[x]++;
      }
      printf(1, "Process %d\n", pid);
      for (i = 0; i < MAX_LEVEL; i++)
        printf(1, "L%d: %d\n", i, count[i]);
    }
    exit_children();
    printf(1, "[Test 1] finished\n");
    printf(1, "done\n");
    exit();
  }

  // interrupt 129
  else if (strcmp(argv[1], "autolock") == 0) {
    __asm__("int $129");
    printf(1, "IRQ_LOCK success\n");
    exit();
  }
  // interrupt 130
  else if (strcmp(argv[1], "autounlock") == 0){
    __asm__("int $130");
    printf(1, "IRQ_UNLOCK success\n");
    exit();
  }
  // schedulerLock in user mode
  else if (strcmp(argv[1], "lock")==0){
    int pw = atoi(argv[2]);
    schedulerLock(pw);
    printf(1, "schedulerLock success\n");
    exit();
  }
  // lock, unlock in function
  else if (strcmp(argv[1], "lockunlocktest")==0){
    schedulerLock(2021006317);
    printf(1, "running process...\n");
    schedulerUnlock(2021006317);
    printf(1, "lock-unlock success\n");
    exit();
  }
  // setPriority in user mode
  else if (strcmp(argv[1], "setPriority")==0){
    int setpid = atoi(argv[2]);
    int setpr = atoi(argv[3]);
    setPriority(setpid, setpr);
    printf(1, "setPriority success, pid : %d , pr : %d\n", setpid, setpr);
    exit();
  }
  // getLevel in user mode
  else if (strcmp(argv[1], "getLevel")==0){
    int pid = getpid();
    int lv = getLevel();
    printf(1, "getLevel success. pid : %d, level : %d\n", pid, lv);
    exit();
  }
  // call yield in user mode
  else if (strcmp(argv[1], "yield")==0){
    yield();
    printf(1, "call yield success\n");
  }
  else{exit();}
}