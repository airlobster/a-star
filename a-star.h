// a-star.h

#ifndef _A_STAR_H_
#define _A_STAR_H_

#include <stdlib.h>

typedef struct _a_star_node_t {
	void* reserved;	// for use by a-star algorithm only!
} a_star_node_t;

typedef struct _a_star_edge_t {
	a_star_node_t *from, *to;
} a_star_edge_t;

typedef struct _a_star_graph_t {
	a_star_node_t** nodes;	// array of node pointers
	size_t nnodes;
	a_star_edge_t* edges;	// array of edges
	size_t nedges;
	a_star_node_t *begin, *end;
} a_star_graph_t;

typedef struct _a_star_progress_info_t {
	long nframe;
	unsigned long currDistance, maxDistance;
	a_star_node_t *analized, *current;
} a_star_progress_info_t;

typedef long(*a_star_distance_func_t)(const a_star_node_t* n1, const a_star_node_t* n2, void* cookie);
typedef void (*a_star_progress_func_t)(const a_star_progress_info_t* info, void* cookie);

/**
 * Returns length of path on success, or a negative value on error.
 * If greater then zero, the returned path array should be deallocated using free().
 * 
 * the 'progress' argument may be null if no progress is needed.
 **/
int a_star_shortest_path(
		a_star_graph_t* graph,
		a_star_distance_func_t g_dist,
		a_star_distance_func_t h_dist,
		a_star_progress_func_t progress,
		void* cookie,
		a_star_node_t*** path
		);

#endif
