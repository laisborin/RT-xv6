#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
#include "memlayout.h"

static __inline__ unsigned long long ti()
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
  int i, pid, c, d;

  for(i = 0; i < 30 ; i++){
    #ifdef RT
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
  } 

  while(wait() != -1);
#ifdef RT
  print();
#endif
  exit();
}
