#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

#ifdef RT
int
sys_print(void)
{
  int flag;
  if(argint(0, &flag) < 0)
    return -1;
  
  print(flag);
  return 0;
}
int
sys_freeze(void)
{
  int f;
  if(argint(0, &f) < 0)
    return -1;
  
  freeze(f);
  return 0;
}
#endif

int
sys_fork(void)
{
  #ifdef RT

  #if RT
  int D, C;

  if(argint(0, &D) < 0)
    return -1;
  if(argint(1, &C) < 0)
    return -1;
  return fork(D, C);

  #else
  int P, O, D, C;
  if(argint(0, &D) < 0)
    return -1;
  if(argint(1, &C) < 0)
    return -1;
  if(argint(2, &P) < 0)
    return -1;
  if(argint(3, &O) < 0)
    return -1;
  return fork(D, C, P, O);
  #endif
  #else
  return fork();
  #endif
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
