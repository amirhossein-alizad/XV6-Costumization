#include "user.h"
#include "types.h"
#include "stat.h"
// #include "defs.h"
int main()
{
  semaphore_initialize(0,5,0);
  int id1 = fork();
  if(id1 == 0)
  {
    consumer(0);
  }
  else
  {
    int id2 = fork();
    if(id2 == 0)
    {
      producer(0);
    }
  }
  wait();
  exit();
}
