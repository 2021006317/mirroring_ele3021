#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "elf.h"
#include "spinlock.h"

extern struct proc* find_proc(int pid);
extern struct proc* initproc;

extern void forkret(void);
extern void trapret(void);

extern struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

int nexttid=1;

struct proc* allocate(){
  struct proc* curproc=myproc();
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = curproc->pid;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trapframe.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;
  return p;
}

//! fork + exec 을 따라해봅시다.
int thread_create(thread_t *thread, void*(*start_routine)(void*), void* arg){
  struct proc* curproc = myproc();
  struct proc* thrd;
  uint sp, ustack[2];
  if((thrd = allocate()) == 0){ // Allocate process.
      return -1;
  }
  struct proc* parent = curproc->parent;

  if(curproc->isThread == 1) curproc = parent;

  acquire(&ptable.lock);
  // thread를 위한 페이지 (stack, guard), 할당해주기
  curproc->sz = PGROUNDUP(curproc->sz);
  if((curproc->sz = allocuvm(curproc->pgdir, curproc->sz, curproc->sz + 2*PGSIZE)) == 0){
    release(&ptable.lock);
    cprintf("error : stackpage alloc\n");
    return -1;
  }
  clearpteu(curproc->pgdir, (char*)(curproc->sz - 2*PGSIZE));
  sp = curproc->sz;

  // page table 공유하기
  thrd->pgdir = curproc->pgdir;
  thrd->sz = curproc->sz;
  *thrd->tf = *curproc->tf;

  // user stack
  ustack[0] = 0xffffffff;
  ustack[1] = (uint)arg;
  sp -= 2*4;
  if(copyout(curproc->pgdir, sp, ustack, 2*4) < 0){
    release(&ptable.lock);
    cprintf("error : ustack fail\n");
    return -1;
  }

  // start routine
  thrd->tf->eip = (uint)start_routine;
  thrd->tf->esp = sp;

  //* curproc이 스레드일 때 새로운 thread를 생성하고자 하면, curproc의 부모의 자식이 되도록 한다.
  if (curproc->isThread==1){
    thrd->parent = curproc->parent;
  }
  // curproc이 메인스레드인 경우 그 자식이 되도록 한다.
  else{
    thrd->parent = curproc;
  }
  
  //* 스레드 관련 수정
  thrd->isThread = 1;
  thrd->tid = nexttid++;
  *thread = thrd->tid;

  release(&ptable.lock);
  
  // Clear %eax so that fork returns 0 in the child
  thrd->tf->eax = 0;
  for(int i=0; i<NOFILE; i++){
    if(curproc->ofile[i]){
      thrd->ofile[i] = filedup(curproc->ofile[i]);
    }
  }
  thrd->cwd = idup(curproc->cwd);
  safestrcpy(thrd->name, curproc->name, sizeof(curproc->name));

  acquire(&ptable.lock);

  thrd->state = RUNNABLE;

  release(&ptable.lock);
  return 0;
}

int sys_thread_create(void){
  thread_t *thread;
  void* arg;
  void*(*start_routine)(void*)=0;
  int res1 = argptr(0, (void*)&thread, sizeof(thread));
  int res2 = argptr(1, (void*)&start_routine, sizeof(start_routine));
  int res3 = argptr(2, (void*)&arg, sizeof(arg));

  if((res1==-1)||(res2==-1)||(res3==-1)){
    return -1;
  }
  return thread_create(thread, start_routine, arg);
}

// The ptable lock must be held.
void
wake(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

//! exit을 따라해봅시다.
void thread_exit(void *retval){
  struct proc *curproc = myproc();
  // struct proc *child;
  int fd;

  if(curproc->pid==1)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // wakeup1
  wake(curproc->parent);
  
  curproc->retval = retval;
  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

int sys_thread_exit(void){
  void* retval;
  if(argptr(0, (void*)&retval, sizeof(retval))==-1){
    return -1;
  }
  thread_exit(retval);
  return 0;
}

//* 자원 회수 함수. defs.h 에 작성해서 exec 에서도 쓸 수 있도록 함.
// 페이지 테이블, 스택, 메모리 등
void recovery(struct proc* thread){
  kfree(thread->kstack);
  thread->kstack = 0;
  thread->pid = 0;
  thread->parent = 0;
  thread->name[0] = 0;
  thread->killed = 0;
  thread->state = UNUSED;

  thread->isThread = 0;
  thread->tid = 0;
}

//! wait을 따라해봅시다.
// 원래 wait 은 curproc이 불렀던 함수임. 자식의 종료를 기다림.
// 지금은 그건 모르겠고 thread를 tid로 가지는 스레드의 종료를 기다림.
// 아무튼 목적은 자원회수하는 함수로, 얘를 부르는 건 메인스레드.
int thread_join(thread_t thread, void **retval){
  struct proc *p;
  struct proc *curproc = myproc(); //curproc

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->tid != thread)
        continue;
      
      //* 내가 원하는 스레드인 경우
      // 잘못된 호출
      if (p->parent != curproc){
        release(&ptable.lock);          
        cprintf("error : this proc is not parent of the thread\n");
        return -1;
      }
      // 정상 호출. 자원회수
      if(p->state == ZOMBIE){
        recovery(p);
        *retval = p->retval;
        release(&ptable.lock);
        return 0;
      }
    }

    if(curproc->killed){
      release(&ptable.lock);
      cprintf("error : parent has been already killed\n");
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

int sys_thread_join(void){
  thread_t thread;
  void** retval;
  int res1 = argint(0,(int*)&thread);
  int res2 = argptr(1, (void*)&retval, sizeof(retval));
  
  if ((res1==-1)||(res2==-1)){
    cprintf("error : sys_thread_join\n");
    return -1;
  }
  return thread_join(thread, retval);
}