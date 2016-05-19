#include "types.h"
#include "defs.h"
#include "heap.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#define TAM 50000 // TAM*NPROC = statistic

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
  #ifdef RT
  struct proc *queue[NPROC]; // Ready queue 
  #endif
} ptable;


#ifdef RT
int miss;
uint itr_q; // Iterator in ready queue
uint itr_s; // Iterator in statistic
uint itr_p; // Iterator in preempt
uint frozen;
struct statistic statistic[TAM];  

void getData(struct proc *p);
void printQueue(); // aux
void freeze(int);
int  print(int);
#endif

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)

{  initlock(&ptable.lock, "ptable");
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
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
  p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;
  
  // Leave room for trap frame.
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
  #ifdef RT
  interrupt = 0;
  p->D = 0;
  p->C = 0;
  p->ctxswt = 0;
  p->arrtime = 0;       // Tempo de chegada
  p->firstsch = 0;      // Instante do primeiro escalonamento
  #if !RT
  p->P = 0;
  p->O = 0;
  #endif
  #endif

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];
  
  p = allocproc();
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;
  #ifdef RT
  miss = 0;
  acquire(&ptable.lock);
  itr_q = 0;
  itr_s = 0;
  p->D = 100;                            // First user process, requires a start
  p->C = 100;
  #if !RT
  p->P = 1;
  p->O = 1;     
  frozen = 0;   // Sinaliza a criação de processos do sistema
  #endif
  ptable.queue[itr_q] = p; 
  release(&ptable.lock);
  #endif
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  
  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
#ifdef RT
#if RT
fork(int d, int c)
#else
fork(int d, int c, int p, int o)
#endif
#else
fork(void)
#endif
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;

  *np->tf = *proc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  safestrcpy(np->name, proc->name, sizeof(proc->name));
 
  pid = np->pid;

  // lock to force the compiler to emit the np->state write last.
  acquire(&ptable.lock);
  np->state = RUNNABLE;
  #ifdef RT
  #if RT // EDF
  np->D = d;              // Set deadline
  np->C = c;              // Set computation time
  np->arrtime = tick();                 // Get arrival time
  np->ctxswt = 0;

  itr_q++;
  ptable.queue[itr_q] = np;             // Insert np into queue
  increasekey(ptable.queue, itr_q);     // Sort queue by EDF 
  release(&ptable.lock);
  yield();
  //interrupt = 1;

  #else // PT
  np->D = d;                            // Set deadline
  np->C = c;                            // Set computation time
  np->P = p;              // Set priority
  np->O = o;              // Set threshold
  np->ctxswt = 0;

  if(frozen){np->state = WAITING;}      // Criação de processos em lote
  else{                                 // Criação de processos do sistema
    np->arrtime = tick();               // Get arrival time
    itr_q++;
    ptable.queue[itr_q] = np;           // Insert np into queue
    increasekey(ptable.queue, itr_q);   // Sort queue by PT
  }
  release(&ptable.lock);
  #endif
  #else
  release(&ptable.lock);
  #endif
  
    
  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;

  for(;;){
    // Enable interrupts on this processor.
    sti();
    acquire(&ptable.lock);
    
    #ifdef RT
    if(itr_q != -1){
      p = ptable.queue[0];
      ptable.queue[0]->ctxswt++;
      #if !RT
      if(!frozen || p->P > proc->O){                // Escalonamento com PT
      #endif

        ptable.queue[0] = ptable.queue[itr_q];
        heapify(ptable.queue, itr_q, 0);            // Sort queue
        itr_q--;

        proc = p;
        switchuvm(p);
        p->state = RUNNING;

        if(p->firstsch == 0) p->firstsch = tick();  // Get data
        
        swtch(&cpu->scheduler, proc->context);
        
        switchkvm();
        proc = 0;

        #if !RT
        }
        #endif
    }
    #else // Convencional
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;
      proc = p;
      switchuvm(p);
      p->state = RUNNING;
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      proc = 0;
    }
    #endif
    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
 
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  #ifdef RT
  #if !RT
  if(ptable.queue[0]->P > proc->O){
  #endif
    proc->state = RUNNABLE;
    itr_q++;
    ptable.queue[itr_q] = proc;               // Insert proc into queue
    increasekey(ptable.queue, itr_q);
    sched();
  #if !RT
  }
  #endif
  interrupt = 0;
  #else
  proc->state = RUNNABLE;
  sched();
  #endif
  
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot 
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }
  
  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
  //cprintf("sleep %d - %d\n", *(int*)chan, proc->pid);
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && *(int *)p->chan == *(int *)chan){
      //cprintf("wakeup1 %d - %d\n", *(int *)chan, p->pid);
      p->state = RUNNABLE;
      p->chan = 0;
      #ifdef RT
      p->arrtime = tick();              //Necessário para realizar os testes
      p->firstsch = 0;
      p->ctxswt = 0;
      itr_q++;
      ptable.queue[itr_q] = p;          // Insert p into queue
      increasekey(ptable.queue, itr_q);
      #if !RT
      interrupt = 1;          // Enable interrupts
      #endif
      #endif
    }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING){
        p->state = RUNNABLE;
        #ifdef RT
        itr_q++;
        ptable.queue[itr_q] = p;                     // Insert p into queue
        increasekey(ptable.queue, itr_q);
        printQueue();
        #endif
      }
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie",
  [WAITING]   "waiting"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];
  
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
       cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

#ifdef RT
void getData(struct proc *p)
{

  if(itr_s > (TAM -1)){
    cprintf(".");
    //cprintf("ERROR: FULL DATA! Ignoring statistic process... %d\n", itr_s);
  }else{
    #if !RT
    statistic[itr_s].P = p->P;
    statistic[itr_s].O = p->O;
    #endif
    statistic[itr_s].firstsch = p->firstsch; p->firstsch = 0;
    statistic[itr_s].arrtime = p->arrtime; p->arrtime = tick();
    statistic[itr_s].finish = tick();
    statistic[itr_s].pid = p->pid;
    statistic[itr_s].C = p->C;
    statistic[itr_s].D = p->D;
    statistic[itr_s].ctxswt = p->ctxswt; 
    p->ctxswt= 0;
    itr_s++;
  }
}

int print(int flag)
{
  int i;
  if(flag == 2){
    getData(proc);
  }else if(flag == 1 ){
    #if !RT
    cprintf("\nPid-P-O-D-C-Ctxswt-Arr-First-Finish-Res-Laten-Miss\n");
    #else
    cprintf("\nPid-D-C-Ctxswt-Arr-First-Finish-Res-Laten-Miss\n");
    #endif
    for(i = 0; i < itr_s; i++){
      // bug in the cprintf
      if(!statistic[i].P) continue;
      cprintf("%d-",  statistic[i].pid);
      #if !RT
      cprintf("%d-",  statistic[i].P);
      cprintf("%d-",  statistic[i].O);
      #endif
      cprintf("%d-",  statistic[i].D);
      cprintf("%d-",  statistic[i].C);
      
      cprintf("%d-",  statistic[i].ctxswt);
      cprintf("%d-",  statistic[i].arrtime);
      cprintf("%d-",  statistic[i].firstsch);
      cprintf("%d-",  statistic[i].finish);
      cprintf("%d-", (statistic[i].firstsch - statistic[i].arrtime));
      cprintf("%d-", (statistic[i].finish - statistic[i].arrtime));
      if((statistic[i].finish - statistic[i].arrtime) > statistic[i].D){
        cprintf("1-MISS\n");  // Miss
        miss++;
      }
      else{
        cprintf("0\n");
      }
    }
    cprintf("miss = %d-",  miss);
    miss = 0 ;    
  }else{
    itr_s = 0;
  }
  return 0; 
}

void freeze(int f)
{
  struct proc *p;

  if(f){
    frozen = 1;
  }else{
    frozen = 0;
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state == WAITING){
        p->state = RUNNABLE;          // Unfreeze all created processes
        itr_q++;
        ptable.queue[itr_q] = p;                     
        increasekey(ptable.queue, itr_q);
      }
    }
    release(&ptable.lock);
  }
}

void printQueue()
{
  int i;
  for(i = 0; i <= itr_q; i++){
   cprintf("p->pid = %d -- itr_q = %d\n", ptable.queue[i]->pid, itr_q);
  }
}

#endif