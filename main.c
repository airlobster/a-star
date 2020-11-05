// main.c

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <math.h>
#include <strings.h>
#include <unistd.h>
#include "a-star.h"
#include "dlist.h"

#ifdef _DEBUG_
#	include <assert.h>
#	define ASSERT(x)	assert(x)
#else
#	define ASSERT(x)
#endif

static inline int min(int a, int b) { return a < b ? a : b; }
static inline int max(int a, int b) { return a > b ? a : b; }
static inline int minmax(int a, int lo, int hi) { return min(max(a, lo),hi); }
static inline int inrange(int a, int lo, int hi) { return a >= lo && a <= hi; }

typedef enum _options_t {
	oCutCorners	= 1U << 0,
	oAnimate	= 1U << 1,
} options_t;

typedef struct _app_parameters_t {
	int rows, columns, barriers, startRow, startCol, endRow, endCol;
	unsigned int options;
} app_parameters_t;
static app_parameters_t parameters={0};

typedef enum _cell_attributes_t {
	caRegular	= 0,
	caBarrier	= 1U << 0,
	caPathStep	= 1U << 1,
	caStart		= 1U << 2,
	caEnd		= 1U << 3,
	caAnalized	= 1U << 4,
	caCurrent	= 1U << 5,
} cell_attributes_t;

typedef struct _cell_t {
	a_star_node_t a;
	int row, column;
	unsigned int attributes;
} cell_t;

typedef struct _grid_t {
	int rows, columns;
	cell_t* g;
	cell_t *start, *end;
	cell_t* current;
} grid_t;

static int getTerminalSize(int* rows, int* columns) {
	struct winsize ws;
	int err = ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
	if( err < 0 || (ws.ws_col==0 && ws.ws_row==0) )
		return -1;
	*rows = ws.ws_row;
	*columns = ws.ws_col;
	return 0;
}

static void setEnds(grid_t* g, cell_t* start, cell_t* end) {
	if( g->start )
		g->start->attributes &= ~caStart;
	g->start = start;
	g->start->attributes |= caStart;
	if( g->end )
		g->end->attributes &= ~caEnd;
	g->end = end;
	g->end->attributes |= caEnd;
}
static cell_t* getcell(const grid_t* g, int row, int column) {
	if( ! inrange(row, 0, g->rows-1) )
		return 0;
	if( ! inrange(column, 0, g->columns-1) )
		return 0;
	return &g->g[row*g->columns+column];
}
static grid_t* create_grid(int rows, int columns) {
	int r, c;
	ASSERT(offsetof(cell_t, a)==0);
	grid_t* g = (grid_t*)malloc(sizeof(grid_t));
	g->rows = rows;
	g->columns = columns;
	g->start = g->end = 0;
	g->current = 0;
	g->g = (cell_t*)malloc(sizeof(cell_t) * rows * columns);
	for(r=0; r < rows; r++)
		for(c=0; c < columns; c++) {
			cell_t* cell = getcell(g, r, c);
			cell->row = r;
			cell->column = c;
			cell->attributes = 0;
		}
	setEnds(g, getcell(g, 0, 0), getcell(g, rows-1, columns-1));
	return g;
}
static void free_grid(grid_t* g) {
	free(g->g);
	free(g);
}
static void set_random_barriers(grid_t* g, int ratio) {
	int i, nBarriers;
	ratio = minmax(ratio, 0, 90);
	nBarriers = (int)floor(g->rows * g->columns * ratio / 100.0);
	for(i=0; i < nBarriers; i++) {
		for(;;) {
			cell_t* cell = getcell(g, arc4random() % g->rows, arc4random() % g->columns);
			if( cell == g->start || cell == g->end )
				continue;
			if( cell->attributes & caBarrier )
				continue;
			cell->attributes |= caBarrier;
			break;
		}
	}
}
static inline cell_t* get_valid_neighbor(const grid_t* g, int row, int column) {
	cell_t* cell = getcell(g, row, column);
	if( ! cell || (cell->attributes & caBarrier) )
		return 0;
	return cell;
}
static a_star_edge_t* create_edge(cell_t* from, cell_t* to) {
	a_star_edge_t* edge = (a_star_edge_t*)malloc(sizeof(a_star_edge_t));
	edge->from = (a_star_node_t*)from;
	edge->to = (a_star_node_t*)to;
	return edge;
}
static int _cbCollectEdges(int index, void* data, void* cookie) {
	a_star_edge_t* edge = (a_star_edge_t*)data;
	a_star_edge_t* edges = (a_star_edge_t*)cookie;
	edges[index] = *edge;
	return 0;
}
static void graph_from_grid(const grid_t* g, a_star_graph_t** graph) {
	dlist_t *nodes, *edges;
	int r, c;
	int cutCorners = (parameters.options & oCutCorners) != 0;

	dlist_init(0, 0, 0, &nodes);
	dlist_init(free, 0, 0, &edges);

	for(r=0; r < g->rows; r++)
		for(c=0; c < g->columns; c++) {
			cell_t *cell = getcell(g, r, c), *neighbor;
			if( cell->attributes & caBarrier )
				continue;
			dlist_push_back(nodes, cell);
			if( cutCorners && (neighbor=get_valid_neighbor(g, cell->row-1, cell->column-1)) )
				dlist_push_back(edges, create_edge(cell, neighbor));
			if( (neighbor=get_valid_neighbor(g, cell->row-1, cell->column)) )
				dlist_push_back(edges, create_edge(cell, neighbor));
			if( cutCorners && (neighbor=get_valid_neighbor(g, cell->row-1, cell->column+1)) )
				dlist_push_back(edges, create_edge(cell, neighbor));
			if( (neighbor=get_valid_neighbor(g, cell->row, cell->column+1)) )
				dlist_push_back(edges, create_edge(cell, neighbor));
			if( cutCorners && (neighbor=get_valid_neighbor(g, cell->row+1, cell->column+1)) )
				dlist_push_back(edges, create_edge(cell, neighbor));
			if( (neighbor=get_valid_neighbor(g, cell->row+1, cell->column)) )
				dlist_push_back(edges, create_edge(cell, neighbor));
			if( cutCorners && (neighbor=get_valid_neighbor(g, cell->row+1, cell->column-1)) )
				dlist_push_back(edges, create_edge(cell, neighbor));
			if( (neighbor=get_valid_neighbor(g, cell->row, cell->column-1)) )
				dlist_push_back(edges, create_edge(cell, neighbor));
		}

	*graph = (a_star_graph_t*)malloc(sizeof(a_star_graph_t));
	// node pointers array
	dlist_detach(nodes, &(*graph)->nnodes, (void***)&(*graph)->nodes);
	// edges array
	(*graph)->nedges = dlist_len(edges);
	(*graph)->edges = (a_star_edge_t*)malloc(sizeof(a_star_edge_t) * (*graph)->nedges);
	dlist_enum_ex(edges, _cbCollectEdges, (*graph)->edges);
	// end points
	(*graph)->begin = (a_star_node_t*)g->start;
	(*graph)->end = (a_star_node_t*)g->end;

	dlist_deinit(nodes);
	dlist_deinit(edges);
}
static void free_graph(a_star_graph_t* graph) {
	free(graph->nodes);
	free(graph->edges);
	free(graph);
}
static long distance(const a_star_node_t* n1, const a_star_node_t* n2, void* cookie) {
	const cell_t* c1 = (const cell_t*)n1;
	const cell_t* c2 = (const cell_t*)n2;
	long dRow = c2->row - c1->row;
	long dCol = c2->column - c1->column;
	return floor(10 * pow(dRow * dRow + dCol * dCol, .5));
}
static void draw(const grid_t* g, const a_star_progress_info_t* progress) {
	int r, c;
	if( parameters.options & oAnimate ) {
		printf("\e[1;1H");
		printf("\e[?25l"); // hide cursor
		if( progress ) {
			int completion = 100 - progress->currDistance * 100 / progress->maxDistance;
			int i, w = completion * (g->columns*3) / 100 - 7;
			printf("\e[0K");
			for(i=0; i < w; i++)
				printf(">");
			printf(" %d%%\n", completion);
		}
		else {
			printf("\e[2;1H");
		}
	}
	for(r=0; r < g->rows; r++) {
		for(c=0; c < g->columns; c++) {
			const cell_t* cell = getcell(g, r, c);
			if( cell->attributes & caStart )
				printf("\e[107;30mS");
			else if( cell->attributes & caEnd )
				printf("\e[107;30mE");
			else if( cell->attributes & caBarrier )
				printf("\e[31mx");
			else if( cell->attributes & caPathStep )
				printf("\e[5;96mo");
			else if( cell->attributes & caCurrent )
				printf("\e[97mA");
			else if( cell->attributes & caAnalized )
				printf("\e[34m?");
			else
				printf("\e[90m.");
			printf("\e[0m  ");
		}
		printf("\n");
	}
	if( parameters.options & oAnimate ) {
		printf("\e[?25h"); // show cursor
	}
}
static void drawPlain(const grid_t* g) {
	int r, c;
	for(r=0; r < g->rows; r++) {
		for(c=0; c < g->columns; c++) {
			const cell_t* cell = getcell(g, r, c);
			if( cell->attributes & caStart )
				printf("S");
			else if( cell->attributes & caEnd )
				printf("E");
			else if( cell->attributes & caBarrier )
				printf("x");
			else if( cell->attributes & caPathStep )
				printf("o");
			else if( cell->attributes & caCurrent )
				printf("A");
			else if( cell->attributes & caAnalized )
				printf("?");
			else
				printf(".");
			printf("  ");
		}
		printf("\n");
	}
}
static void apply_path(grid_t* g, int n, cell_t** steps) {
	int i;
	for(i=0; i < n; i++)
		steps[i]->attributes |= caPathStep;
}
static void progress(const a_star_progress_info_t* pi, void* cookie) {
	cell_t* cAnalized = (cell_t*)pi->analized;
	if( cAnalized )
		cAnalized->attributes |= caAnalized;
	if( cookie ) {
		grid_t* grid = (grid_t*)cookie;
		if( grid->current )
			grid->current->attributes &= ~caCurrent;
		grid->current = (cell_t*)pi->current;
		grid->current->attributes |= caCurrent;
		draw(grid, pi);
		usleep(1000*10);
	}
}

typedef struct _barrier_t {
	int fromRow, fromCol;
	int toRow, toCol;
} barrier_t;
static void applyBarrierSpecs(grid_t* g, dlist_t* barriers) {
	for(;;) {
		barrier_t* b = (barrier_t*)dlist_pop_front(barriers);
		if( ! b )
			break;
		if( b->toRow < 0 || b->toCol < 0 ) {
			// single dot
			cell_t* c = getcell(g, b->fromRow, b->fromCol);
			if( c && (c->attributes & (caStart | caEnd))==0 )
				c->attributes |= caBarrier;
		} else if( b->fromRow == b->toRow ) {
			// horizontal line
			int i;
			for(i=b->fromCol; i <= b->toCol; i++) {
				cell_t* c = getcell(g, b->fromRow, i);
				if( c && (c->attributes & (caStart | caEnd))==0 )
					c->attributes |= caBarrier;
			}
		} else if( b->fromCol == b->toCol ) {
			// vertical line
			int i;
			for(i=b->fromRow; i <= b->toRow; i++) {
				cell_t* c = getcell(g, i, b->fromCol);
				if( c && (c->attributes & (caStart | caEnd))==0 )
					c->attributes |= caBarrier;
			}
		}
		free(b);
	}
}

static const char __help[] =
"a-star - an A* Path Finding Algorithm Visualizer\n"
"-------------------------------------------------------\n"
"Usage:\n"
"	a-start {r|c|b|s|e|l|a|d|h}\n"
"Options:\n"
"	-r <rows>\n"
"		#of rows\n"
"	-c <columns>\n"
"		#of columns\n"
"	-b <barriers ratio>\n"
"		#Ratio of random barriers in procentage (0..100)\n"
"	-s <row>:<col>\n"
"		Path start point (0-based)\n"
"	-e <row:col>\n"
"		Path end point (0-based)\n"
"	-l <row>:<col>{..<row>:<col>}\n"
"		Barrier point/line. Diagonal lines are not allowed.\n"
"	-a\n"
"		Animate\n"
"	-d\n"
"		Allot cutting corners\n"
"	-h\n"
"		Show this help information\n"
;

int main(int argc, char** argv) {
	grid_t* grid;
	a_star_graph_t* graph;
	cell_t** path=0;
	dlist_t* l_barriers;
	int n, opt;

	parameters.rows=20;
	parameters.columns=20;
	parameters.barriers=30;
	parameters.startRow=parameters.startCol=parameters.endRow=parameters.endCol=-1;

	dlist_init(free, 0, 0, &l_barriers);

	if( getTerminalSize(&parameters.rows, &parameters.columns) == 0 ) {
		parameters.rows = (parameters.rows - 3) * 90 / 100; // 90% of the current terminal width
		parameters.columns = (parameters.columns / 3) * 90 / 100; // 90% of the current terminal height
	}

	while( (opt=getopt(argc, argv, "r:c:b:s:e:l:adh")) != -1 ) {
		switch( opt ) {
			case 'r': {
				parameters.rows=max(atoi(optarg), 4);
				break;
			}
			case 'c': {
				parameters.columns=max(atoi(optarg), 4);
				break;
			}
			case 'b': {
				parameters.barriers=minmax(atoi(optarg), 0, 100);
				break;
			}
			case 's': {
				int c = sscanf(optarg, "%d:%d", &parameters.startRow, &parameters.startCol);
				if( c > 0 ) parameters.startRow = minmax(parameters.startRow, 0, parameters.rows-1);
				if( c > 1 ) parameters.startCol = minmax(parameters.startCol, 0, parameters.columns-1);
				break;
			}
			case 'e': {
				int c = sscanf(optarg, "%d:%d", &parameters.endRow, &parameters.endCol);
				if( c > 0 ) parameters.endRow = minmax(parameters.endRow, 0, parameters.rows-1);
				if( c > 1 ) parameters.endCol = minmax(parameters.endCol, 0, parameters.columns-1);
				break;
			}
			case 'a': {
				if( isatty(fileno(stdout)) )
					parameters.options |= oAnimate;
				break;
			}
			case 'd': {
				parameters.options |= oCutCorners;
				break;
			}
			case 'l': {
				barrier_t* b = (barrier_t*)malloc(sizeof(barrier_t));
				b->fromRow=b->fromCol=b->toRow=b->toCol=-1;
				int n = sscanf(optarg, "%u:%u..%u:%u",
							&b->fromRow, &b->fromCol, &b->toRow, &b->toCol);
				if( n == 2 ) {
					dlist_push_back(l_barriers, b);
				} else if( n == 4 ) {
					if( b->fromRow != b->toRow && b->fromCol != b->toCol ) {
						free(b);
						fprintf(stderr, "Diagonal barriers are not supported!\n");
						exit(99);
					}
					dlist_push_back(l_barriers, b);
				} else {
					free(b);
					fprintf(stderr, "Invalid barrier specification!\n");
					exit(99);
				}
				break;
			}
			case 'h': {
				printf("%s\n", __help);
				exit(0);
				break;
			}
		}
	}

	if( parameters.startRow < 0)
		parameters.startRow = 0;
	if( parameters.startCol < 0 )
		parameters.startCol = 0;
	if( parameters.endRow < 0 )
		parameters.endRow = parameters.rows - 1;
	if( parameters.endCol < 0 )
		parameters.endCol = parameters.columns - 1;

	grid = create_grid(parameters.rows, parameters.columns);
	setEnds(grid,
		getcell(grid, parameters.startRow, parameters.startCol),
		getcell(grid, parameters.endRow, parameters.endCol));

	if( dlist_len(l_barriers) )
		applyBarrierSpecs(grid, l_barriers);
	else
		set_random_barriers(grid, parameters.barriers);

	graph_from_grid(grid, &graph);

	// clear screen
	if( parameters.options & oAnimate )
		printf("\e[1;1H\e[2J"); // clear screen

	n=a_star_shortest_path(graph, distance, distance, progress,
			(parameters.options & oAnimate) ? grid : 0, (a_star_node_t***)&path);
	if( n > 0 ) {
		apply_path(grid, n, path);
	}

	if( grid->current ) {
		grid->current->attributes &= ~caCurrent;
		grid->current = 0;
	}

	if( parameters.options & oAnimate )
		draw(grid, 0);
	else
		drawPlain(grid);

	dlist_deinit(l_barriers);
	free(path);
	free_graph(graph);
	free_grid(grid);

	return n == 0;
}

