// dlist.h

#ifndef _DLIST_H_
#define _DLIST_H_

#include <stdlib.h>

typedef void dlist_t;
typedef void(*dlist_dtor)(void*);
typedef int(*dlist_predicate_t)(const void* e1, const void* e2, void* context);

void dlist_init(dlist_dtor dtor, dlist_predicate_t pred, void* context, dlist_t** l);
void dlist_deinit(dlist_t* l);
void dlist_reset(dlist_t* l);
size_t dlist_len(dlist_t* l);
void dlist_push_back(dlist_t* l, void* e);
void dlist_push_front(dlist_t* l, void* e);
void dlist_push_ordered(dlist_t* l, void* e);
void* dlist_pop_front(dlist_t* l);
void dlist_remove(dlist_t* l, void* e);
int dlist_exists(dlist_t* l, void* e);
int dlist_enum(dlist_t* l, int(*cb)(void* data, void* cookie), void* cookie);
int dlist_enum_ex(dlist_t* l, int(*cb)(int index, void* data, void* cookie), void* cookie);
void dlist_detach(dlist_t* l, size_t* n, void*** a);
void dlist_toarray(dlist_t* l, size_t* n, void*** a);
void* dlist_get_front(dlist_t* l);

#endif
