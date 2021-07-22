#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int g(int x) {
  return x+3;
}

int f(int x) {
  return g(x);
}

void main(void) {
//  printf("%d %d\n", f(8)+1, 13);
//  unsigned int i = 0x00646c72;
//  char *p = (char *)&i;
//  printf("H%x Wo%c", 57616, p[0]);
    printf("x=%d y=%d", 3);
  exit(0);
}
