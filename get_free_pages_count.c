#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main (void) {

    printf(1,"Number of free pages: %d\n",get_free_pages_count());
    exit();
}