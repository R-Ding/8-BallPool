#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "forces.h"
#include "polygon.h"
#include "scene.h"
#include "sdl_wrapper.h"

const int MIN_Y = 0;
const int MIN_X = 0;
const int MAX_Y = 500;
const int MAX_X = 2 * MAX_Y;
const int N_BODIES = 100;
const int MAX_RADIUS = 30;
const int MIN_RADIUS = 10;
const double DENSITY = 2;
const int VERTICES = 4;
const double G = 20;
const double STAR_SCALE = 0.5;

int generate_random_range(int lower, int upper) {
    // lower bound must be less than upper bound
    assert(lower < upper);
    return (int)(rand() % (upper - lower + 1) + lower);
}

vector_t generate_random_coords() {
    return (vector_t){generate_random_range(MIN_X, MAX_X),
                      generate_random_range(MIN_Y, MAX_Y)};
}

list_t *generate_initial_points(int vertices, int radius) {
    // ensure that valid number of vertices and radius is passed
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
            curr_pt[0] = vec_multiply(STAR_SCALE, vec_rotate(ref_pt, angle * i));
        }
        list_add(points, curr_pt);
    }

    // move the star so all its points have random x and y values
    polygon_translate(points, generate_random_coords());

    return points;
}

body_t *generate_body() {
    // make the points list
    int radius = (int)(rand() % (MAX_RADIUS - MIN_RADIUS + 1) + MIN_RADIUS);
    list_t *points = generate_initial_points(VERTICES, radius);

    // calculate mass using density
    double area = polygon_area(points);
    double mass = area * DENSITY;

    // get a random color
    rgb_color_t color =
        (rgb_color_t){((float)rand()) / RAND_MAX, ((float)rand()) / RAND_MAX,
                      ((float)rand()) / RAND_MAX};

    // initialize the body
    body_t *body = body_init(points, mass, color);
    return body;
}

void scene_setup(scene_t *scene) {
    // add all bodies to initial scene
    for (int i = 0; i < N_BODIES; i++) {
        scene_add_body(scene, generate_body());
        // add gravity between all bodies
        for (size_t j = 0; j < i; j++) {
            create_newtonian_gravity(scene, G, scene_get_body(scene, i),
                                     scene_get_body(scene, j));
        }
    }
}

int main() {
    srand(time(0));
    // initialize sdl demo
    sdl_init((vector_t){MIN_X, MIN_Y}, (vector_t){MAX_X, MAX_Y});

    // initialize scene and components
    scene_t *scene = scene_init();
    assert(scene != NULL);
    scene_setup(scene);

    double dt;

    while (!sdl_is_done(scene)) {
        // increment time
        dt = time_since_last_tick();

        // update each body in scene
        scene_tick(scene, dt);
        sdl_render_scene(scene);
    }
    scene_free(scene);
    return 0;
}
