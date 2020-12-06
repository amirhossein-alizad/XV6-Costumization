#include "user.h"
#include "fcntl.h"
#include "types.h"
#include "stat.h"

int main(int argc,char* argv[])
{
  int Pid, line;
  if (argc > 1)
  {
    Pid = atoi(argv[1]);
    line = atoi(argv[2]);
    change_line(Pid,line);
  }

  exit();
}
