#include "types.h"
#include "stat.h"
#include "user.h"

int main(void)
{
    int main_pid = getpid();
    printf(1, "in main: pid: %d\n", main_pid);
    printf(1, "in main: main_children: %d\n\n", get_children(main_pid));
    int pid;

    // for (int i = 0; i < 4; i++)
    // {
        pid = fork();
        if (pid < 0)
        {
            printf(2, "fork failed\n");
            exit();
        }
        if (pid == 0)
        {
            // printf(1, "in child: pid: %d, parent_pid: %d\n", getpid(), get_parent_id());
            exit();
        }
        else
        {
            printf(1, "in main: child_pid: %d, my_children: %d\n", pid, get_children(getpid()));
            printf(1, "in main: main_children: %d\n\n", get_children(main_pid));
            pid = fork();
            if (pid < 0)
            {
                printf(2, "fork failed\n");
                exit();
            }
            if (pid == 0)
            {
                // printf(1, "in child: pid: %d, parent_pid: %d\n", getpid(), get_parent_id());
                int gpid = fork();
                if (gpid < 0)
                {
                    printf(2, "fork failed\n");
                    exit();
                }
                if (gpid == 0)
                {
                    // asdfasd
                }
                else
                {
                    printf(1, "in child: main_children: %d\n\n", get_children(main_pid));
                }

                exit();
            }
            else
            {
                printf(1, "in main: child_pid: %d, my_children: %d\n", pid, get_children(getpid()));
                printf(1, "in main: main_children: %d\n\n", get_children(main_pid));
                pid = fork();
                if (pid < 0)
                {
                    printf(2, "fork failed\n");
                    exit();
                }
                if (pid == 0)
                {
                    // printf(1, "in child: pid: %d, parent_pid: %d\n", getpid(), get_parent_id());
                    exit();
                }
                else
                {
                    printf(1, "in main: child_pid: %d, my_children: %d\n", pid, get_children(getpid()));
                    printf(1, "in main: main_children: %d\n\n", get_children(main_pid));
                }
            }
        }
    // }

    exit();
}