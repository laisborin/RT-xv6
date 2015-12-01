#include "types.h"
#include "defs.h"
#include "heap.h"
#include "param.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#ifdef RT
// Used by EDF
#if RT // MIN HEAP - DEADLINE
int deadline(struct proc *p){
  if(tick() <= (p->arrtime + p->D)){  
    return (p->arrtime + p->D) - tick();
  }
 /* else { 
    p->miss++; 
    cprintf("%d ----- M I S S ------ ct = %d", p->pid, tick());
    cprintf("  c = %d d = %d arr = %d", p->C, p->D, p->arrtime);
    cprintf("  sum = %d\n", (p->arrtime + p->D));
  
  }*/
  return 0;
}

void heapify(struct proc **A, int itr_q, int i){
  int min = i;
  struct proc *swap;

  if(LEFT(i) < itr_q && deadline(A[LEFT(i)]) < deadline(A[i]))       min = LEFT(i);
  if(RIGHT(i) < itr_q && deadline(A[RIGHT(i)]) < deadline(A[min]))  min = RIGHT(i);
  if(min != i){
    swap = A[i];  A[i] = A[min];  A[min] = swap;
    heapify(A, itr_q, min);
  }
}

void increasekey(struct proc **A, int i){
  struct proc *swap;
      
  while(i > 0 && deadline(A[PARENT(i)]) > deadline(A[i])){
    swap = A[i]; A[i] = A[PARENT(i)]; A[PARENT(i)] = swap;
    i = PARENT(i);
  }
}


// Used by PT
#else // MAX HEAP - PRIORITY
void heapify(struct proc **A, int itr_q, int i){
  int max = i;
  struct proc *swap;

  if(LEFT(i) < itr_q && A[LEFT(i)]->P > A[i]->P)       max = LEFT(i);
  if(RIGHT(i) < itr_q && A[RIGHT(i)]->P > A[max]->P)  max = RIGHT(i);
  if(max != i){
    swap = A[i];  A[i] = A[max];  A[max] = swap;
    heapify(A, itr_q, max);
  }
}

void increasekey(struct proc **A, int i){
  struct proc *swap;
      
  while(i > 0 && A[PARENT(i)]->P < A[i]->P){
    swap = A[i]; A[i] = A[PARENT(i)]; A[PARENT(i)] = swap;
    i = PARENT(i);
  }
}
#endif

unsigned long long tick() {
  unsigned long long t;
  __asm__ __volatile__ ("rdtsc  " : "=A"(t));
  return t >> 21;
}
#endif  