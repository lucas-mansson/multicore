#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct xedge_t xedge_t;

struct xedge_t {
  int u;
  int v;
  int c;
};

int preflow(int n, int m, int s, int t, xedge_t *e);

static int next_int(void) {
  int x = 0;
  int c;

  while (isdigit(c = getchar()))
    x = 10 * x + c - '0';

  return x;
}

int main(int argc, char *argv[]) {
  int n;
  int m;
  int i;
  int f;
  xedge_t *e;

  n = next_int();
  m = next_int();

  next_int();
  next_int();

  e = malloc(m * sizeof(xedge_t));

  for (i = 0; i < m; i++) {
    e[i].u = next_int();
    e[i].v = next_int();
    e[i].c = next_int();
  }

  f = preflow(n, m, 0, n - 1, e);

  printf("f = %d\n", f);

  free(e);

  return 0;
}
