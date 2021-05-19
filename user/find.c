// find - Find the given file in the given directory.
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fs.h"


int match(char *path, char *file) {
    char *p;

    // Find the last backslash
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    return strcmp(p, file) == 0;
}


int find(char *path, char *target) {
    int fd;
    char buf[512], *p;
    struct stat st;
    struct dirent de;

    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "Cannot open %s\n", path);
        exit(1);
    }

    if (fstat(fd, &st) < 0) {
        close(fd);
        fprintf(2, "Cannot stat %s\n", path);
    }

    switch (st.type)
    {
    case T_FILE:
        if (match(path, target))
            printf("%s\n", path);
        close(fd);
        break;
    case T_DIR:
        while (read(fd, &de, sizeof(de)) == sizeof(de)) {
            memmove(buf, path, strlen(path));
            p = buf + strlen(path);
            *p++ = '/';
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;

            // Ignore ../ and ./
            if (de.inum == 0) 
                continue;
            if (!strcmp(buf + strlen(buf) - 3, "/.."))
                continue;
            if (!strcmp(buf + strlen(buf) - 2, "/."))
                continue;

            find(buf, target);
        }
        close(fd);
        break;
    
    default:
        break;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(2, "Usage: find <dir> <name>\n");
        exit(1);
    }

    find(argv[1], argv[2]);
    exit(0);
}