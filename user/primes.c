// primes - Implement a prime sieve via multi-processsing and pipes.
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

#define START 2
#define END 35


int sieve(int read_fd, int modulo) {

    int p[2], pid, n, data;
    char *buf[4];
    int mod;

    if (modulo == END + 1) {  // End of the process
        close(read_fd);
        return 0;
    }
    else {
        pipe(p);
        if ((pid = fork()) == 0) {    // Create a right neighbor at the pipe
            close(read_fd);
            close(p[1]);
            sieve(p[0], modulo + 1);
            return 0;
        }
        else {
            close(p[0]);
            while ((n = read(read_fd, buf, 4)) > 0) {
                data = *(int *)buf;
                mod = data % modulo;     
                if (mod == 0 && data == modulo) {
                    printf("prime %d\n" , data);
                }
                else if (mod == 0 && data != modulo)
                {
                    continue;
                }
                else {
                    write(p[1], buf, 4);
                }
            }
            close(read_fd);
            close(p[1]);
            wait(&pid);
        }
        return 0;
    }
}

int main(int argc, char *argv[]) {
    int p[2], i, pid;
    if (pipe(p) < 0) {
        fprintf(2, "Error on opening a pipe");
        exit(1);
    }

    if ((pid = fork()) == 0) {
        // Parent process only needs to write.
        close(p[0]);
        for (i = START; i < 35; i++) {
            write(p[1], &i, 4);
        }
        close(p[1]);
        exit(0);
    }
    else {
        // Child only needs to read.
        close(p[1]);
        sieve(p[0], START);
        wait(&pid);
        exit(0);
    }
}