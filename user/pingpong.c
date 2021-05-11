// pingpong - Pingpong a byte between a parent and a child
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"


int main(int argc, char *argv[])
{
    int p[2];
    char buf[1] = {'a'};
    int n, pid;
    pipe(p);

    if ((pid = fork()) == 0) {
        n = read(p[0], buf, 1);
        if (n > 0) {
            printf("%d: received ping\n", getpid());
        }
        if (n < 0) {
            fprintf(2, "Ping error\n");
            exit(1);
        }

        if (write(p[1], buf, 1) < 0) {
            fprintf(2, "Pong error!\n");
            exit(1);
        }

        close(p[0]);
        close(p[1]);
        exit(0);
    }
    else {
        if (write(p[1], buf, 1) < 0) {
            fprintf(2, "Write ping error\n");
            exit(1);
        }

        wait(&pid);
        n = read(p[0], buf, 1);
        if (n > 0) {
            printf("%d: received pong\n", getpid());
        }
        if (n < 0) {
            fprintf(2, "Read pong error!\n");
            exit(1);
        }
        close(p[0]);
        close(p[1]);
        exit(0);
    }
    exit(0); 
}
