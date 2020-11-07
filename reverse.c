#include "user.h"
#include "fcntl.h"
#include "types.h"
#include "stat.h"

int main(int argc,char* argv[])
{
    int prev;
    asm ("movl %%edi, %0;"
                    : "=r"(prev)
                    :
                    : "%edi");
    int num = atoi(argv[1]);
    asm ("movl %0, %%edi;"
                    :
                    : "r"(num)
                    : "%edi");

    printf(1, "%d\n", reverse_number());
    asm ("movl %0, %%edi;"
                    :
                    : "r"(prev)
                    : "%edi");
    exit();
}
