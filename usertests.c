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
  sleep(t); 
  #endif
  
  for(j = 0; j < 1000; j++){
    for (i = 0; i < 80000*c; i++);
    print(2);
    sleep(t);
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
  insert(p, 20, 50, 70, 3, 3);
  insert(p, 20, 80, 80, 2, 3);
  insert(p, 35, 100, 200, 1, 2);


  int i;
  for(i = 0; i < processos; i++){
    if(fork(p[i].d, p[i].c, p[i].p, p[i].o) == 0){
      work(p[i].t, p[i].c);
    }
  }
 
  #endif

  #if RT
  int i, processos = 3;
  struct procs p[processos];
  // 3
  insert(p, 4, 16, 14);
  insert(p, 4, 11, 16);
  insert(p, 7, 20, 40);


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

