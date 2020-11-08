#include "types.h"
#include "stat.h"
#include "user.h"

int main(void)
{
    int state = 1;

    while (1)
    {
        trace_syscalls(state);
    }
    exit();
}
