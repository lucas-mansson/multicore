import java.util.Scanner;
import java.util.Iterator;
import java.util.ListIterator;
import java.util.LinkedList;
import java.util.concurrent.Semaphore;
import java.io.*;
import java.util.concurrent.atomic.LongAdder;
import java.util.concurrent.locks.ReentrantLock;

class Graph {
    int nbrThread = 2;
    int s;
    int t;
    int n;
    int m;
    LongAdder tot_ex = new LongAdder();
    LongAdder tot_wait_lock = new LongAdder();
    private ReentrantLock queue_lock = new ReentrantLock();
    private ReentrantLock[] node_locks;
    Node excess; // list of nodes with excess preflow
    Node node[];
    Edge edge[];

    Graph(Node node[], Edge edge[]) {
        this.node = node;
        this.n = node.length;
        this.edge = edge;
        this.m = edge.length;
        this.node_locks = new ReentrantLock[node.length];
        for (int i = 0; i < node_locks.length; i++) {
            node_locks[i] = new ReentrantLock();
        }
    }

    void enter_excess(Node u) {
        try {
            long before = System.nanoTime();
            queue_lock.lock();
            long after = System.nanoTime();
            tot_wait_lock.add(after - before);
            if (u != node[s] && u != node[t]) {
                u.next = excess;
                excess = u;
            }
        } catch (Exception e) {
            System.exit(1);
            // do nothing
        } finally {
            queue_lock.unlock();
        }
    }

    Node other(Edge a, Node u) {
        if (a.u == u)
            return a.v;
        else
            return a.u;
    }

    void relabel(Node u) {
        u.h++;
        enter_excess(u);
    }

    void push(Node u, Node v, Edge a) {
        int oldve = v.e;
        int amount = 0;
        try {
            if (u.i > v.i) {
                long before = System.nanoTime();
                node_locks[u.i].lock();
                node_locks[v.i].lock();
                long after = System.nanoTime();
                tot_wait_lock.add(after - before);
            } else {
                long before = System.nanoTime();
                node_locks[v.i].lock();
                node_locks[u.i].lock();
                long after = System.nanoTime();
                tot_wait_lock.add(after - before);
            }
            oldve = v.e;

            if (a.u == u) {
                amount = Math.min(u.e, (a.c - a.f));
                a.f += amount;
            } else {
                amount = Math.min(u.e, a.f + a.c);
                a.f -= amount;
            }

            u.e -= amount;
            v.e += amount;
        } catch (Exception e) {
            // do nothing
        } finally {
            node_locks[v.i].unlock();
            node_locks[u.i].unlock();
        }

        assert (amount >= 0);
        assert (u.e >= 0);
        assert (Math.abs(a.f) <= a.c);

        if (v.e > 0 && !(oldve > 0)) {
            enter_excess(v);
        }
        if (u.e > 0) {
            enter_excess(u);
        }
    }

    int preflow(int source, int t) {
        ListIterator<Edge> iter;
        int b;
        Edge a;
        Node u;
        Node v;

        this.s = source;
        this.t = t;
        node[source].h = n; // setting source node to height n.

        // initial push from source
        iter = node[s].adj.listIterator();
        while (iter.hasNext()) {
            a = iter.next();

            node[source].e += a.c;

            push(node[source], other(a, node[source]), a);
        }
        Thread[] threads = new Thread[nbrThread];

        tot_wait_lock.reset();
        for (int i = 0; i < threads.length; i++) {
            threads[i] = new Thread(() -> {
                try {

                    long before = System.nanoTime();
                    thread_loop();
                    long after = System.nanoTime();
                    tot_ex.add(after - before);
                } catch (Exception e) {
                    // DO NOTHING.
                    System.exit(1);
                }
            });
            threads[i].start();
        }

        for (int i = 0; i < threads.length; i++) {
            try {
                threads[i].join();
            } catch (Exception e) {
                // do nothing
            }
        }
        System.err.println("total time spent waiting for lock: " + tot_wait_lock +
                "\nTotal time spent executing:" + tot_ex +
                "\nfraction waitng for lock:" +
                (tot_wait_lock.floatValue() / tot_ex.floatValue()));
        return node[t].e;
    }

    void thread_loop() throws InterruptedException {
        ListIterator<Edge> iter;
        int direction;
        Edge a;
        Node u;
        Node v;
        int proccessed = 0;
        while (excess != null) {
            try {
                long before = System.nanoTime();
                queue_lock.lock();
                u = excess;
                v = null;
                a = null;
                excess = u.next;
                long after = System.nanoTime();
                tot_wait_lock.add(after - before);
            } finally {
                queue_lock.unlock();
            }
            iter = u.adj.listIterator();
            while (iter.hasNext()) {
                a = iter.next();
                if (u == a.u) {
                    v = a.v;
                    direction = 1;
                } else {
                    v = a.u;
                    direction = -1;
                }
                if (u.h > v.h && direction * a.f < a.c) {
                    break;
                } else {
                    v = null;
                }
            }
            if (v != null) {
                proccessed++;
                push(u, v, a);
            } else
                relabel(u);
        }
        System.err.println("Thread exited with " + proccessed + " pushed nodes.");
    }
}

class Node {
    int h;
    int e;
    int i;
    Node next;
    LinkedList<Edge> adj;

    Node(int i) {
        this.i = i;
        adj = new LinkedList<Edge>();
    }

    public String toString() {
        return "h = " + h + " e = " + e + " i = " + i;

    }
}

class Edge {
    Node u;
    Node v;
    int f;
    int c;

    Edge(Node u, Node v, int c) {
        this.u = u;
        this.v = v;
        this.c = c;

    }
}

class Preflow {
    public static void main(String args[]) {
        double begin = System.currentTimeMillis();
        Scanner s = new Scanner(System.in);
        int n;
        int m;
        int i;
        int u;
        int v;
        int c;
        int f;
        Graph g;

        n = s.nextInt();
        m = s.nextInt();
        s.nextInt();
        s.nextInt();
        Node[] node = new Node[n];
        Edge[] edge = new Edge[m];

        for (i = 0; i < n; i += 1)
            node[i] = new Node(i);

        for (i = 0; i < m; i += 1) {
            u = s.nextInt();
            v = s.nextInt();
            c = s.nextInt();
            edge[i] = new Edge(node[u], node[v], c);
            node[u].adj.addLast(edge[i]);
            node[v].adj.addLast(edge[i]);
        }

        g = new Graph(node, edge);
        f = g.preflow(0, n - 1);
        double end = System.currentTimeMillis();
        System.out.println("t = " + (end - begin) / 1000.0 + " s");
        System.out.println("f = " + f);
    }
}
