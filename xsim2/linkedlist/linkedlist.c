//
// Created by Alex Brodsky on 2023-09-20.
//

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "linkedlist.h"

typedef struct ll_node {
    struct ll_node *prev;
    struct ll_node *next;
    void *item;
} ll_node_t;

typedef struct linked_list {
    ll_node_t *head;
    ll_node_t *tail;
} linked_list;

extern linked_list_t ll_new() {
    linked_list_t * list = calloc(1,sizeof(linked_list_t));
    return list;
}

extern void ll_destroy(linked_list_t ll) {
    linked_list  *list = (linked_list *)ll;
    assert(list != NULL);

    while (list->head != NULL) {
        ll_node_t *tmp = list->head;
        list->head = list->head->next;
        free(tmp);
    }

    free(list);
}

static ll_node_t *new_node(void *item, ll_node_t *prev, ll_node_t *next) {
    ll_node_t *n = calloc(1, sizeof(ll_node_t));
    if (n != NULL) {
        n->item = item;
        n->prev = prev;
        n->next = next;
    }
    return n;
}


extern int ll_add_front(linked_list_t ll, void *item) {
    linked_list  *list = (linked_list *)ll;
    assert(list);

    ll_node_t *n = new_node(item,NULL, list->head);
    if (n != NULL) {
        if (list->tail == NULL) {
            list->tail = n;
        } else {
            list->head->prev = n;
        }
        list->head = n;
    }
    return n != NULL;
}

extern int ll_add_back(linked_list_t ll, void *item) {
    linked_list  *list = (linked_list *)ll;
    assert(list);

    ll_node_t *n = new_node(item,list->tail, NULL);
    if (n != NULL) {
        if (list->head == NULL) {
            list->head = n;
        } else {
            list->tail->next = n;
        }
        list->tail = n;
    }
    return n != NULL;
}

extern void * ll_remove_front(linked_list_t ll) {
    linked_list  *list = (linked_list *)ll;
    assert(list);

    void *item = NULL;
    if (list->head != NULL) {
        ll_node_t *n = list->head;
        list->head = list->head->next;
        if (list->head == NULL) {
            list->tail = NULL;
        } else {
            list->head->prev = NULL;
        }

        item = n->item;
        free(n);
    }
    return item;
}

extern void * ll_remove_back(linked_list_t ll) {
    linked_list  *list = (linked_list *)ll;
    assert(list);

    void *item = NULL;
    if (list->tail != NULL) {
        ll_node_t *n = list->tail;
        list->tail = list->tail->prev;
        if (list->tail == NULL) {
            list->head = NULL;
        } else {
            list->tail->next = NULL;
        }

        item = n->item;
        free(n);
    }
    return item;
}

extern void * ll_get_front(linked_list_t ll) {
    linked_list  *list = (linked_list *)ll;
    assert(list);

    void *item = NULL;
    if (list->head != NULL) {
        item = list->head->item;
    }
    return item;
}

extern void * ll_get_back(linked_list_t ll) {
    linked_list  *list = (linked_list *)ll;
    assert(list);

    void *item = NULL;
    if (list->head != NULL) {
        item = list->tail->item;
    }
    return item;
}

extern int ll_size(linked_list_t ll) {
    linked_list  *list = (linked_list *)ll;
    assert(list);
    int count = 0;
    for (ll_node_t *ptr = list->head; ptr != NULL; ptr = ptr->next) {
        count++;
    }
    return count;
}

extern int ll_is_empty(linked_list_t ll) {
    linked_list  *list = (linked_list *)ll;
    assert(list);
    return list->head == NULL;
}

/************************************************************************
 * Your code below
 ************************************************************************/

typedef struct ll_iterator {
    ll_node_t *prev;
    linked_list *list;
    char may_set;
} ll_iterator;

extern ll_iterator_t  ll_iterator_new(linked_list_t list) {
    linked_list *lst = (linked_list*) list;
    ll_iterator *iter = malloc(sizeof(ll_iterator));
    iter->prev = NULL;
    iter->list = lst;
    iter->may_set = 0;
    return iter;
}

extern void ll_iterator_destroy(ll_iterator_t iter) {
    free(iter);
}

extern void *ll_next(ll_iterator_t iter_r) {
    ll_iterator *iter = (ll_iterator*) iter_r;

    void *tmp;
    if (iter->prev != NULL) {
        tmp = iter->prev->next->item;
        ll_node_t *next =  iter->prev->next;
        iter->prev = next;
    } else {
        // start of list
        tmp = iter->list->head->item;
        iter->prev = iter->list->head;
    }
    iter->may_set = 1;
    return tmp;
}

extern int ll_has_next(ll_iterator_t iter_r) {
    ll_iterator *iter = (ll_iterator*) iter_r;
    return (iter->prev == NULL && iter->list->head != NULL)
           || (iter->prev != NULL && iter->prev->next != NULL);
}

extern int ll_add(ll_iterator_t iter_r, void *item) {
    ll_iterator *iter = (ll_iterator*) iter_r;
    ll_node_t *to_add = new_node(item, iter->prev, iter->prev->next);

    if (to_add == NULL) return 0; // malloc fail - you have a lot of problems

    if (iter->prev == NULL) {
        // adding to front of list
        to_add->next = iter->list->head;
        iter->list->head = to_add;
    } else {
        // adding in the middle
        if (iter->prev->next != NULL) {
            iter->prev->next->prev = to_add;
        }
        iter->prev->next = to_add;
        iter->prev = to_add;
    }

    iter->may_set = 0;

    return 1;
}

extern void *ll_set(ll_iterator_t iter_r, void *item) {
    ll_iterator *iter = (ll_iterator*) iter_r;
    if (!iter->may_set) {
        // haven't called next since the last remove or add
        return NULL;
    }

    if (iter->prev == NULL) {
        // this shouldn't happen
        return NULL;
    }

    void *tmp = iter->prev->item;
    iter->prev->item = item;
    return tmp;
}

extern void *ll_remove(ll_iterator_t iter_r){
    ll_iterator *iter = (ll_iterator*) iter_r;
    if (!iter->may_set) {
        // haven't called next() since the last modifying operation
        return NULL;
    }

    if (iter->prev == NULL) {
        // should never happen
        return NULL;
    }

    ll_node_t *to_remove = iter->prev;

    // cross-link
    if (to_remove->prev != NULL) {
        to_remove->prev->next = to_remove->next;
    }
    if (to_remove->next != NULL) {
        to_remove->next->prev = to_remove->prev;
    }

    // cleanup head/tail
    if (iter->list->tail == to_remove) {
        iter->list->tail = to_remove->prev;
    }
    if (iter->list->head == to_remove) {
        iter->list->head = to_remove->next;
    }

    // cleanup iterator state
    iter->prev = to_remove->prev;

    void *tmp = to_remove->item;
    free(to_remove);
    return tmp;
}
