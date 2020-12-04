#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main (int argc, char **argv) {
    int n = 0;

    if (argc < 2) {
        n = 1;
    } else {
        n = atoi(argv[1]);
    }

    int i = 0, j = 0;
    float x = 0;
    for (i = 0; i < n; i++) {
        if (fork() == 0) {
            for (j = 0; j < (1<<24); j++) {
                x = x + 123;
                x = x / 123;
                x = x * 123;
                x = x - 123;
            }
            exit();
        }
    }
    for (i = 0; i < n; i++) {
        wait();
    }
    printf(1, "end\n");
    exit();
}