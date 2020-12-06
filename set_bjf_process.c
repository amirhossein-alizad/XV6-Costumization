#include "user.h"
#include "fcntl.h"
#include "types.h"
#include "stat.h"

int main(int argc,char* argv[])
{
  int Pid, Priority_ratio, Arrival_time_ratio, Executed_cycle_ratio;
  if (argc > 1)
  {
    Pid = atoi(argv[1]);
    Priority_ratio = atoi(argv[2]);
    Arrival_time_ratio = atoi(argv[3]);
    Executed_cycle_ratio = atoi(argv[4]);
    set_bjf_param_process(Pid, Priority_ratio, Arrival_time_ratio, Executed_cycle_ratio);
  }

  exit();
}