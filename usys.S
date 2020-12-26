#include "syscall.h"
#include "traps.h"

#define SYSCALL(name) \
  .globl name; \
  name: \
    movl $SYS_ ## name, %eax; \
    int $T_SYSCALL; \
    ret

SYSCALL(fork)
SYSCALL(exit)
SYSCALL(wait)
SYSCALL(pipe)
SYSCALL(read)
SYSCALL(write)
SYSCALL(close)
SYSCALL(kill)
SYSCALL(exec)
SYSCALL(open)
SYSCALL(mknod)
SYSCALL(unlink)
SYSCALL(fstat)
SYSCALL(link)
SYSCALL(mkdir)
SYSCALL(chdir)
SYSCALL(dup)
SYSCALL(getpid)
SYSCALL(sbrk)
SYSCALL(sleep)
SYSCALL(uptime)
SYSCALL(reverse_number)
SYSCALL(get_children)
SYSCALL(trace_syscalls)
SYSCALL(change_line)
SYSCALL(set_ticket)
SYSCALL(set_bjf_param_process)
SYSCALL(set_bjf_param_system)
SYSCALL(print_info)
SYSCALL(semaphore_initialize)
SYSCALL(semaphore_acquire)
SYSCALL(semaphore_release)