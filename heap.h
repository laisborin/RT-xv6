#define LEFT(x) (2 * x + 1)
#define RIGHT(x) (2 * x + 2)
#define PARENT(x) (x / 2)

void heapify(struct proc **A, int, int);
void increasekey(struct proc **A, int);
int deadline(struct proc *p);
unsigned long long tick();