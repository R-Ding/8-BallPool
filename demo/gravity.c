#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "sdl_wrapper.h"
#include "star.h"

const int MIN_X = 0;
const int MIN_Y = 0;
const int MAX_Y = 500;
const int MAX_X = 2 * MAX_Y;
const double ANG_VELOCITY = 3;
const double VEL_X = 250;
const double ACCEL_Y = -750;
const int STAR_RADIUS = 50;
const double SPAWN_TIME = 0.5;
const int NUM_STARS = 8;
const double ELASTICITY_LB = 0.15;
const double ELASTICITY_UB = 0.75;

// returns 1 if the star has gone off-screen, 0 otherwise
int update_pos(star_t *st, double dt) {
    // update velocity as according to gravity
    st->vel->y += dt * ACCEL_Y;

    // translate the polygon proportionally to the time
    polygon_translate(st->points, vec_multiply(dt, st->vel[0]));

    double x_min = MAX_X;
    double y_min = MAX_Y;

    // find the maximum and minimum x and y values of outer points of the polygon
    for (size_t i = 0; i < list_size(st->points); i++) {
        vector_t curr = ((vector_t *)list_get(st->points, i))[0];
        if (curr.x < x_min) {
            x_min = curr.x;
        }
        if (curr.y < y_min) {
            y_min = curr.y;
        }
    }

    // check for star leaving bounds of the screen
    if (x_min >= MAX_X) {
        return 1;
    }
    // if star touches ground adjust velocity to simulate bounce
    else if (y_min <= MIN_Y) {
        polygon_translate(st->points, vec_multiply(-1 * dt, st->vel[0]));
        // compute random elasticity (0.75 to 0.9) and apply to new y-velocity
        st->vel->y *= -(ELASTICITY_LB * rand() / RAND_MAX + ELASTICITY_UB);
    }

    // rotate the star proportionally to the time
    polygon_rotate(st->points, (double)dt * ANG_VELOCITY, polygon_centroid(st->points));
    return 0;
}

void update_all(list_t *stars, double dt) {
    for (size_t i = 0; i < list_size(stars); i++) {
        star_t *star = (star_t *)list_get(stars, i);

        // update position of all stars, remove star if it passes edge of window
        if (star != NULL && update_pos(star, dt)) {
            star_free(list_remove(stars, i));
        }
    }
}

void draw_all_stars(list_t *stars) {
    for (size_t i = 0; i < list_size(stars); i++) {
        star_t *star = (star_t *)list_get(stars, i);
        sdl_draw_polygon(star->points, star->col);
    }
}

int main() {
    vector_t min = (vector_t){MIN_X, MIN_Y};
    vector_t max = (vector_t){MAX_X, MAX_Y};
    sdl_init(min, max);

    // create list of stars
    list_t *stars = list_init(NUM_STARS, (free_func_t)star_free);
    assert(stars != NULL);

    double t = 1.0;
    double dt = 0.0;
    int vertices = 2;

    while (!sdl_is_done(NULL)) {
        // compute time change
        dt = time_since_last_tick();
        t += dt;

        // add star if SPAWN_TIME has passed since last star was created
        if (t >= SPAWN_TIME) {
            star_t *star = star_init(vertices, (int)STAR_RADIUS, VEL_X, 0.0);
            polygon_translate(star->points,
                              (vector_t){-1 * STAR_RADIUS, MAX_Y - 3 * STAR_RADIUS});
            list_add(stars, star);
            vertices += 1;
            t = 0.0;
        }

        update_all(stars, dt);

        // update sdl board
        sdl_clear();
        draw_all_stars(stars);
        sdl_show();
    }
    list_free(stars);
    return 0;
}
