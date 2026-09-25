#[macro_use] extern crate text_io;

use std::sync::{Mutex,Arc};
use std::collections::LinkedList;
use std::cmp::{self, min};
use std::thread;
use std::collections::VecDeque;

struct Node {
	i:	usize,			/* index of itself for debugging.	*/
	excess:	i32,			/* excess preflow.			*/
	height:	i32,			/* height.				*/
}

struct Edge {
        u:      usize,  
        v:      usize,
        flow:      i32,
        capacity:      i32,
}

impl Node {
	fn new(ii:usize) -> Node {
		Node { i: ii, excess: 0, height: 0 }
	}

}

impl Edge {
        fn new(uu:usize, vv:usize,cc:i32) -> Edge {
                Edge { u: uu, v: vv, flow: 0, capacity: cc }      
        }
}

fn main() {
	let n: usize = read!();		/* n nodes.						*/
	let m: usize = read!();		/* m edges.						*/
	let _c: usize = read!();	/* underscore avoids warning about an unused variable.	*/
	let _p: usize = read!();	/* c and p are in the input from 6railwayplanning.	*/
	let mut nodes = vec![];
	let mut edges = vec![];
	let mut adj: Vec<LinkedList<usize>> =Vec::with_capacity(n);
	let mut excess: VecDeque<usize> = VecDeque::new();
	let debug = false;

	let s = 0;
	let t = n-1;

	println!("n = {}", n);
	println!("m = {}", m);

	for i in 0..n {
		let u:Node = Node::new(i);
		nodes.push(Arc::new(Mutex::new(u))); 
		adj.push(LinkedList::new());
	}

	for i in 0..m {
		let u: usize = read!();
		let v: usize = read!();
		let c: i32 = read!();
		let e:Edge = Edge::new(u,v,c);
		adj[u].push_back(i);
		adj[v].push_back(i);
		edges.push(Arc::new(Mutex::new(e))); 
	}

	if debug {
		for i in 0..n {
			print!("adj[{}] = ", i);
			let iter = adj[i].iter();

			for e in iter {
				print!("e = {}, ", e);
			}
			println!("");
		}
	}

	println!("initial pushes");
	let iter = adj[s].iter();
	nodes[0].lock().unwrap().height = n as i32;

	for &edge_idx in iter {
		let mut edge = edges[edge_idx].lock().unwrap();
		let neighbor_idx = edge.v;
		let c = edge.capacity;
	
		let source = &mut nodes[s].lock().unwrap();
		let neighbor = &mut nodes[neighbor_idx].lock().unwrap();

		edge.flow = c;
		source.excess -= c;
		neighbor.excess += c;

		add_to_excess_list(neighbor_idx, &mut excess, t);

		println!("Source with height {} pushing {} to node {}", source.height, c, neighbor_idx);
	}

	while !excess.is_empty() {
		let curr_node_i = excess.pop_front().unwrap();


		let iter = adj[curr_node_i].iter();
		for &edge_idx in iter {
			let u = &mut nodes[curr_node_i].lock().unwrap();
			let mut edge = edges[edge_idx].lock().unwrap();

			if u.excess == 0 {
				break;
			}		
			
			println!("Curr node: {} with excess {}", curr_node_i, u.excess);

			let mut neighbor_i;
			let mut direction = 1;
			if curr_node_i == edge.u {
				neighbor_i = edge.v;
				direction = 1;
			} else if curr_node_i == edge.v {
				neighbor_i = edge.u;
				direction = -1;
			} else {
				panic!("Illegal state");
			}
			let v = &mut nodes[neighbor_i].lock().unwrap();

			
			let can_push = u.height > v.height && direction * edge.flow < edge.capacity;
			if can_push {
				push(u, v, &mut edge, &mut excess, t);
			} else {
				println!("Relabelling node {} with height {}", curr_node_i, u.height);
				relabel(u);
				add_to_excess_list(u.i, &mut excess, t);
			}
		}
	}

	println!("f = {}", nodes[t].lock().unwrap().excess);

}

fn push(u: &mut Node, v: &mut Node, edge: &mut Edge, excess: &mut VecDeque<usize>, sink: usize) {
	let mut delta = 0;

	if u.i == edge.u {
		delta = min(u.excess, edge.capacity - edge.flow);
		edge.flow += delta;
	} else if u.i == edge.v {
		delta = min(u.excess, edge.capacity + edge.flow);
		edge.flow -= delta;
	} else {
		panic!("Bruh");
	}

	println!("Pushing {} from {} (e={}) to {} (e={})", delta, u.i, u.excess, v.i, v.excess);

	u.excess -= delta;
	v.excess += delta;

	assert!(delta >= 0);
	assert!(u.excess >= 0);
	assert!(edge.flow.abs() <= edge.capacity);


	println!("u.excess {}", u.excess);
	println!("v.excess {}", v.excess);
	if u.excess > 0 {
		add_to_excess_list(v.i, excess, sink);
	}
	// if v has delta flow, it previously had 0
	if v.excess - delta == 0 {
		add_to_excess_list(v.i, excess, sink);
	}
}

fn relabel(node: &mut Node) {
	if node.excess <= 0 {
		panic!("You STUPID");
	}
	node.height += 1;
}

fn add_to_excess_list(node_i: usize, excess: &mut VecDeque<usize>, t: usize) {
	if node_i != t {
		excess.push_back(node_i);
	}
}