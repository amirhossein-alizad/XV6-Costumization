#include "user.h"
#include "types.h"
#include "stat.h"
#include "spinlock.h"

struct condvar
{
    struct spinlock lock;
};


int main()
{
  struct condvar condvar;
  
  int pid = fork();
  if (pid < 0) 
  {
      printf(1, "Error forking first child.\n");
  }
  else if (pid == 0)
  {
      sleep(100);
      printf(1, "Child 1 Executing.\n");
      cv_signal(&condvar);
  }
  else
  {
      pid = fork();
      if (pid < 0) 
      {
          printf(1, "Error forking second child.\n");
      }
      else if (pid == 0)
      {
          cv_wait(&condvar);
          printf(1, "Child 2 Executing.\n");
      }
      else
      {
          int i;
          for (i=0; i<2; i++)
          {
              wait();
          }
          
      }
  }
  exit();
}
