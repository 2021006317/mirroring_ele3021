#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "elf.h"

extern struct proc* find_proc(int pid);

// void add_thrd(struct proc* p, struct thrd* thread){
//     if (thread == 0){
//         p->threads = thread;
//     }
//     else { // p.threads 에 추가 연결 해주기
//         struct thrd* cur_thread = p->threads;
//         while(cur_thread->next != 0){
//             cur_thread = cur_thread->next;
//         }
//         cur_thread->next = thread;
//     }
// }

//! fork 를 따라해봅시다. allocproc 을 따라해야 되나?
int thread_create(thread_t *thread, void*(*start_routine)(void*), void* arg){
    int pid = fork();
    struct proc* child = find_proc(pid);
    child->isThread = 1;
    child->tid = thread;
    child->start_routine = start_routine;

    return 0;
    //! return -1 하는 경우는 뭐가 있을까?
}
    
int sys_thread_create(void){
    return 0;
}

void thread_exit(void *retval){
   
}

int sys_thread_exit(void){
    return 0;
}

int thread_join(thread_t thread, void **retval){
    return 0;
}
int sys_thread_join(void){
    return 0;
}