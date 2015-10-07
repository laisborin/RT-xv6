// Create a zombie process that 
// must be reparented at exit().

#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
	#ifdef RT
	if(fork(5, 5) > 0)
	#else
	if(fork() > 0)
	#endif
		sleep(5);  // Let child exit() before parent.
	exit();
}
