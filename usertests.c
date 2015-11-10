#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
#include "memlayout.h"

static __inline__ unsigned long long T()
{
  unsigned long long t;
  __asm__ __volatile__ ("rdtsc" : "=A"(t));
  return t >> 21;
}


void a(int t, int c){
  int i, j;
  #if !RT
  sleep(t);
  #endif
  
  for(j = 0; j < 100; j++){
    for (i = 0; i < 80000*c; i++);
    print(2);
    sleep(t);
  }
  exit();
}

int
main(int argc, char *argv[])
{
  
  #ifdef RT
  print(0); // Zera
  #if !RT
  freeze(1); // Sinaliza a criação dos processos em lote
  
  int pid, c, d, t, o, p;


  d = 18; c = 3; t = 4; o = 2; p = 2;
  if((pid=fork(d, c, p, o)) == 0){
    a(t, c);
  }
  
  d = 15; c = 2; t = 4; o = 3; p = 3;
  if((pid=fork(d, c, p, o)) == 0){
    a(t, c);
  }

  d = 10; c = 4; t = 6; o = 4; p = 4;
  if((pid=fork(d, c, p, o)) == 0){
    a(t, c);
  }
  #endif

  #if RT
  
  int pid, c, d, t;


  d = 18; c = 3; t = 4;
  if((pid=fork(d, c)) == 0){
    a(t, c);
  }
  
  d = 15; c = 2; t = 4;
  if((pid=fork(d, c)) == 0){
    a(t, c);
  }

  d = 10; c = 4; t = 6;
  if((pid=fork(d, c)) == 0){
    a(t, c);
  }
  #endif
/*
  d = 10; c = 1; t = 6; o = 4; p = 4;
  if((pid=fork(d, c, p, o)) == 0){
    a(t, c);
  }

  d = 10; c = 1; t = 6; o = 4; p = 4;
  if((pid=fork(d, c, p, o)) == 0){
    a(t, c);
  }

  d = 10; c = 1; t = 6; o = 4; p = 4;
  if((pid=fork(d, c, p, o)) == 0){
    a(t, c);
  }

  d = 10; c = 1; t = 6; o = 4; p = 4;
  if((pid=fork(d, c, p, o)) == 0){
    a(t, c);
  }

  d = 10; c = 1; t = 6; o = 4; p = 4;
  if((pid=fork(d, c, p, o)) == 0){
    a(t, c);
  }

  d = 10; c = 1; t = 6; o = 4; p = 4;
  if((pid=fork(d, c, p, o)) == 0){
    a(t, c);
  }

  d = 10; c = 1; t = 6; o = 4; p = 4;
  if((pid=fork(d, c, p, o)) == 0){
    a(t, c);
  }
*/


 /* long long a, b;
  int i, j;
  for(j = 0; j < 50; j++){
    a = T();
    for(i = 0; i < 100000; i++);
    b = T();
    printf(1, "%d\n", b - a);
  }*/
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
  }*/
  
  #endif

  while(wait() != -1);
  #ifdef RT
  print(1); //Mostra
  #endif
  exit();
}