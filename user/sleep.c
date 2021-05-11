// sleep - Pause the system for a given number of ticks
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"


int
main(int argc,  char *argv[]) {
    int nticks;

    if (argc < 2) {
        fprintf(2, "Usage: sleep <n>\n");
        exit(1);
    }
    nticks = atoi(argv[1]);
    sleep(nticks);
    exit(0);
}
