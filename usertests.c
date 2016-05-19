#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
#include "memlayout.h"


#ifdef RT
unsigned long long J() {
  unsigned long long t;
  __asm__ __volatile__ ("rdtsc  " : "=A"(t));
  return t >> 21;
}

typedef struct procs {
  int c, d, t, p, o;
}procs;

int index = 0;

#if RT
void insert(procs p[], int c, int d, int t){
#else
void insert(procs p[], int c, int d, int t, int pr, int o){
#endif
  p[index].c = c; p[index].d = d; p[index].t = t;
  #if !RT
  p[index].p = pr; p[index].o = o;
  #endif
  index++;
}

void work(int t, int c){
  int i, j;
  #if !RT
  sleep(t + 2*(J()%4+1)); 
  #endif
  
  for(j = 0; j < 50; j++){
    for (i = 0; i < 2*200000*c; i++);
    print(2);
    sleep(t + 2*(J()%4+1)); // J is a random jitter
  }
  exit();
}
#endif
int
main(int argc, char *argv[])
{
  
  #ifdef RT
  print(0); // Zera
  #if !RT
  freeze(1); // Sinaliza a criação dos processos em lote

  int processos = 3;
  struct procs p[processos];
  //        c   d   t  p  o
  insert(p, 2, 15, 15, 5, 5);
  insert(p, 3, 20, 25, 4, 4);
  //insert(p, 5, 25, 45, 3, 3);
  insert(p, 4, 22, 30, 2, 3);
 // insert(p, 1, 20, 37, 1, 3);

  int i;
  for(i = 0; i < processos; i++){
    if(fork(p[i].d, p[i].c, p[i].p, p[i].o) == 0){
      work(p[i].t, p[i].c);
    }
  }
 
  #endif

  #if RT
  int i, processos = 5;
  struct procs p[processos];

  insert(p, 2, 15, 15, 5, 5);
  insert(p, 3, 20, 25, 4, 4);
  insert(p, 5, 25, 45, 3, 3);
  insert(p, 4, 22, 30, 2, 3);
  insert(p, 1, 20, 37, 1, 3);
  
  /*// 3
  insert(p, 4, 16, 14);
  insert(p, 4, 11, 16);
  insert(p, 7, 20, 40);
*/

  for(i = 0; i < processos; i++){
    if(fork(p[i].d, p[i].c) == 0){
      work(p[i].t, p[i].c);
    }
  }

  #endif


  #if !RT
  freeze(0); // Sinaliza o fim da criação de um lote, colocando-os em execução
  #endif

  #else
   if(fork()==0){
    printf(1, "Convencional mode!\n");
    exit();
   }
  #endif

  while(wait() != -1);
  #ifdef RT
  print(1); //Mostra
  #endif
  exit();
}

