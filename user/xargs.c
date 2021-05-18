// xargs - Execute a command with dirrferent lines of arguements.
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

static char io_buf[4096];
static int nread = 0;
static int nleft = 0;

// readline - Simple buffered io that read a line from standard input.
int readline(char *line) {
    char *p = line;

    if (nleft == 0)
        nleft = read(0, io_buf, 4096); 

    while (nleft > 0) {
        nread = 0;
        for (; nread < nleft; nread++) {
            if (io_buf[nread] == '\n') {
                nread++;
                break;
            }
            *p++ = io_buf[nread];
        }
        *p = 0;
        nleft -= nread;
        return 1;
    }

    if (nleft < 0) {
        return -1;
    }

    // Nothing to read
    return 0;
}

void print_argv(char **argv) {
    char *p = argv[0];
    while (p != 0) {
        printf("%p\n", p);
    }
    return;
}

int main(int argc, char *argv[]) {

    char buf[512];    // Argument values are limited to be 128-bytes long
    int pid;

    if (argc < 2) {
        fprintf(2, "Usage: xargs <command>");
        exit(1);
    }

    while (readline(buf) > 0) {
        print_argv(argv);
        argv[argc] = buf;
        argv[argc + 1] = 0;
        if ((pid = fork()) == 0) {
            exec(argv[1], argv + 1);
            fprintf(2, "Execution error!\n");
            exit(1);
        }
        else {
            wait(&pid);
        }
    }
    exit(0);
}
