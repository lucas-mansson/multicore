#include <assert.h>
#include <bits/pthreadtypes.h>
#include <ctype.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRINT 1
#if PRINT
#define pr(...)                                                                \
  do {                                                                         \
    fprintf(stderr, __VA_ARGS__);                                              \
  } while (0)
#else
#define pr(...) /* no effect at all */
#endif

#define MIN(a, b) (((a) <= (b)) ? (a) : (b))

#define NBR_THREADS 3

typedef struct graph_t graph_t;
typedef struct node_t node_t;
typedef struct edge_t edge_t;
typedef struct list_t list_t;

struct list_t {
  edge_t *edge;
  list_t *next;
};

struct node_t {
  int height;   /* height.			*/
  int excess;   /* excess flow.			*/
  list_t *adj;  /* adjacency list.		*/
  node_t *next; /* with excess preflow.		*/
  pthread_mutex_t node_mutex;
};

struct edge_t {
  node_t *node_1; /* one of the two nodes.	*/
  node_t *node_2; /* the other. 			*/
  int flow;       /* flow > 0 if from u to v.	*/
  int capacity;   /* capacity.			*/
  pthread_mutex_t edge_mutex;
};

struct graph_t {
  int nbr_nodes;        /* nodes.			*/
  int nbr_edges;        /* edges.			*/
  node_t *nodes;        /* array of n nodes.		*/
  edge_t *edges;        /* array of m edges.		*/
  node_t *source;       /* source.			*/
  node_t *sink;         /* sink.			*/
  node_t *excess_nodes; /* nodes with e > 0 except s,t.	*/
  pthread_mutex_t excess_nodes_mutex;
};

static char *progname;

static int id(graph_t *g, node_t *v) {
  /* return the node index for v.*/
  return v - g->nodes;
}

void lock_node(graph_t *g, node_t *u) {

  pthread_mutex_lock(&u->node_mutex);
  pr("locking node %d mutex \n", id(g, u));
}

void unlock_node(graph_t *g, node_t *u) {
  pr("unlocking node %d mutex \n", id(g, u));
  pthread_mutex_unlock(&u->node_mutex);
}

void lock_nodes(graph_t *g, node_t *u, node_t *v) {
  node_t *first = u;
  node_t *second = v;
  if (id(g, u) > id(g, v)) {
    first = v;
    second = u;
  }
  lock_node(g, first);
  lock_node(g, second);
}

void lock_excess_list(graph_t *g) {
  pthread_mutex_lock(&g->excess_nodes_mutex);
  // pr("locking excess_nodes_mutex \n");
}
void unlock_excess_list(graph_t *g) {
  // pr("unlocking excess_nodes_mutex \n");
  pthread_mutex_unlock(&g->excess_nodes_mutex);
}

void error(const char *fmt, ...) {
  /* print error message and exit.*/

  va_list ap;
  char buf[BUFSIZ];

  va_start(ap, fmt);
  vsprintf(buf, fmt, ap);

  if (progname != NULL)
    fprintf(stderr, "%s: ", progname);

  fprintf(stderr, "error: %s\n", buf);
  exit(1);
}

static int next_int() {
  int x;
  int c;

  x = 0;
  while (isdigit(c = getchar()))
    x = 10 * x + c - '0';

  return x;
}

static void *xmalloc(size_t s) {
  void *p;

  /* allocate s bytes from the heap and check that there was
   * memory for our request.
   *
   * memory from malloc contains garbage except at the beginning
   * of the program execution when it contains zeroes for
   * security reasons so that no program should read data written
   * by a different program and user.
   *
   * size_t is an unsigned integer type (printed with %zu and
   * not %d as for int).
   *
   */

  p = malloc(s);

  if (p == NULL)
    error("out of memory: malloc(%zu) failed", s);

  return p;
}

static void *xcalloc(size_t n, size_t s) {
  void *p;

  p = xmalloc(n * s);

  /* memset sets everything (in this case) to 0. */
  memset(p, 0, n * s);

  /* for the curious: so memset is equivalent to a simple
   * loop but a call to memset needs less memory, and also
   * most computers have special instructions to zero cache
   * blocks which usually are used by memset since it normally
   * is written in assembler code. note that good compilers
   * decide themselves whether to use memset or a for-loop
   * so it often does not matter. for small amounts of memory
   * such as a few bytes, good compilers will just use a
   * sequence of store instructions and no call or loop at all.
   *
   */

  return p;
}

static void add_edge(node_t *u, edge_t *e) {
  list_t *p;

  /* allocate memory for a list link and put it first
   * in the adjacency list of u.
   *
   */

  p = xmalloc(sizeof(list_t));
  p->edge = e;
  p->next = u->adj;
  u->adj = p;
}

static void connect(node_t *u, node_t *v, int c, edge_t *e) {
  /* connect two nodes by putting a shared (same object)
   * in their adjacency lists.
   *
   */

  e->node_1 = u;
  e->node_2 = v;
  e->capacity = c;

  add_edge(u, e);
  add_edge(v, e);
}

static node_t *other(node_t *u, edge_t *e) {
  if (u == e->node_1)
    return e->node_2;
  else
    return e->node_1;
}

static graph_t *new_graph(FILE *in, int n, int m) {
  graph_t *g;
  node_t *u;
  node_t *v;
  int i;
  int a;
  int b;
  int c;

  g = xmalloc(sizeof(graph_t));

  g->nbr_nodes = n;
  g->nbr_edges = m;

  g->nodes = xcalloc(n, sizeof(node_t));
  g->edges = xcalloc(m, sizeof(edge_t));

  g->source = &g->nodes[0];
  g->sink = &g->nodes[n - 1];
  g->excess_nodes = NULL;
  pthread_mutex_init(&g->excess_nodes_mutex, NULL);

  for (i = 0; i < m; i += 1) {
    a = next_int();
    b = next_int();
    c = next_int();
    u = &g->nodes[a];
    v = &g->nodes[b];
    connect(u, v, c, g->edges + i);
  }

  // init nodes mutexes
  for (int i = 0; i < n; i++) {
    pthread_mutex_init(&g->nodes[i].node_mutex, NULL);
    pr("initializing mutex for node %d\n", id(g, &g->nodes[i]));
  }
  // init edge mutexes
  for (int i = 0; i < m; i++) {
    pthread_mutex_init(&g->edges[i].edge_mutex, NULL);
    pr("initializing mutex for edge %d\n", i);
  }

  return g;
}

static void add_to_excess_list(graph_t *g, node_t *v) {
  /* put v at the front of the list of nodes
   * that have excess preflow > 0.
   *
   * note that for the algorithm, this is just
   * a set of nodes which has no order but putting it
   * it first is simplest.
   */

  if (v != g->sink && v != g->source) {
    v->next = g->excess_nodes;
    g->excess_nodes = v;
  }
}

static node_t *get_from_excess_list(graph_t *g) {
  node_t *v;

  /* take any node from the set of nodes with excess preflow
   * and for simplicity we always take the first.
   */

  v = g->excess_nodes;

  if (v != NULL)
    g->excess_nodes = v->next;

  return v;
}

static void push(graph_t *graph, node_t *from, node_t *to, edge_t *edge) {
  int remaining_capacity; /* remaining capacity of the edge. */

  pr("push from %d to %d: ", id(graph, from), id(graph, to));
  pr("f = %d, c = %d, so ", edge->flow, edge->capacity);

  if (from == edge->node_1) {
    remaining_capacity = MIN(from->excess, edge->capacity - edge->flow);
    edge->flow += remaining_capacity;
  } else {
    remaining_capacity = MIN(from->excess, edge->capacity + edge->flow);
    edge->flow -= remaining_capacity;
  }

  pr("pushing %d\n", remaining_capacity);

  from->excess -= remaining_capacity;
  to->excess += remaining_capacity;

  /* the following are always true. */
  assert(remaining_capacity >= 0);
  assert(from->excess >= 0);
  assert(abs(edge->flow) <= edge->capacity);

  if (from->excess > 0) {
    /* still some remaining so let u push more. */
    add_to_excess_list(graph, from);
  }

  if (to->excess == remaining_capacity) {
    /* since v has d excess now it had zero before and
     * can now push.
     */
    add_to_excess_list(graph, to);
  }
}

static void relabel(graph_t *g, node_t *u) {
  u->height += 1;

  pr("relabel %d now h = %d\n", id(g, u), u->height);

  add_to_excess_list(g, u);
}

/*
 * let each thread decide on how much its nodes should push to a neighbor or
 * that a relabel is needed
 */
typedef struct work_vector_t work_vector_t;
typedef struct work_t work_t;

struct work_t {
  node_t *curr_node;
  node_t *neighbor;
  edge_t *edge;
  // int amount;
  bool relabel;
};

struct work_vector_t {
  work_t *data;
  size_t size;
  size_t capacity;
  pthread_mutex_t mutex;
};

void init_list(work_vector_t *list) {
  list->data = NULL;
  list->size = 0;
  list->capacity = 0;
  pthread_mutex_init(&list->mutex, NULL);
}

void list_destroy(work_vector_t *list) {
  pthread_mutex_destroy(&list->mutex);
  free(list->data);
}

int grow(work_vector_t *list) {
  size_t newCap = list->capacity == 0 ? 4 : list->capacity * 2;
  work_t *newData = realloc(list->data, newCap * sizeof(work_t));
  if (!newData) {
    return -1;
  }
  list->data = newData;
  list->capacity = newCap;
  return 0;
}

void add_to_work_vector(work_vector_t *list, work_t *data) {
  pthread_mutex_lock(&list->mutex);
  if (list->size == list->capacity) {
    if (grow(list) != 0) {
      pthread_mutex_unlock(&list->mutex);
      return;
    }
  }
  list->data[list->size++] = *data;
  pthread_mutex_unlock(&list->mutex);
}

void remove_from_work_vector(work_vector_t *list, size_t pos) {
  assert(pos < list->size);

  pthread_mutex_lock(&list->mutex);

  for (size_t i = pos; i < list->size - 1; i++)
    list->data[i] = list->data[i + 1];
  list->size--;

  pthread_mutex_unlock(&list->mutex);
}

work_t *work_vector_get(work_vector_t *list, size_t pos) {
  assert(pos < list->size);
  pthread_mutex_lock(&list->mutex);

  work_t *val = &list->data[pos];

  pthread_mutex_unlock(&list->mutex);
  return val;
}

// calculate work struct and add to work vector
work_t *phase_1(node_t *u, node_t *v, edge_t *edge, list_t *p, graph_t *graph) {
  work_t *work = malloc(sizeof(work_t));
  int flow_direction;

  v = NULL;
  p = u->adj;

  while (p != NULL) {
    edge = p->edge;
    p = p->next;

    if (u == edge->node_1) {
      v = edge->node_2;
      flow_direction = 1;
    } else {
      v = edge->node_1;
      flow_direction = -1;
    }

    int should_break = false;
    if (u->height > v->height && flow_direction * edge->flow < edge->capacity) {
      should_break = true;
    }

    if (should_break) {
      break;
    }
    v = NULL;
  }

  work->curr_node = u;

  if (v != NULL) {
    // Should push
    work->neighbor = v;
    work->edge = edge;
    work->relabel = false;
  } else {
    // Should relabel
    work->neighbor = NULL;
    work->edge = NULL;
    work->relabel = true;
  }
  return work;
}

void phase_2(work_vector_t *work_list, graph_t *graph) {

  for (int i = 0; i < work_list->size; i++) {
    work_t *curr_work = work_vector_get(work_list, i);
    if (curr_work->neighbor == NULL && curr_work->edge == NULL &&
        curr_work->relabel == true) {
      // Should relabel
      pr("SHOULD RELABEL\n");
      relabel(graph, curr_work->curr_node);

    } else if (curr_work->neighbor != NULL && curr_work->edge != NULL &&
               curr_work->relabel == false) {
      // should push
      pr("SHOULD PUSH\n");
      push(graph, curr_work->curr_node, curr_work->neighbor, curr_work->edge);

    } else {
      pr("ERROR ILLEGAL STATE!!!!");
      exit(1);
    }
  }
  for (int i = 0; i < work_list->size; i++) {
    remove_from_work_vector(work_list, i);
  }
}

pthread_barrier_t barrier1;

struct work_args_t {
  graph_t *graph;
  work_vector_t *vector;
};
void *work(void *arg) {
  struct work_args_t *args = arg;

  node_t *u;
  node_t *v;
  edge_t *edge;
  list_t *p;
  graph_t *graph;
  work_vector_t *work_list;

  graph = args->graph;
  work_list = args->vector;

  bool finished = false;

  while (true) {
    lock_excess_list(graph);
    u = get_from_excess_list(graph);
    unlock_excess_list(graph);

    // For threads that find a node, calculate what needs to be pushed or if
    // relabel is necessary
    if (u != NULL) {
      work_t *work = phase_1(u, v, edge, p, graph);
      add_to_work_vector(work_list, work);
      pr("LIST SIZE: %zu\n", work_list->size);
    }

    // All threads wait here
    pr("Wait 1\n");
    int ret = pthread_barrier_wait(&barrier1);
    pr("Ret: %d\n", ret);

    // Only one thread performs the updating of the flows, heights, and excesses

    if (ret == PTHREAD_BARRIER_SERIAL_THREAD) {
      if (work_list->size == 0) {
        finished = true;
      }
      phase_2(work_list, graph);
    }
    pr("Wait 2\n");
    pthread_barrier_wait(&barrier1);
    pr("SINK %d\n", graph->sink->excess);
    pr("SOURCE %d\n", graph->source->excess);
    if (graph->sink->excess > 0 &&
        graph->sink->excess == graph->source->excess) {
      return NULL;
    }
  }

  return NULL;
}

int preflow(graph_t *graph) {
  node_t *source;
  node_t *u;
  node_t *v;
  edge_t *edge;
  list_t *p;
  int flow_direction;

  source = graph->source;
  source->height = graph->nbr_nodes;

  p = source->adj;

  /* start by pushing as much as possible (limited by
   * the edge capacity) from the source to its neighbors.
   */
  while (p != NULL) {
    edge = p->edge;
    p = p->next;

    source->excess += edge->capacity;
    push(graph, source, other(source, edge), edge);
  }

  work_vector_t *list = malloc(sizeof(work_vector_t));
  init_list(list);

  /* then loop until only s and/or t have excess preflow. */
  /* u is any node with excess preflow. */

  struct work_args_t thread_arg = {graph, list};
  int nbr_threads = NBR_THREADS;
  pthread_barrier_init(&barrier1, NULL, NBR_THREADS);
  pthread_t thread[nbr_threads];

  for (int i = 0; i < nbr_threads; i++) {
    printf("Creating thread %d \n", i);
    if (pthread_create(&thread[i], NULL, work, &thread_arg) != 0) {
      error("pthread create failed \n");
    }
  }

  for (int i = 0; i < nbr_threads; i++) {
    if (pthread_join(thread[i], NULL) != 0) {
      error("pthread join failed");
    }
    printf("Destroying thread %d \n", i);
  }

  return graph->sink->excess;
}

static void free_graph(graph_t *g) {
  int i;
  list_t *p;
  list_t *q;

  for (i = 0; i < g->nbr_nodes; i += 1) {
    p = g->nodes[i].adj;
    while (p != NULL) {
      q = p->next;
      free(p);
      p = q;
    }
  }
  free(g->nodes);
  free(g->edges);
  free(g);
}

int main(int argc, char *argv[]) {
  FILE *in;   /* input file set to stdin	*/
  graph_t *g; /* undirected graph. 		*/
  int f;      /* output from preflow.		*/
  int n;      /* number of nodes.		*/
  int m;      /* number of edges.		*/

  progname = argv[0]; /* name is a string in argv[0]. */

  in = stdin; /* same as System.in in Java.	*/

  n = next_int();
  m = next_int();

  /* skip C and P from the 6railwayplanning lab in EDAF05 */
  next_int();
  next_int();

  g = new_graph(in, n, m);

  fclose(in);

  f = preflow(g);

  printf("f = %d\n", f);

  free_graph(g);

  return 0;
}
