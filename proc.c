#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
  int state;
} ptable;

struct {
  struct spinlock lock;
  struct proc* proc[NPROC];
  int last;
  int v;
  int m;
} semaphores[5];

int get_trace_state()
{
  int trace_state ;
  acquire(&ptable.lock);
  trace_state = ptable.state;
  release(&ptable.lock);
  return trace_state;
}

int set_trace_state(int st)
{
  acquire(&ptable.lock);
  ptable.state = st;
  release(&ptable.lock);
  return 0;
}
static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;

  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");

  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  p->queue = 2;
  p->tickets = 10;
  p->cycle = 1;
  p->arrival_time = ticks;
  p->arrival_time_ratio = 1;
  p->executed_cycle_ratio = 1;
  p->priority_ratio = 1;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();

  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

int rand()
{
  static int random = 3251;
  random = (((random*random)/100)+53)%10000;
  return random;
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;

  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    struct proc *queue1[NPROC],*queue2[NPROC],*queue3[NPROC];
    int indexQ1=0,indexQ2=0,indexQ3=0;
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      if(p->cycle >= 10000 && p->queue != 1){
        p->queue = 1;
        p->cycle = 1;
      }

      if(p->queue == 1){
        queue1[indexQ1] = p;
        indexQ1++;
      }
      else if(p->queue == 2){
        queue2[indexQ2] = p;
        indexQ2++;
      }
      else if(p->queue == 3){
        queue3[indexQ3] = p;
        indexQ3++;
      }
    }

    if (indexQ1 > 0){
      //round robin
      //check for time quantum???????????
      p = queue1[0];
    }

    else if (indexQ2 > 0){
      //ticket

      int tickets_sum =0;
      for (int i = 0; i < indexQ2; i++)
      {
        tickets_sum += queue2[i]->tickets;
      }

      int winner_ticket = rand()%tickets_sum;
      tickets_sum =0;

      for (int i = 0; i < indexQ2; i++)
      {
        tickets_sum += queue2[i]->tickets;
        if (tickets_sum > winner_ticket)
        {
          p = queue2[i];
          break;
        }

      }

    }

    else if (indexQ3 > 0){
      //best job first

      int max=0;
      int index=0;
      for (int i = 0; i < indexQ3; i++)
      {
        int rank = (queue3[i]->priority_ratio/queue3[i]->tickets)+(queue3[i]->arrival_time * queue3[i]->arrival_time_ratio)
        + (queue3[i]->cycle * queue3[i]->executed_cycle_ratio);
        if(max < rank)
          {
            max = rank;
            index = i;
          }
      }
      p = queue3[index];
    }


    else if(indexQ1 ==0 && indexQ2 ==0 && indexQ3 ==0 )
    {
      release(&ptable.lock);
      continue;
    }

    //aging
    p->cycle ++;


    // Switch to chosen process.  It is the process's job
    // to release ptable.lock and then reacquire it
    // before jumping back to us.
    c->proc = p;
    switchuvm(p);
    p->state = RUNNING;

    swtch(&(c->scheduler), p->context);
    switchkvm();

    // Process is done running for now.
    // It should have changed its p->state before coming back.
    c->proc = 0;
    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.

static char *states[] = {
  [UNUSED]    "UNUSED  ",
  [EMBRYO]    "EMBRYO  ",
  [SLEEPING]  "SLEEPING",
  [RUNNABLE]  "RUNNABLE",
  [RUNNING]   "RUNNING ",
  [ZOMBIE]    "ZOMBIE  "
  };

void
procdump(void)
{
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

int
reverse_number(int n)
{
  int rev = 0, remainder;
  while (n != 0) {
      remainder = n % 10;
      rev = rev * 10 + remainder;
      n /= 10;
  }
  return rev;
}

int
count_digits(int n)
{
  int count = 0;
  while(n != 0)
  {
    n /= 10;
    count += 1;
  }
  return count;
}

int
get_children_recursive(int parent_pid)
{
    int res = 0;
    struct proc *p;
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
        if (p->parent->pid == parent_pid)
        {
            int pid = p->pid;
            int count = count_digits(pid);
            for (int i = 0; i < count; i++)
                res *= 10;
            res += pid;

            int grandChildren = get_children_recursive(pid);
            cprintf("parent = %d, id = %d, children = %d\n", parent_pid, pid, grandChildren);
            if (grandChildren)
            {
                count = count_digits(grandChildren);
                for (int i = 0; i < count; i++)
                    res *= 10;
                res += grandChildren;
            }
        }
    }
    return res;
}

int
get_children(int parent_pid)
{
    acquire(&ptable.lock);
    int res = get_children_recursive(parent_pid);
    release(&ptable.lock);
    return res;
}
static char* syscallnames[] = {
          "fork",
         "exit",
         "wait",
         "pipe",
         "read",
         "kill",
         "exec",
         "fstat",
          "chdir",
          "dup",
         "getpid",
          "sbrk",
          "sleep",
         "uptime",
           "open",
          "write",
          "mknod",
         "unlink",
         "link",
          "mkdir",
          "close",
 "reverse_number",
   "get_children",
 "trace_syscalls",
 "change_line",
 "set_ticket",
 "set_bjf_param_process",
 "set_bjf_param_system",
 "print_info"
};
int
trace_syscalls(int state)
{
  acquire(&ptable.lock);
  if (ptable.state == 1)
  {
    struct proc *p;
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if(p->name[0] != '\0')
      {
        cprintf("\nNAME : %s\n", p->name);
        for(int i = 0; i < 29; i++)
        {
          if(p->number_of_calls[i])
            cprintf("%s : %d\n", syscallnames[i],  p->number_of_calls[i]);
        }
      }

    }
  }
  release(&ptable.lock);
  return 0;
}

int
change_line(int Pid, int line)
{
  acquire(&ptable.lock);
  struct proc *p;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->pid == Pid)
    {
        p->queue = line;
        break;
    }
  }
  release(&ptable.lock);
  return 0;
}

int
set_ticket(int Pid, int ticket)
{
  acquire(&ptable.lock);
  struct proc *p;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->pid == Pid)
    {
        p->tickets = ticket;
        break;
    }
  }
  release(&ptable.lock);
  return 0;
}

int
set_bjf_param_process(int Pid, int Priority_ratio, int Arrival_time_ratio, int Executed_cycle_ratio)
{
  acquire(&ptable.lock);
  struct proc *p;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->pid == Pid)
    {
        p->priority_ratio = Priority_ratio;
        p->arrival_time_ratio = Arrival_time_ratio;
        p->executed_cycle_ratio = Executed_cycle_ratio;
        break;
    }
  }
  release(&ptable.lock);
  return 0;
}

int
set_bjf_param_system(int Priority_ratio, int Arrival_time_ratio, int Executed_cycle_ratio)
{
  struct proc *p;
  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if(p->pid > 0){
      p->priority_ratio = Priority_ratio;
      p->arrival_time_ratio = Arrival_time_ratio;
      p->executed_cycle_ratio = Executed_cycle_ratio;
    }
  }
  release(&ptable.lock);
  return 0;
}

void print_fixed_size_str(char* str, int flag, int number){
  if(flag){
    int c = count_digits(number);
    cprintf("%d", number);
    for(int i = 0; i < 23 - c; i++){
      cprintf(" ");
    }
  }
  else{
    cprintf("%s", str);
    for(int i = 0; i < 23 - strlen(str); i++){
      cprintf(" ");
    }
  }
}
int
print_info(void)
{
  char *state;
  print_fixed_size_str("name",0,0);
  print_fixed_size_str("pid",0,0);
  print_fixed_size_str("state",0,0);
  print_fixed_size_str("queue",0,0);
  print_fixed_size_str("tickets",0,0);
  print_fixed_size_str("priority_ratio",0,0);
  cprintf("%s  \t","arrival_time_ratio");
  cprintf("%s  \t","executed_cycle_ratio");
  print_fixed_size_str("cycles",0,0);
  cprintf("\n");
  cprintf("----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
  struct proc *p;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";

    int rank = (p->priority_ratio/p->tickets) + (p->arrival_time*p->arrival_time_ratio) + (p->executed_cycle_ratio*p->cycle);
    print_fixed_size_str(p->name,0,0);
    print_fixed_size_str(" ",1,p->pid);
    print_fixed_size_str(state,0,0);
    print_fixed_size_str(" ",1,p->queue);
    print_fixed_size_str(" ",1,p->tickets);
    print_fixed_size_str(" ",1,p->priority_ratio);
    print_fixed_size_str(" ",1,p->arrival_time_ratio);
    print_fixed_size_str(" ",1,p->executed_cycle_ratio);
    print_fixed_size_str(" ",1,rank);
    cprintf("\n");



  }
  return 0;
}

void
semaphore_initialize(int i, int v_, int m_)
{
  semaphores[i].v = v_;
  semaphores[i].m = m_;
  semaphores[i].last = 0;
}

void
semaphore_acquire(int i)
{
  acquire(&semaphores[i].lock);
  if (semaphores[i].m < semaphores[i].v)
  {
    semaphores[i].m++;
  }
  else
  {
    semaphores[i].proc[semaphores[i].last] = myproc();
    sleep(myproc(), &semaphores[i].lock);
    semaphores[i].last++;
  }
  release(&semaphores[i].lock);
}

void
semaphore_release(int i)
{
  acquire(&semaphores[i].lock);
  if (semaphores[i].m < semaphores[i].v && semaphores[i].m > 0)
  {
    semaphores[i].m--;
  }
  else if (semaphores[i].m == semaphores[i].v)
  {
    if (semaphores[i].last == 0)
    {
      semaphores[i].m--;
    }
    else
    {
      wakeup(semaphores[i].proc[0]);
      for (int j = 1; j < NPROC; j++)
      {
        semaphores[i].proc[j-1] = semaphores[i].proc[j];
      }
    }
  }
  release(&semaphores[i].lock);
}