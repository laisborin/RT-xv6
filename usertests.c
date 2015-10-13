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
  int i = 0, stop, pid, c, d, start[3] = {T(), T(), T()};

  #ifdef RT
  //print(0); // Zera
  while(1){
    stop = T();

    d = 3; c = 1;
    if(stop - start[0] == 5 && (pid=fork(d, c)) == 0){
      a(c);
      exit();
      start[0] = T();
      i++;
    }
    d = 4; c = 2;
    if(stop - start[1] == 15 && (pid=fork(d, c)) == 0){
      a(c);
      exit();
      start[1] = T();
      i++;
    }

    d = 2; c = 5;
    if(stop - start[3] == 20 && (pid=fork(d, c)) == 0){
      a(c);
      exit();
      start[2] = T();
      i++;
    }
    if(stop - start[0] > 10000){
      printf(1, "aa\n");
      break;
    }
  }
   printf(1, "i = %d\n", i);
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
