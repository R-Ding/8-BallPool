#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "vec_list.h"

typedef struct vec_list {
    vector_t **vectors;
    size_t size;
    size_t capacity;
} vec_list_t;

vec_list_t *vec_list_init(size_t initial_size) {
    //Allocate memory for list
    vec_list_t *list = malloc(sizeof(vec_list_t));
    assert(list != NULL);
    //Allocate memory for array inside list
    list->vectors = malloc(initial_size * sizeof(vector_t *));
    assert(list->vectors != NULL);
    //Initialize list size and capacity
    list->size = 0;
    list->capacity = initial_size;
    return list;
}

void vec_list_free(vec_list_t *list) {
    //free individual vectors, then array, then struct
    for(unsigned long i = 0; i < list->size; i++) {
        free(list->vectors[i]);
    }
    free(list->vectors);
    free(list);
}

size_t vec_list_size(vec_list_t *list) {
    return list->size;
}

vector_t *vec_list_get(vec_list_t *list, size_t index) {
    assert(index < list->size);
    return list->vectors[index];
}

void vec_list_add(vec_list_t *list, vector_t *value) {
    assert(list->size < list->capacity);
    assert(value != NULL);
    list->vectors[list->size] = value;
    list->size++;
}

vector_t *vec_list_remove(vec_list_t *list) {
    assert(list->size > 0);
    //Stores vector outside of list before nullifying list index
    vector_t *vec = list->vectors[list->size - 1];
    list->vectors[list->size - 1] = NULL;
    list->size--;
    return vec;
}
