#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char* argv[])
{
    trace_syscalls(atoi(argv[1]));
    exit();
}
