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
  
  
  struct procs p[15];
//int pid, c,  d,  t, p, o;
  insert(p, 6, 30, 41, 15, 15);
  insert(p, 3, 27, 34, 14, 14);
  insert(p, 2, 23, 41, 13, 13);
  insert(p, 4, 42, 48, 12, 13);
  insert(p, 2, 31, 35, 11, 11);

  insert(p, 1, 23, 30, 10, 10);
  insert(p, 3, 48, 50, 9, 10);
  insert(p, 5, 36, 47, 8, 8);
  insert(p, 2, 45, 84, 7, 7);
  insert(p, 6, 50, 79, 6, 6);
  insert(p, 4, 45, 68, 5, 6);
  insert(p, 6, 61, 80, 4, 4);
  insert(p, 3, 56, 65, 3, 3);
  insert(p, 4, 70, 98, 2, 3);
  insert(p, 5, 77, 89, 1, 2);
/*  
 
  struct procs p[3];
  insert(p, 4, 14, 10, 3, 3);
  insert(p, 4, 16, 16, 2, 3);
  insert(p, 7, 40, 20, 1, 2);
*/

  int i;
  for(i = 0; i < 15; i++){
    if(fork(p[i].d, p[i].c, p[i].p, p[i].o) == 0){
      work(p[i].t, p[i].c);
    }
  }
 
  #endif

  #if RT
  int i;
  struct procs p[15];

/*  
  insert(p, 4, 14, 10);
  insert(p, 4, 16, 16);
  insert(p, 7, 40, 20);
*/
  insert(p, 6, 30, 41);
  insert(p, 3, 27, 34);
  insert(p, 2, 23, 41);
  insert(p, 4, 42, 48);
  insert(p, 2, 31, 35);
  insert(p, 1, 23, 30);
  insert(p, 3, 48, 50);
  insert(p, 5, 36, 47);
  insert(p, 2, 45, 84);
  insert(p, 6, 50, 79);
  insert(p, 4, 45, 68);
  insert(p, 6, 61, 80);
  insert(p, 3, 56, 65);
  insert(p, 4, 70, 98);
  insert(p, 5, 77, 89);

  for(i = 0; i < 15; i++){
    if(fork(p[i].d, p[i].c) == 0){
      work(p[i].t, p[i].c);
    }
  }

  #endif

/* long long a, b;
  int i, j;
  for(j = 0; j < 50; j++){
    a = T();
    for(i = 0; i < 100000; i++);
    b = T();
    printf(1, "%d\n", b - a);
  }
*/
  #if !RT
  freeze(0); // Sinaliza o fim da criação de um lote, colocando-o em execução
  #endif
 // Teste de tempo

/*long long a, b;
  int i;
  for (i = 0; i < 100; i++){
    a = T();
    sleep(1000);
    b = T();
    printf(1, "%d\n", b - a);
  }
*/
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

