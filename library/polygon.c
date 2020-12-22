#include "polygon.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

const double AREA_SCALE = 0.5;
const double CENTROID_SCALE = 1.0 / 6.0;

// use Shoelace formula to calculate the area of the polygon
double polygon_area(list_t *polygon) {
    size_t size = list_size(polygon);
    double area = 0;

    vector_t curr;
    vector_t next;

    // sum the cross products of each vector with the next vector in the list
    for (unsigned long i = 0; i < size; i++) {
        curr = ((vector_t *)list_get(polygon, i % size))[0];
        next = ((vector_t *)list_get(polygon, (i + 1) % size))[0];
        area = area + vec_cross(curr, next);
    }

    // take the absolute value of the area
    if (area < 0) {
        area = area * -1;
    }

    return (AREA_SCALE * area);
}

vector_t polygon_centroid(list_t *polygon) {
    double area = polygon_area(polygon);
    size_t size = list_size(polygon);
    vector_t centroid = VEC_ZERO;

    vector_t curr;
    vector_t next;

    // sum and cross product each vector with the next one and multiply the values
    for (unsigned long i = 0; i < size; i++) {
        curr = ((vector_t *)list_get(polygon, i % size))[0];
        next = ((vector_t *)list_get(polygon, (i + 1) % size))[0];
        centroid =
            vec_add(centroid, vec_multiply(vec_cross(curr, next), vec_add(curr, next)));
    }

    // multiply the sum by 1/(6A) (A = area of polygon)
    centroid = vec_multiply(CENTROID_SCALE / area, centroid);
    return centroid;
}

void polygon_translate(list_t *polygon, vector_t translation) {
    size_t size = list_size(polygon);
    vector_t *curr;

    // set each vector equal to the sum of it and the translation
    for (unsigned long k = 0; k < size; k++) {
        curr = (vector_t *)list_get(polygon, k);
        curr[0] = vec_add(curr[0], translation);
    }
}

void polygon_rotate(list_t *polygon, double angle, vector_t point) {
    size_t size = list_size(polygon);
    vector_t *curr;

    // for each vector subtract point from it, rotate it around the origin by
    // angle radians, then add point to it
    for (unsigned long k = 0; k < size; k++) {
        curr = (vector_t *)list_get(polygon, k);
        curr[0] = vec_add(vec_rotate(vec_subtract(curr[0], point), angle), point);
    }
}
