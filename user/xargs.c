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

    if (nleft == 0) {
        nleft = read(0, io_buf, 4096); 
        nread = 0;
    }

    while (nread < nleft) {
        for (;; nread++) {
            if (io_buf[nread] == '\n') {
                nread++;
                break;
            }
            *p++ = io_buf[nread];
        }
        *p = 0;
        return 1;
    }

    if (nleft < nread) {
        return -1;
    }

    // Nothing to read
    return 0;
}

void print_argv(char **argv) {
    for (int i = 0; i < 5; i++) {
        printf("%p %s\n", argv[i], argv[i]);
        if (!argv[i]) break;
    }
    printf("\n");
    return;
}

int main(int argc, char *argv[]) {

    char buf[512];    // Argument values are limited to be 128-bytes long
    char *new_argv[MAXARG];
    int pid, i;

    if (argc < 2) {
        fprintf(2, "Usage: xargs <command>");
        exit(1);
    }

    while (readline(buf) > 0) {
        memset(new_argv, 0, MAXARG);
        for (i = 1; i < argc; i++) {
            new_argv[i - 1] = argv[i];
        }
        new_argv[i - 1] = buf;

        if ((pid = fork()) == 0) {
            exec(new_argv[0], new_argv);
            fprintf(2, "Execution error!\n");
            exit(1);
        }
        else {
            wait(&pid);
        }
    }
    exit(0);
}
