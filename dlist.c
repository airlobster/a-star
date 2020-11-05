// dlist.c

#include "dlist.h"

typedef struct _dlist_node_t {
	struct _dlist_node_t *next, *prev;
	void* data;
} dlist_node_t;

typedef struct _dlist_context_t {
	dlist_node_t *head, *tail;
	dlist_predicate_t pred;
	dlist_dtor dtor;
	void* context;
	size_t len;
} dlist_context_t;

void dlist_init(dlist_dtor dtor, dlist_predicate_t pred, void* context, dlist_t** l) {
	dlist_context_t* x = (dlist_context_t*)malloc(sizeof(dlist_context_t));
	x->dtor = dtor;
	x->pred = pred;
	x->context = context;
	x->head = x->tail = 0;
	x->len = 0;
	*l = x;
}

void dlist_deinit(dlist_t* l) {
	dlist_reset(l);
	free(l);
}

void dlist_reset(dlist_t* l) {
	dlist_context_t* x = (dlist_context_t*)l;
	dlist_node_t* e = x->head;
	while( e ) {
		dlist_node_t* next = e->next;
		if( e->data && x->dtor )
			(*x->dtor)(e->data);
		free(e);
		e = next;
	}
	x->head = x->tail = 0;
	x->len = 0;
}

size_t dlist_len(dlist_t* l) {
	dlist_context_t* x = (dlist_context_t*)l;
	return x->len;
}

void dlist_push_back(dlist_t* l, void* e) {
	dlist_context_t* x = (dlist_context_t*)l;
	if( x->pred ) {
		dlist_push_ordered(l, e);
		return;
	}
	dlist_node_t* n = (dlist_node_t*)malloc(sizeof(dlist_node_t));
	n->next = 0;
	n->prev = x->tail;
	n->data = e;
	if( x->tail )
		x->tail->next = n;
	x->tail = n;
	if( ! x->head )
		x->head = n;
	x->len++;
}

void dlist_push_front(dlist_t* l, void* e) {
	dlist_context_t* x = (dlist_context_t*)l;
	if( x->pred ) {
		dlist_push_ordered(l, e);
		return;
	}
	dlist_node_t* n = (dlist_node_t*)malloc(sizeof(dlist_node_t));
	n->data = e;
	n->prev = 0;
	n->next = x->head;
	if( x->head )
		x->head->prev = n;
	x->head = n;
	if( ! x->tail )
		x->tail = n;
	++x->len;
}

void dlist_push_ordered(dlist_t* l, void* e) {
	dlist_context_t* x = (dlist_context_t*)l;
	if( ! x->pred ) {
		dlist_push_back(l, e);
		return;
	}
	dlist_node_t* c;
	dlist_node_t* n = (dlist_node_t*)malloc(sizeof(dlist_node_t));
	n->data = e;
	n->prev = n->next = 0;
	c = x->head;
	while( c ) {
		if( x->pred(e, c->data, x->context) < 0 )
			break;
		c = c->next;
	}
	n->next = c;
	if( c ) {
		if( c->prev )
			c->prev->next = n;
		c->prev = n;
	} else
		x->tail = n;
	if( x->head == c )
		x->head = n;
	++x->len;
}


int dlist_enum(dlist_t* l, int(*cb)(void* data, void* cookie), void* cookie) {
	dlist_context_t* x = (dlist_context_t*)l;
	int rc = 0;
	dlist_node_t* e = x->head;
	while( e && ! rc ) {
		dlist_node_t* next = e->next;
		rc = cb(e->data, cookie);
		e = next;
	}
	return rc;
}

int dlist_enum_ex(dlist_t* l, int(*cb)(int index, void* data, void* cookie), void* cookie) {
	dlist_context_t* x = (dlist_context_t*)l;
	int rc = 0, i=0;
	dlist_node_t* e = x->head;
	while( e && ! rc ) {
		dlist_node_t* next = e->next;
		rc = cb(i++, e->data, cookie);
		e = next;
	}
	return rc;
}

void dlist_detach(dlist_t* l, size_t* n, void*** a) {
	int i=0;
	dlist_context_t* x = (dlist_context_t*)l;
	size_t _n = dlist_len(l);
	void** _a = (void**)malloc(_n * sizeof(void*));
	dlist_node_t* e = x->head;
	while( e ) {
		_a[i++] = e->data;
		e->data = 0; // detach from data
		e = e->next;
	}
	dlist_reset(l);
	*n = _n;
	*a = _a;
}

void dlist_remove(dlist_t* l, void* elem) {
	dlist_context_t* x = (dlist_context_t*)l;
	dlist_node_t* e = x->head;
	while( e ) {
		if( e->data == elem ) {
			if( e->prev )
				e->prev->next = e->next;
			else
				x->head = e->next;
			if( e->next )
				e->next->prev = e->prev;
			else
				x->tail = e->prev;
			if( e->data && x->dtor )
				(*x->dtor)(e->data);
			free(e);
			x->len--;
			break;
		}
		e = e->next;
	}
}

int dlist_exists(dlist_t* l, void* elem) {
	dlist_context_t* x = (dlist_context_t*)l;
	dlist_node_t* e = x->head;
	while( e ) {
		if( e->data == elem )
			return 1;
		e = e->next;
	}
	return 0;
}

void dlist_toarray(dlist_t* l, size_t* n, void*** a) {
	int i=0;
	dlist_context_t* x = (dlist_context_t*)l;
	size_t _n = dlist_len(l);
	void** _a = (void**)malloc((_n+1) * sizeof(void*));
	dlist_node_t* e = x->head;
	while( e ) {
		_a[i++] = e->data;
		e = e->next;
	}
	_a[i] = 0;
	*n = _n;
	*a = _a;
}

void* dlist_get_front(dlist_t* l) {
	dlist_context_t* x = (dlist_context_t*)l;
	return x->head ? x->head->data : 0;
}

void* dlist_pop_front(dlist_t* l) {
	dlist_context_t* x = (dlist_context_t*)l;
	if( ! x->head )
		return 0;
	dlist_node_t* e = x->head;
	void* data = e->data;
	if( e->next )
		e->next->prev = 0;
	else
		x->tail = 0;
	x->head = e->next;
	free(e);
	x->len--;
	return data;
}

