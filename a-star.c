// a-star.c

#include <limits.h>
#include "dlist.h"
#include "a-star.h"

typedef struct _a_star_node_info_t {
	long g, h, f;
	a_star_node_t* prev;
	int close;
} a_star_node_info_t;

#define	NINFO(n)	((a_star_node_info_t*)(n)->reserved)

static a_star_node_info_t* create_node_info() {
	a_star_node_info_t* ni = (a_star_node_info_t*)malloc(sizeof(a_star_node_info_t));
	ni->g = ni->h = ni->f = LONG_MAX;
	ni->prev = 0;
	ni->close = 0;
	return ni;
}

static int get_path(const a_star_graph_t* graph, a_star_node_t*** path) {
	a_star_node_t* step;
	int i, n;

	// count path steps
	for(step=graph->end, n=0; step; step=NINFO(step)->prev, n++)
		;
	// build a path array
	*path = (a_star_node_t**)malloc(sizeof(a_star_node_t*) * n);
	for(step=graph->end, i=0; step; step=NINFO(step)->prev, i++)
		(*path)[n-i-1] = step;

	return n;
}

static int _cbCmpEdges(const void* v1, const void* v2) {
	const a_star_edge_t* e1 = *(const a_star_edge_t**)v1;
	const a_star_edge_t* e2 = *(const a_star_edge_t**)v2;
	return e1->from - e2->from;
}

static int _cmpNodes(const void* e1, const void* e2, void* cookie) {
	const a_star_node_t* n1 = (const a_star_node_t*)e1;
	const a_star_node_t* n2 = (const a_star_node_t*)e2;
	long f1 = NINFO(n1)->f;
	long f2 = NINFO(n2)->f;
	return f1 - f2;
}

int a_star_shortest_path(
		a_star_graph_t* graph,
		a_star_distance_func_t g_dist,
		a_star_distance_func_t h_dist,
		a_star_progress_func_t progress,
		void* cookie,
		a_star_node_t*** path
		) {
	dlist_t *l_open;
	int i, n=0, complete=0;
	a_star_progress_info_t progressInfo={0,0,0,0,0};

	// arguments validation
	if( ! graph || ! graph->nodes || ! graph->edges || ! graph->begin || ! graph->end )
		return -1;
	if( ! g_dist || ! h_dist )
		return -1;
	if( ! path )
		return -1;

	// initialization
	progressInfo.maxDistance = h_dist(graph->begin, graph->end, cookie);
	dlist_init(0, _cmpNodes, 0, &l_open);

	*path = 0;

	// sort edges by 'from'
	qsort(graph->edges, graph->nedges, sizeof(a_star_edge_t*), _cbCmpEdges);

	// attach private data to each node
	for(i=0; i < graph->nnodes; i++)
		graph->nodes[i]->reserved = create_node_info();

	dlist_push_ordered(l_open, graph->begin);
	for(;;) {
		// since l_open is ordered by F-cost, the first element should be the lowest
		a_star_node_t* curr = (a_star_node_t*)dlist_get_front(l_open);
		if( ! curr )
			break; // FAILED!

		if( progress ) {
			progressInfo.nframe++;
			progressInfo.currDistance = h_dist(curr, graph->end, cookie);
			progressInfo.analized = progressInfo.current = curr;
			(*progress)(&progressInfo, cookie);
		}

		dlist_remove(l_open, curr);
		NINFO(curr)->close = 1;

		if( curr == graph->end ) {
			complete = 1;
			break; // FINISH!
		}

		// search edges sorted array for beginning of 'from'==curr
		for(i=0; i < graph->nedges && graph->edges[i]->from != curr; i++)
			;
		for(; i < graph->nedges && graph->edges[i]->from==curr; i++) {
			a_star_node_t* neighbor = graph->edges[i]->to;
			if( NINFO(neighbor)->close )
				continue;
			if( dlist_exists(l_open, neighbor) )
				continue;
			if( progress ) {
				progressInfo.analized = neighbor;
				(*progress)(&progressInfo, cookie);
			}
			long gCost = (*g_dist)(curr, neighbor, cookie) + graph->edges[i]->cost;
			if( gCost > NINFO(neighbor)->g )
				continue;
			long hCost = (*h_dist)(neighbor, graph->end, cookie);
			if( gCost == NINFO(neighbor)->g && hCost >= NINFO(neighbor)->h )
				continue;
			NINFO(neighbor)->g = gCost;
			NINFO(neighbor)->h = hCost;
			NINFO(neighbor)->f = gCost + hCost;
			NINFO(neighbor)->prev = curr;
			dlist_push_ordered(l_open, neighbor);
		}
	}

	if( complete )
		n = get_path(graph, path);

	for(i=0; i < graph->nnodes; i++)
		free(graph->nodes[i]->reserved);

	dlist_deinit(l_open);

	return n;
}
