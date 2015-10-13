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

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
  struct proc *queue[NPROC]; // Ready queue
  
} ptable;

#if RT
uint itr_q; // Iterator in ready queue

uint itr_s; // Iterator in statistic
struct statistic statistic[NPROC*3];  

unsigned int ctxswt;         // Number of context swtch

void getData(struct proc *p);

void printQueue();

int teste(int, int); //Chamada de sistema
int print(int);
void testeTwo(void);




static __inline__ unsigned long long tick()
{
  unsigned long long t;
  __asm__ __volatile__ ("rdtsc" : "=A"(t));
  return t >> 22;
}


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
  #if RT
  p->D = 0;
  p->C = 0;
  p->miss = 0;
  p->arrtime = 0;       // Tempo de chegada
  p->firstsch = 0;      // Instante do primeiro escalonamento

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
  #if RT
  acquire(&ptable.lock);
  itr_q = 0;
  itr_s = 0;
 //cprintf("userinit\n");
  p->D = 100;                            // First user process, requires a start
  p->C = 100;
  p->arrtime = tick();
  ptable.queue[itr_q] = p; 
  crtime = tick();
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
fork(int d, int c)
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
  #if RT

  np->D = d; // Set deadline
  np->C = c; // Set computation time
  //cprintf("d = %d c = %d\n", np->D, np->C);
  np->arrtime = tick(); // Get arrival time
  //cprintf("--- fork(%d) ------------------------------------ %d\n",np->pid, itr_q);
  //if(itr_q == -1) itr_q = 0;
  itr_q++;
  ptable.queue[itr_q] = np;                     // Insert p into queue
  crtime = tick();                       // Get current time
  increasekey(ptable.queue, itr_q);             // Sort queue by EDF
  //cprintf("-d = %d c = %d\n", np->D, np->C);
  //cprintf("itr_q = %d\n", itr_q);
  //printQueue();
  #endif
  release(&ptable.lock);
  
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

  getData(proc); // Get data before exit the process

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
        #if RT
        p->D = 0;
        p->C = 0;
        p->arrtime = 0;
        p->firstsch = 0;
        p->miss = 0;
        #endif
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

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    //cprintf("itr_q = %d\n", itr_q);
    if(itr_q != -1){
      //cprintf("Escalonando um processo pid = %d itr_q = %d\n", ptable.queue[0]->pid, itr_q);      
      
      // Get the EDF process
      p = ptable.queue[0];
      ptable.queue[0] = ptable.queue[itr_q];
      crtime = tick();

      // Sort queue
      heapify(ptable.queue, itr_q, 0);
      //printQueue();
      itr_q--;

     //cprintf("itr_q = %d\n", itr_q);
      
      proc = p;
      switchuvm(p);
      p->state = RUNNING;
      
      //cprintf("swtch - scheduler\n");

      // Get data
      if(p->firstsch == 0) p->firstsch = tick();

      ctxswt++;
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      proc = 0;
    }
    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
void
sched(void)
{
  int intena;
 //cprintf("sched\n");

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;

  ctxswt++;
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{

  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE; 
  #if RT
 //cprintf("yield------------------------- %d\n", itr_q);
  //if(itr_q == -1) itr_q = 0;
  itr_q++;
  ptable.queue[itr_q] = proc;               // Insert p into queue

  increasekey(ptable.queue, itr_q);
  //itr_q++;
 //cprintf("itr_q = %d\n", itr_q);
  //printQueue();
  //heapinsert(queue, itr_q, proc);
  #endif
  sched();
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
    if(p->state == SLEEPING && p->chan == chan){
      p->state = RUNNABLE;
      #if RT
     //cprintf("wakeup1 ------------------------- %d\n", itr_q);
      //if(itr_q == -1) itr_q = 0;
      itr_q++;
      ptable.queue[itr_q] = p;                     // Insert p into queue
      increasekey(ptable.queue, itr_q); 
      //itr_q++;
     //cprintf("itr_q = %d\n", itr_q);
      //printQueue();
      //heapinsert(queue, itr_q, p);
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
        #if RT
       //cprintf("kill ------------------ %d\n", itr_q);
        //if(itr_q == -1) itr_q = 0;
        itr_q++;
        ptable.queue[itr_q] = p;                     // Insert p into queue
        increasekey(ptable.queue, itr_q);
        //itr_q++;
       //cprintf("itr_q = %d\n", itr_q);
        printQueue();
        //heapinsert(queue, itr_q, p);
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
  [ZOMBIE]    "zombie"
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

#if RT

void getData(struct proc *p){

  if(itr_s > (NPROC*3 -1)){
    cprintf("ERROR: FULL DATA! Ignoring statistic process... %d\n", itr_s);
  }else{
    statistic[itr_s].firstsch = p->firstsch;
    statistic[itr_s].arrtime = p->arrtime;
    statistic[itr_s].pid = p->pid;
    statistic[itr_s].miss = p->miss;
    statistic[itr_s].C = p->C;
    statistic[itr_s].D = p->D;
    itr_s++;
  }
}

int print(int flag){
  int i;
  if(flag > 0 ){
    cprintf("\nPid D   C  Miss  A     F\n");
    for(i = 0; i < itr_s; i++){
      // bug in the cprintf
      cprintf("%d   ",  statistic[i].pid);
      cprintf("%d   ",  statistic[i].D);
      cprintf("%d   ",  statistic[i].C);
      cprintf("%d   ",  statistic[i].miss);
      cprintf("%d   ",  statistic[i].arrtime);
      cprintf("%d\n",  statistic[i].firstsch);
    }
    
    cprintf("Context swtch = %d\n", ctxswt);
  }
  for(i = 0; i < itr_s; i++){
    statistic[i].pid = 0;
    statistic[i].D = 0;
    statistic[i].C = 0;
    statistic[i].miss = 0;
    statistic[i].arrtime = 0;
    statistic[i].firstsch = 0;
  }
  itr_s = ctxswt = 0;

  return 0; 
}

int teste(int i, int a){


  return i;
}

void printQueue(){
  int i;
 //cprintf("\nQueue:\n ");

  for(i = 0; i <= itr_q; i++){
   //cprintf("p->pid = %d -- itr_q = %d\n", ptable.queue[i]->pid, itr_q);
  }
 //cprintf("\n ");
}

void testeTwo(void){
 //cprintf("Teste dois executado com sucesso!\n");
}

#endif