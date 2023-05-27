#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "elf.h"
#include "spinlock.h"
#include "fcntl.h"

extern struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

//* find process with pid
struct proc*
find_proc(int pid)
{
    struct proc* p;
    for(p = ptable.proc; p<&ptable.proc[NPROC]; p++){
        if (p->pid == pid){ return p; }
    }
    return p;
}

int
exec2(char *path, char **argv, int stacksize)
{
  char *s, *last;
  int i, off;
  uint argc, sz, sp, ustack[3+MAXARG+1];
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;
  struct proc *curproc = myproc();

  begin_op();

  if((ip = namei(path)) == 0){
    end_op();
    cprintf("exec2: fail\n");
    return -1;
  }
  ilock(ip);
  pgdir = 0;

  // Check ELF header
  if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;
  if(elf.magic != ELF_MAGIC)
    goto bad;

  if((pgdir = setupkvm()) == 0)
    goto bad;

  // Load program into memory.
  sz = 0;
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
      goto bad;
    if(ph.vaddr % PGSIZE != 0)
      goto bad;
    if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;
  }
  iunlockput(ip);
  end_op();
  ip = 0;

  // Allocate n pages at the next page boundary.
  // Make the first inaccessible. Use the (n-1) as the user stacks.
  // n = stacksize+1
  curproc->stacksize = stacksize;
  sz = PGROUNDUP(sz);
  if((sz = allocuvm(pgdir, sz, sz + (stacksize+1)*PGSIZE)) == 0)
    goto bad;
  clearpteu(pgdir, (char*)(sz - (stacksize+1)*PGSIZE));
  sp = sz;
  cprintf("custom stacksize: %d\n", curproc->stacksize);
  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
    if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[3+argc] = sp;
  }
  ustack[3+argc] = 0;

  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = argc;
  ustack[2] = sp - (argc+1)*4;  // argv pointer

  sp -= (3+argc+1) * 4;
  if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
    goto bad;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(curproc->name, last, sizeof(curproc->name));

  // Commit to the user image.
  oldpgdir = curproc->pgdir;
  curproc->pgdir = pgdir;
  curproc->sz = sz;
  curproc->tf->eip = elf.entry;  // main
  curproc->tf->esp = sp;
  switchuvm(curproc);
  freevm(oldpgdir);
  return 0;

 bad:
  if(pgdir)
    freevm(pgdir);
  if(ip){
    iunlockput(ip);
    end_op();
  }
  return -1;
}

int
setmemorylimit(int pid, int limit)
{
    // error 1. limit가 음수인 경우
    if (limit < 0){
        cprintf("limit cannot be negative\n");
        return -1;
    }
    struct proc* p = find_proc(pid);
    // error 2. 일치하는 pid 가 없는 경우
    if (p->pid != pid){
      cprintf("error : find_proc fail\n");
      return -1;
    }
    // error 3. 기존 할당받은 메모리보다 limit이 작은 경우
    if (limit < p->memlim){ //! memlim 이 아니라 sz 로 봐야 하나?
        cprintf("limit is smaller than assigned memory\n");
        return -1;
    }
    // 정상 작동
    p->memlim = limit;
    //! 실제로 동작상의 limit를 걸어줘야 한다. 0이면 제한 없음. 양수면 제한.
    return 0;
}

//* for debugging: print process' state
void printstate(struct proc* p){
  switch(p->state){
    case RUNNABLE:
      cprintf("RUNNABLE\n");
      break;
    case RUNNING:
      cprintf("RUNNING\n");
      break;
    case EMBRYO:
      cprintf("EMBRYO\n");
      break;
    case ZOMBIE:
      cprintf("ZOMBIE\n");
      break;
    case UNUSED:
      cprintf("UNUSED\n");
      break;  
    case SLEEPING:
      cprintf("SLEEPING\n");
      break;
  }
}

//* pmanager kernel func : list
void list(void){
  int cnt = 0;
  for(struct proc* p = ptable.proc; p<&ptable.proc[NPROC]; p++){
    if ((p->isThread == 0)&&((p->state == RUNNABLE)||p->state == RUNNING)){
      cnt++;
      cprintf("process[%d] state: ", cnt);
      printstate(p);
      cprintf("name: %s, pid: %d, stack_page: %d, mem_size: %d, mem_limit: %d\n",
              p->name, p->pid, p->stacksize, p->sz, p->memlim);
    }
  }
}

//* pmanager wrapper : list
int sys_list(void){
  list();
  return 0;
}

//* pmanager wrapper : execute
int sys_exec2(void){
  char* path;
  int stacksize;
  char** argv;
  int res = argstr(0, &path);
  if (res==-1){
      cprintf("error: related to path of execute\n");
      return -1;
  }
  int res2 = argptr(1, (void*)&argv, sizeof(argv));
  if (res2==-1){
      cprintf("error: related to argv of execute\n");
      return -1;
  }
  int res3 = argint(2, &stacksize);
  if (res3==-1){
      cprintf("error: related to stacksize of execute\n");
      return -1;
  }

  exec2(path, argv, stacksize);
  return 0;
}

//* pmanager wrapper : memlim (== sys_setmemorylimit)
int memlim(void){
    int pid, limit;
    int res = argint(0, &pid);
    if (res==-1){
        cprintf("error: related to pid of memlim\n");
        return -1;
    }
    int res2 = argint(1, &limit);
    if (res2==-1){
        cprintf("error: related to limit of memlim\n");
        return -1;
    }

    int sml = setmemorylimit(pid, limit);

    struct proc* p = find_proc(pid);
    // sml 만 확인해도 되지만, 혹시 모르니까?
    if((p->memlim == limit) && (sml==0)){
        cprintf("memlim success\n");
        return 0;
    } else {
        cprintf("memlim failed\n");
        cprintf("sml: %d, p.memlim: %d, limit: %d\n", sml, p->memlim, limit);
        return -1;
    }
}