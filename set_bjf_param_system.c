#include "user.h"
#include "fcntl.h"
#include "types.h"
#include "stat.h"

int main(int argc,char* argv[])
{
  int Priority_ratio, Arrival_time_ratio, Executed_cycle_ratio;
  if (argc > 1)
  {
    Priority_ratio = atoi(argv[1]);
    Arrival_time_ratio = atoi(argv[2]);
    Executed_cycle_ratio = atoi(argv[3]);
    set_bjf_param_system(Priority_ratio, Arrival_time_ratio, Executed_cycle_ratio);
  }

  exit();
}