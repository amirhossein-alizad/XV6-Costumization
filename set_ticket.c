#include "user.h"
#include "fcntl.h"
#include "types.h"
#include "stat.h"

int main(int argc,char* argv[])
{
  int Pid, ticket;
  if (argc > 1)
  {
    Pid = atoi(argv[1]);
    ticket = atoi(argv[2]);
    set_ticket(Pid,ticket);
  }

  exit();
}