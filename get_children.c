#include "types.h"
#include "stat.h"
#include "user.h"

int main(void)
{
    printf(1, "starting from root, id = %d\n", getpid());
    int pid1 = fork();
    if(pid1 == 0)
    {
      int pid2 = fork();
      if(pid2 == 0)
      {
        int pid3 = fork();
        if(pid3 == 0)
        {
          sleep(1000);
          wait();
          exit();
        }
        else{
          sleep(1000);
          wait();
          exit();
        }
      }
      else
      {
        sleep(1000);
        wait();
        exit();
      }
    }
    else{
      int pid2 = fork();
      if (pid2 == 0)
      {
        sleep(1000);
        wait();
        exit();
      }
      else{
        sleep(150);
        printf(1, "id = %d, children and grandChildren = %d\n",getpid(), get_children(getpid()));
        wait();
        exit();
      }
    }
}
