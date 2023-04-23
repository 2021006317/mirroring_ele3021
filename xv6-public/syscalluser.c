#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

extern struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

extern uint ticks; // global tick
extern struct queue queueLzero;

int isLocked = 0; 
int isUnlocked = 0;

extern struct proc* me;

// 현재 실행중인 프로세스가 속한 queue level 반환
int getLevel(void)
{
    int lv = myproc()->ql;
    return lv;
}

// ptable을 돌면서 pid가 일치하는 process를 찾아 priority 설정
void setPriority(int pid, int priority)
{   
    if (priority>3 || priority<0) panic("setPriority error");
    for(struct proc* p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->pid == pid){
            p->pr = priority;
            break;
        }
    }
}

void schedulerLock(int password)
{
    me = myproc();

    if (password == 2021006317){
        isLocked = 1;
        acquire(&tickslock);
        ticks=0; // gt 초기화
        wakeup(&ticks);
        release(&tickslock);
    }
    else {
        cprintf("[Lock failed] pid: %d, time quantum: %d, qLevel: %d\n",
                me->pid, me->lt, me->ql);
        isLocked=0;
        kill(me->pid);
    }
}

void schedulerUnlock(int password)
{
    me = myproc();
    // struct cpu *c = mycpu();

    if (password == 2021006317){
        isUnlocked = 1;
        isLocked = 0;
        
        // switchuvm(p);
        // p->state = RUNNABLE; //? 몰루게따~
        // swtch(&(c->scheduler), p->context); // gt++
        // switchkvm();
        // p->lt++;

        // if (isThere(&queueLzero, p)!=1) queueLzero.count++;
        // struct proc* oldFront = queueLzero.front;
        // queueLzero.front=p;
        // p->next=oldFront;
        // p->pr=3;
        // p->lt=0;
    }
    else {
        cprintf("[Unlock failed] pid: %d, time quantum: %d, qLevel: %d\n",
                me->pid, me->lt, me->ql);
        kill(me->pid);
    }
}

//* Wrap for user

int sys_getLevel(void)
{
    return getLevel();
}

int sys_setPriority(void)
{
    int pid;
    int priority;
    
    if ((argint(0, &pid) < 0)||argint(1,&priority)<0) return -1;
    setPriority(pid, priority);
    cprintf("sys_setPriority\n");
    return 0;
}

int sys_schedulerLock(void)
{
    int pw;

    if (argint(0, &pw) < 0) return -1;
    schedulerLock(pw);
    cprintf("sys_schedulerLock\n");
    return 0;
    
}
int sys_schedulerUnlock(void)
{
    int pw;

    if (argint(0, &pw) < 0) return -1;
    schedulerUnlock(pw);
    cprintf("sys_schedulerUnlock\n");
    return 0;
}

int my_yield(void){
    yield();
    return 0;
}