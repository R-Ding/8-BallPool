#include "star.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

const double VERT = 0.5;

// returns a random color (used when initializing a star)
float rand_col() {
    return ((float)rand()) / RAND_MAX;
}

star_t *star_init(int vertices, int radius, double vel_x, double vel_y) {
    // verify parameters will create a valid star
    assert(vertices >= 2);
    assert(radius > 0);

    // calculate angle between vertices and initialize points list
    double angle = (M_PI) / ((double)vertices);
    vector_t ref_pt = (vector_t){0, radius};
    list_t *points = list_init((size_t)vertices * 2, (free_func_t)free);

    // create vector for each point in the star
    for (int i = 0; i < vertices * 2; i++) {
        vector_t *curr_pt = malloc(sizeof(vector_t));
        assert(curr_pt != NULL);
        // add even vertices as outer vertices and odd vertices as inner vertices
        if (i % 2 == 0) {
            curr_pt[0] = vec_rotate(ref_pt, angle * i);
        } else {
            curr_pt[0] = vec_multiply(VERT, vec_rotate(ref_pt, angle * i));
        }
        list_add(points, curr_pt);
    }

    // move the star so all its points have positive x and y values
    polygon_translate(points, (vector_t){radius * 2, radius * 2});

    // create and return star object
    star_t *star = malloc(sizeof(star_t));
    assert(star != NULL);
    star->points = points;
    star->col = (rgb_color_t){rand_col(), rand_col(), rand_col()};
    star->vel = malloc(sizeof(vector_t));
    assert(star->vel != NULL);
    star->vel[0] = (vector_t){vel_x, vel_y};

    return star;
}

void star_free(star_t *star) {
    list_free(star->points);
    free(star->vel);
    free(star);
}
