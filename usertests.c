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
  return t >> 22;
}

void a(int c){
  int i;
  for (i = 0; i < 275000*c; i++);
}

int
main(int argc, char *argv[])
{
  

  #ifdef RT
  print(0); // Zera
  int pid, c, d;
  unsigned long long i = T(), stop, start[4] = {T(), T(), T(), T()};
  while(1){
    stop = T();

    d = 4; c = 1;
    if(stop - start[0] == 10){
      start[0] = T();
      if((pid=fork(d, c)) == 0){ 
        a(c);
        exit();
      } 
    }
    d = 6; c = 2;
    if(stop - start[1] == 15){
      start[1] = T();
      if((pid=fork(d, c)) == 0){ 
        a(c);
        exit();
      } 
    }

    d = 10; c = 5;
    if(stop - start[2] == 25){
      start[2] = T();
      if((pid=fork(d, c)) == 0){ 
        a(c);
        exit();
      } 
    }

    d = 16; c = 3;
    if(stop - start[3] == 30){
      start[3] = T();
      if((pid=fork(d, c)) == 0){ 
        a(c);
        exit();
      } 
    }

    if(stop - i > 10000){
      printf(1, "aa\n");
      break;
    }
  }

  #endif
  /*for(i = 0; i < 30 ; i++){

    d = 3;
    c = 1;
    if((pid=fork(d, c))==0){
    #else
    if((pid=fork())==0){
    #endif
      #if RT
       // /;print();
      //teste(d, c);
      a(c);
      #endif
      exit();
    }
  } */

  while(wait() != -1);
  #ifdef RT
  print(1); //Mostra
  #endif
  exit();
}
