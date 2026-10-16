#include <assert.h>
#include <bits/pthreadtypes.h>
#include <ctype.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

#define TIME 0  /* for timing on power.		*/
#define PRINT 0
#if PRINT
#define pr(...)                                                                \
  do {                                                                         \
    fprintf(stderr, __VA_ARGS__);                                              \
  } while (0)
#else
#define pr(...) /* no effect at all */
#endif

#define MIN(a, b) (((a) <= (b)) ? (a) : (b))

#define NBR_THREADS 8

typedef struct xedge_t xedge_t;

struct xedge_t {
  int u; /* one of the two nodes.	*/
  int v; /* the other. 			*/
  int c; /* capacity.			*/
};


typedef struct graph_t graph_t;
typedef struct node_t node_t;
typedef struct edge_t edge_t;
typedef struct list_t list_t;

struct list_t {
  edge_t *edge;
  list_t *next;
};

struct node_t {
  atomic_int height;   /* height.			*/
  int excess;   /* excess flow.			*/
  list_t *adj;  /* adjacency list.		*/
  node_t *next; /* with excess preflow.		*/
};

struct edge_t {
  node_t *node_1; /* one of the two nodes.	*/
  node_t *node_2; /* the other. 			*/
  int flow;       /* flow > 0 if from u to v.	*/
  int capacity;   /* capacity.			*/
};

struct graph_t {
  int nbr_nodes;        /* nodes.			*/
  int nbr_edges;        /* edges.			*/
  node_t *nodes;        /* array of n nodes.		*/
  edge_t *edges;        /* array of m edges.		*/
  node_t *source;       /* source.			*/
  node_t *sink;         /* sink.			*/
  int top_of_ex; 
  int max_ex;
  int *excess_nodes; /* nodes with e > 0 except s,t.	*/
};

static char *progname;

static int id(graph_t *g, node_t *v) {
  /* return the node index for v.*/
  return v - g->nodes;
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

static graph_t *new_graph(int n, int m, int s, int t, xedge_t *e) {
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

  for (i = 0; i < m; i += 1) {
    a = e[i].u;
    b = e[i].v;
    c = e[i].c;
    u = &g->nodes[a];
    v = &g->nodes[b];
    connect(u, v, c, g->edges + i);
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
    if(g->top_of_ex >= g->max_ex){
      error("tried to add tto excess but its full %d", g->top_of_ex);
    }
    g->excess_nodes[g->top_of_ex++] = (int) (v - g->nodes); // this is sorta confusing but its using the base adress of the vector minus the adress for the node to get the index.
  }
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

node_t *getNode(size_t thread_id, node_t *excess_nodes){
    node_t *u = excess_nodes;
    for (int i = 0; i < thread_id && u != NULL; i++){
      u = u->next;
    }
    return u;
}

/*
 * let each thread decide on how much its nodes should push to a neighbor or
 * that a relabel is needed
 */
typedef struct work_t work_t;

struct work_t {
  node_t *curr_node;
  node_t *neighbor;
  edge_t *edge;
  // int amount;
  bool relabel;
  int newHeight;
};

// calculate work struct and add to work vector
static void phase_1(node_t *u, node_t *v, edge_t *edge, list_t *p, work_t *work, graph_t *graph) {
  int flow_direction;

  v = NULL;
  p = u->adj;
  int min_height = __INT_MAX__;

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

    if(flow_direction * edge->flow < edge->capacity){
      if(v->height < min_height){
        min_height = v->height;
      }
      if(u->height > v->height) {
        break;
      }
    }
    v = NULL;
  }
  assert(min_height != __INT_MAX__);
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
    work->newHeight = min_height + 1;
    atomic_store_explicit(&u->height, min_height + 1, memory_order_relaxed);
  }
}

static void phase_2(work_t **work_list, size_t *work_counts, graph_t *graph) {
  graph->top_of_ex=0;

  for (int i = 0; i < NBR_THREADS; i++) {

    for(size_t j = 0; j < work_counts[i]; ++j){
      work_t *work = &work_list[i][j];
      if (work->relabel == true) {
        pr("SHOULD RELABEL\n");
        add_to_excess_list(graph, work->curr_node);
          
      } else {
        pr("SHOULD PUSH\n");
        push(graph, work->curr_node, work->neighbor, work->edge);
      } 
    }
    work_counts[i] = 0;
  }
}

pthread_barrier_t barrier1;

struct work_args_t {
  graph_t *graph;
  work_t **vector; // vecotr of pointers
  size_t *work_counts;
  size_t id;
};
static void *work(void *arg) {
  struct work_args_t *args = arg;
  node_t *u;
  node_t *v;
  edge_t *edge;
  list_t *p;
  graph_t *graph;
  work_t **work_list;
  int thread_id ;
  size_t *work_counts = args->work_counts;
  
  thread_id = args->id;
  graph = args->graph;
  work_list = args->vector;

  pr("thread id %d\n",thread_id);
  
  while (true) {
    int next = thread_id;
    u = &graph->nodes[graph->excess_nodes[next]];
    while(next < graph->top_of_ex){
      work_t *work = &work_list[thread_id][work_counts[thread_id]++];

      phase_1(u, v, edge, p, work, graph);
      next += NBR_THREADS;
      u = &graph->nodes[graph->excess_nodes[next]];
    }

    // All threads wait here
    pr("Wait 1\n");
    int ret = pthread_barrier_wait(&barrier1);
    pr("Ret: %d\n", ret);


    if (ret == PTHREAD_BARRIER_SERIAL_THREAD) {
      phase_2(work_list, work_counts, graph);
    }
    pr("Wait 2\n");
    pthread_barrier_wait(&barrier1);
    pr("SINK %d\n", graph->sink->excess);
    pr("SOURCE %d\n", graph->source->excess);
    if (graph->sink->excess > 0 &&
        graph->sink->excess == (-(graph->source->excess))) {
      return NULL;
    }
  }

  return NULL;
}

static int xpreflow(graph_t *graph) {
  node_t *source;
  node_t *u;
  node_t *v;
  edge_t *edge;
  list_t *p;
  int flow_direction;


  source = graph->source;
  atomic_store_explicit(&source->height, graph->nbr_nodes, memory_order_relaxed);

  p = source->adj;
  graph->top_of_ex=0;
  graph->max_ex = 2 * graph->nbr_nodes;
  graph->excess_nodes = xmalloc(graph->max_ex * sizeof(int));

  while (p != NULL) {  
    edge = p->edge;
    p = p->next;

    int amount;

    if (source == edge->node_1) {
        amount = edge->capacity - edge->flow;
        edge->flow += amount;
    } else {
        amount = edge->capacity + edge->flow;
        edge->flow -= amount;
    }

    node_t *to = other(source, edge);

    source->excess -= amount;
    to->excess += amount;

    add_to_excess_list(graph, to);
  }
  work_t *work_list[NBR_THREADS] = {0};


  int nbr_threads = NBR_THREADS;
  pthread_barrier_init(&barrier1, NULL, NBR_THREADS);
  pthread_t thread[nbr_threads];
  size_t work_counts[NBR_THREADS] = {0};
  struct work_args_t thread_arg[nbr_threads];
  
  for (int i = 0; i < nbr_threads; i++) {
    pr("Creating thread %d \n", i);
    work_list[i] = xmalloc(graph->nbr_nodes * sizeof(work_t));
    thread_arg[i] =(struct work_args_t) {graph, work_list, work_counts, i}; // if i dont do this all the threads get the same id.
    if (pthread_create(&thread[i], NULL, work, &thread_arg[i]) != 0) {
      error("pthread create failed \n");
      free(work_list[i]); // i guess?
    }
  }

  for (int i = 0; i < nbr_threads; i++) {
    if (pthread_join(thread[i], NULL) != 0) {
      error("pthread join failed");
    }
    free(work_list[i]);
    pr("Destroying thread %d \n", i);
  }

  return graph->sink->excess;
}

static void free_graph(graph_t *g);

int preflow(int n, int m, int s, int t, xedge_t *e){
    graph_t *g;
    int f;
    double begin;
    double end;

  #if TIME
    init_timebase();
    begin = timebase_sec();
  #endif
    g = new_graph(n, m, s, t, e);
    f = xpreflow(g);
    free_graph(g);
  #if TIME
    end = timebase_sec();
    printf("t = %10.3lf s\n", end - begin);
  #endif
    return f;
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
  free(g->excess_nodes);
  free(g);
}


