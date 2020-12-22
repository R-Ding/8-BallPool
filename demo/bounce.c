#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "sdl_wrapper.h"
#include "star.h"

const double ANG_VELOCITY = 1;
const int MIN_X = 0;
const int MIN_Y = 0;
const int MAX_X = 1000;
const int MAX_Y = 500;
const int VERTICES = 5;
const double X_VEL = 200.0;
const double Y_VEL = 200.0;
const int RADIUS = 50;

void compute_new_positions(star_t *star, double time) {
    // rotate the polygon proportionally to the time
    polygon_rotate(star->points, time * ANG_VELOCITY, polygon_centroid(star->points));

    // translate the polygon proportionally to the time
    polygon_translate(star->points, vec_multiply(time, star->vel[0]));

    int x_min = MAX_X;
    int y_min = MAX_Y;
    int x_max = MIN_X;
    int y_max = MIN_Y;

    // find the maximum and minimum x and y values of outer points of the polygon
    for (size_t i = 0; i < list_size(star->points); i = i + 2) {
        vector_t curr = ((vector_t *)list_get(star->points, i))[0];
        if (curr.x < x_min) {
            x_min = (int)curr.x;
        }
        if (curr.y < y_min) {
            y_min = (int)curr.y;
        }
        if (curr.x > x_max) {
            x_max = (int)curr.x;
        }
        if (curr.y > y_max) {
            y_max = (int)curr.y;
        }
    }

    // if any max/min points pass a window edge, invert the corresponding velocity
    // and move the polygon into the window
    if (x_min <= MIN_X) {
        polygon_translate(star->points, (vector_t){2 * (MIN_X - x_min), 0});
        star->vel[0] = (vector_t){-1 * star->vel[0].x, star->vel[0].y};
    } else if (x_max >= MAX_X) {
        polygon_translate(star->points, (vector_t){2 * (MAX_X - x_max), 0});
        star->vel[0] = (vector_t){-1 * star->vel[0].x, star->vel[0].y};
    }
    if (y_min <= MIN_Y) {
        polygon_translate(star->points, (vector_t){0, 2 * (MIN_Y - y_min)});
        star->vel[0] = (vector_t){star->vel[0].x, -1 * star->vel[0].y};
    } else if (y_max >= MAX_Y) {
        polygon_translate(star->points, (vector_t){0, 2 * (MAX_Y - y_max)});
        star->vel[0] = (vector_t){star->vel[0].x, -1 * star->vel[0].y};
    }
}

int main() {
    vector_t min = (vector_t){MIN_X, MIN_Y};
    vector_t max = (vector_t){MAX_X, MAX_Y};
    sdl_init(min, max);

    // create a 5 pointed star and set its initial velocity vector and radius
    star_t *star = star_init(VERTICES, RADIUS, X_VEL, Y_VEL);

    double dt = 0.0;

    while (!sdl_is_done(NULL)) {
        dt = time_since_last_tick();
        compute_new_positions(star, dt);

        sdl_clear();
        sdl_draw_polygon(star->points, star->col);
        sdl_show();
    }

    star_free(star);
    return 0;
}
