#include "user.h"
#include "types.h"
#include "stat.h"

int main()
{
  semaphore_initialize(0,1,0); //mutex
  semaphore_initialize(1,5,0); //empty
  semaphore_initialize(2,5,5); //full
  int id1 = fork();
  if(id1 == 0)
  {
    producer(0);
  }
  else
  {
    consumer(0);
    wait();
  }
  wait();
  exit();
}
