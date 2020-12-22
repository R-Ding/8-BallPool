#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "list.h"

typedef struct list {
    void **objects;
    size_t size;
    size_t cap;
    free_func_t freer;
} list_t;

list_t *list_init(size_t initial_size, free_func_t freer) {
    //allocate memory for list
    list_t *list = malloc(sizeof(list_t));
    assert(list != NULL);
    //allocate memory for array inside list
    list->objects = malloc(initial_size * sizeof(void *));
    assert(list->objects != NULL);
    //initialize list size and capacity
    list->size = 0;
    list->cap = initial_size;
    list->freer = freer;
    return list;
}

void list_free(list_t *list) {
    //free individual objects, then array, then struct
    if (list->freer != NULL) {
        for (size_t i = 0; i < list->size; i++) {
            (list->freer)(list->objects[i]);
        }
    }
    free(list->objects);
    free(list);

}

size_t list_size(list_t *list) {
    return list->size;
}

void *list_get(list_t *list, size_t index) {
    assert(index < list->size);
    return list->objects[index];
}

void list_add(list_t *list, void *value) {
    assert(value != NULL);
    //if list is full, double the capacity using realloc()
    if(list->size >= list->cap) {
        
        void **tmp = realloc(list->objects, (2 * list->cap + 1) * sizeof(void *));
        assert(tmp != NULL);
        list->objects = tmp;
        list->cap = list->cap * 2 + 1;
    }
    list->objects[list->size] = value;
    list->size++;
}

void *list_remove(list_t *list, size_t index) {
    assert(index < list->size);
    //Stores object outside of list before nullifying list index
    void *obj = list->objects[index];
    //transfer every object after index one to the left
    for (size_t i = index; i < list->size - 1; i++) {
        list->objects[i] = list->objects[i + 1];
    }
    //nullify last index?
    list->size--;
    return obj;
}
