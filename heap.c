#include "types.h"
#include "defs.h"
#include "heap.h"
#include "param.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"

int deadline(struct proc *p){
  int decorrido = crtime - p->arrtime;
  int rest = p->D - decorrido;

  if(rest >= p->C)
    return rest - p->C;
  else {p->miss++; cprintf("%d ----- M I S S ------ %d\n",p->pid, p->miss);}

  //cprintf("%d curr = %d cheg = %d d = %d c = %d\n",p->pid, crtime, p->arrtime, p->D, p->C);
    //cprintf(" ----- M I S S ------\n");

  return -1;
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

struct proc *extractmin(struct proc **A, int itr_q){
  struct proc *min;
  if(itr_q > -1){
    min = A[0];
    A[0] = A[itr_q];
    heapify(A, itr_q, 0);
    itr_q--;
  }

  return min;
}

void increasekey(struct proc **A, int i){
  struct proc *swap;
      
  while(i > 0 && deadline(A[PARENT(i)]) > deadline(A[i])){
    swap = A[i]; A[i] = A[PARENT(i)]; A[PARENT(i)] = swap;
    i = PARENT(i);
  }
}

void heapinsert(struct proc **A, int itr_q, struct proc *p){
  A[itr_q] = p;
  increasekey(A, itr_q);
  itr_q++;
}

void oi(void){
  cprintf("oie -----------------------------\n");
}


/*
Extracted function of http://www.strudel.org.uk/itoa/
Written by Lukás Chmela
Version 0.4
*/

char * itoa (int value, char *result, int base){
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}