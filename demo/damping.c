#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "forces.h"
#include "scene.h"
#include "sdl_wrapper.h"

const int MIN_Y = 0;
const int MIN_X = 0;
const int MAX_Y = 500;
const int MAX_X = 2 * MAX_Y;
const int NUM_CIRCLES = 50;
const int CIRCLE_RADIUS = MAX_Y / NUM_CIRCLES;
const int CIRCLE_VERTICES = 1000;
const int CIRCLE_MASS = 25;
const int ANCHOR_RADIUS = 1;
const int ANCHOR_VERTICES = 10;
const double ANCHOR_MASS = DBL_MAX;
const double K_DECAY = 1.1;
const double DRAG_GAMMA = 5.0;
const double SPRING_K = 1.0;
const double INIT_IMPULSE = 1000;

// generates random color to be used for circles
rgb_color_t random_color() {
    return (rgb_color_t){((float)rand()) / RAND_MAX, ((float)rand()) / RAND_MAX,
                         ((float)rand()) / RAND_MAX};
}

list_t *generate_circle_points(int vertices, int radius) {
    // asserts args are proper
    assert(vertices >= 2);
    assert(radius >= 0);
    // reference vector
    vector_t ref = {radius, 0.0};
    double angle = 2 * M_PI / vertices;
    list_t *points = list_init((size_t)vertices, (free_func_t)free);
    // creates n points in a circle using reference vector
    for (int i = 0; i < vertices; i++) {
        vector_t *curr = malloc(sizeof(vector_t));
        assert(curr != NULL);
        *curr = vec_rotate(ref, angle * i);
        list_add(points, curr);
    }
    return points;
}

body_t *generate_anchor() {
    list_t *points = generate_circle_points(ANCHOR_VERTICES, ANCHOR_RADIUS);
    return body_init(points, ANCHOR_MASS, (rgb_color_t){1, 1, 1});
}

body_t *generate_circle() {
    list_t *points = generate_circle_points(CIRCLE_VERTICES, CIRCLE_RADIUS);
    return body_init(points, CIRCLE_MASS, random_color());
}

void populate_scene(scene_t *scene) {
    for (size_t i = 0; i < NUM_CIRCLES; i++) {
        // create anchor
        body_t *anchor = generate_anchor();
        // create display circle
        body_t *circle = generate_circle();
        // positions circles such that they touch end to end from left/right
        vector_t pos = {CIRCLE_RADIUS + 2 * CIRCLE_RADIUS * ((int)i), (double)MAX_Y / 2};
        body_set_centroid(anchor, pos);
        body_set_centroid(circle, pos);
        scene_add_body(scene, anchor);
        scene_add_body(scene, circle);
        // create spring force between each circle and anchor
        create_spring(scene, SPRING_K * pow(K_DECAY, i), anchor, circle);
        create_drag(scene, DRAG_GAMMA, circle);
        // add initial impulse to each body
        vector_t impulse = {0, INIT_IMPULSE};
        body_add_impulse(circle, impulse);
    }
}

int main() {
    srand(time(0));
    // initialize sdl demo
    sdl_init((vector_t){MIN_X, MIN_Y}, (vector_t){MAX_X, MAX_Y});
    // initialize scene and components
    scene_t *scene = scene_init();
    assert(scene != NULL);
    populate_scene(scene);
    while (!sdl_is_done(scene)) {
        scene_tick(scene, time_since_last_tick());
        sdl_render_scene(scene);
    }

    scene_free(scene);
    return 0;
}
