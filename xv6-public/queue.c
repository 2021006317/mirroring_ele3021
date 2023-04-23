// def queue with linkedList
#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

/* queue의 형태
    front -> ... -> rear -> 0
    하나만 있다면 front=rear=p, rear->next=0
*/

void init_queue(struct queue* que, int level){
    que->front = (void*)0;
    que->rear = (void*)0;
    que->count = 0;
    que->level = level;
    que->tq = level*2+4;
}

void checkEmpty(struct queue* queue, int* num){ //update exist variable
    struct queue que = *queue;
    if (que.count==0) *num=0; //empty
    else *num=1; // not empty
}

int isThere(struct queue* que, struct proc* p){
    int exist = 0;
    for (struct proc* q = que->front; q!=0;q=q->next){
        if (q==p) {exist=1; break;}
    }
    return exist;
}

void debugQ(struct queue* que){
    // cprintf("debugQ\n");
    for (struct proc* p =que->front; p!=0;p=p->next){
        cprintf("debug pid %d\n", p->pid);
    }
}

void push(struct queue* to, struct proc* newRear){ // 큐 맨 뒤에 삽입
    if (to->count == 0) {
        to->front = newRear;
        to->front->next =0;
        to->rear = newRear;
        to->rear->next = 0;
        to->count++;
    }
    else{
        struct proc* oldRear = to->rear;
        to->rear = newRear;
        oldRear->next = newRear;
        to->rear->next = (void*)0;
        to->count++;
    }
}

void del(struct queue* from, struct proc* p){ // linked list 에서 p 삭제
    if (from->count == 0) {panic("pop error");}
    else{
        for (struct proc* q = from->front; q!=0; q=q->next){
            if (q==from->front && q==p){ // p=front 일 때
                from->front = from->front->next;
                from->count--;
                break;
            }
            else if (q->next == from->rear && from->rear==p) { // p=rear 일 때
                from->rear = q;
                q->next = 0;
                from->count--;
                break;
            }
            else if (q->next == p){
                q->next = p->next;
                from->count--;
                break;
            }
        }
    }
}