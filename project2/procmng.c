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
    cprintf("exec: fail\n");
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

  // Allocate two pages at the next page boundary.
  // Make the first inaccessible. //* 이게 가드용페이지 인거 같지?
  // Use the second as the user stack. //* 이게 스택용페이지.
  sz = PGROUNDUP(sz);
  if((sz = allocuvm(pgdir, sz, sz + stacksize*PGSIZE)) == 0)
    goto bad;
  clearpteu(pgdir, (char*)(sz - stacksize*PGSIZE));
  sp = sz;

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
    struct proc* p;
    int check = 0;
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if (p->pid == pid){
            check = 1;
            break;
        }
    }
    // error 2. 일치하는 pid 가 없는 경우
    if (!check){
        cprintf("pid unmatched\n");
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

///////////////////////////////////////////////////////////////////////////
//* find process with pid
struct proc*
find_proc(int pid)
{
    struct proc* p;
    for(p = ptable.proc; p<&ptable.proc[NPROC]; p++){
        if (p->pid == pid){ return p; }
    }
    cprintf("cannot find proc\n");
    return p;
}

//* pmanager kernel func : list
void list(void){
  int cnt = 0;
  for(struct proc* p = ptable.proc; p<&ptable.proc[NPROC]; p++){
    if ((p->isThread == 0) && (p->state == RUNNING)){ //! thread 인 경우를 고려해주어야 한다.
      cnt++;
      cprintf("RUNNNING process[%d]:\n", cnt);
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

//* pmanager wrapper : kill.. 
int sys_mykill(void){
  int pid;
  int res = argint(0,&pid);
  if (res==-1){
      panic("error: related to pid of kill\n");
  }
  kill(pid);
  struct proc* p = find_proc(pid);
  if (p->killed!=0){
      cprintf("kill successs\n");
      return 0;
  } else{
      cprintf("kill failed\n");
      return -1;
  }
}

//* pmanager wrapper : execute
int sys_execute(void){
    char* path;
    int stacksize;
    int res = argstr(0, &path);
    if (res==-1){
        panic("error: related to path of execute\n");
    }
    int res2 = argint(0,&stacksize);
    if (res2==-1){
        panic("error: related to stacksize of execute\n");
    }
    exec2(path, &path, stacksize); //! argv..?
    return 0;
}

//* pmanager wrapper : memlim (setmemorylimit)
int memlim(void){
    int pid, limit;
    int res = argint(0, &pid);
    if (res==-1){
        panic("error: related to pid of memlim\n");
    }
    int res2 = argint(1, &limit);
    if (res2==-1){
        panic("error: related to limit of memlim\n");
    }

    int sml = setmemorylimit(pid, limit);

    struct proc* p = find_proc(pid);
    if((p->memlim == limit) && (sml==1)){
        cprintf("memlim success\n");
        return 0;
    } else {
        cprintf("memlim failed\n");
        return -1;
    }
}

//* pmanager wrapper : exit
int sys_myexit(void){
    exit();
    return 0;
}