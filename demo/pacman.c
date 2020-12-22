#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "body.h"
#include "sdl_wrapper.h"

const double MIN_Y = 0;
const double MIN_X = 0;
const double MAX_Y = 500;
const double MAX_X = 2 * MAX_Y;
const double SPAWN_TIME = 5.0;
const double DEFAULT_VEL = 100;
const double VEL_SCALE = 100;
const int INITIAL_PELLETS = 15;
const double PELLET_MASS = 1;
const int PELLET_RADIUS = 5;
const int CIRCLE_VERTICES = 1000;
const double PACMAN_MASS = 10000;
const int PACMAN_RADIUS = 50;
const double MOUTH_ANGLE = 60;
const double CONV_ANGLE = 180;

const rgb_color_t YELLOW = {1, (float)0.93, 0};

double generate_random_range(double lower, double upper) {
    // lower bound must be less than upper bound
    assert(lower < upper);
    return ((((float)rand() / RAND_MAX) * (upper - lower + 1)) + lower);
}

vector_t generate_random_coords() {
    return (vector_t){
        generate_random_range(MIN_X + (2 * PELLET_RADIUS), MAX_X - (2 * PELLET_RADIUS)),
        generate_random_range(MIN_Y + (2 * PELLET_RADIUS), MAX_Y - (2 * PELLET_RADIUS))};
}

list_t *generate_pellet_points(size_t vertices, int radius) {
    // ensure that valid number of vertices and radius is passed
    assert(vertices >= 2);
    assert(radius > 0);

    double angle = (2 * M_PI) / vertices;
    vector_t ref = {0, radius};
    list_t *points = list_init(vertices, (free_func_t)free);

    for (size_t i = 0; i < vertices; i++) {
        vector_t *curr = malloc(sizeof(vector_t));
        assert(curr != NULL);
        *curr = vec_rotate(ref, angle * i);
        list_add(points, curr);
    }
    return points;
}

body_t *generate_pellet() {
    list_t *points = generate_pellet_points(CIRCLE_VERTICES, PELLET_RADIUS);
    // check if this makes a new pellet every time
    body_t *pellet = body_init(points, PELLET_MASS, YELLOW);
    vector_t pos = generate_random_coords();
    body_set_centroid(pellet, pos);
    return pellet;
}

list_t *generate_pacman_points(size_t vertices, int radius) {
    // ensure that valid number of vertices and radius is passed
    assert(vertices >= 2);
    assert(radius >= 0);

    double mouth_angle_radians = MOUTH_ANGLE * M_PI / CONV_ANGLE;
    // the angle of rotation between each point on the circle
    double angle = (2 * M_PI - mouth_angle_radians) / vertices;
    vector_t ref = {0, radius};
    // rotating reference vector to mouth_angle_radians/2 above the x-axis
    // points to the "top lip" of pacman to begin
    ref = vec_rotate(ref, (mouth_angle_radians / 2) - (M_PI / 2));
    list_t *points = list_init(vertices + 1, (free_func_t)free);
    for (size_t i = 0; i <= vertices; i++) {
        vector_t *curr = malloc(sizeof(vector_t));
        assert(curr != NULL);
        if (i == 0) {
            // first vector is the center of pacman
            *curr = (vector_t){0, 0};
        } else {
            *curr = vec_rotate(ref, angle * (double)(i - 1));
        }
        list_add(points, curr);
    }
    return points;
}

body_t *generate_pacman() {
    list_t *points = generate_pacman_points(CIRCLE_VERTICES, PACMAN_RADIUS);
    body_t *pacman = body_init(points, PACMAN_MASS, YELLOW);
    body_set_velocity(pacman, (vector_t){DEFAULT_VEL, 0});
    body_set_centroid(pacman, (vector_t){MAX_X / 2, MAX_Y / 2});
    return pacman;
}

void scene_setup(scene_t *scene) {
    // add pacman and pellets to initial scene
    scene_add_body(scene, generate_pacman());
    for (int i = 0; i < INITIAL_PELLETS; i++) {
        scene_add_body(scene, generate_pellet());
    }
}

int pacman_collision(body_t *b1, body_t *b2) {
    list_t *pacman = body_get_shape(b1);
    vector_t pellet_centroid = body_get_centroid(b2);

    // get the center point of pacman (center of the circle making up pacman)
    vector_t pacman_cent = ((vector_t *)list_get(pacman, 0))[0];
    list_free(pacman);

    // check if pellet is within the the circle of pacman
    vector_t diff = vec_subtract(pellet_centroid, pacman_cent);
    double distance = sqrt(diff.x * diff.x + diff.y * diff.y);
    if (distance < PACMAN_RADIUS) {
        return 1;
    }

    return 0;
}

void update_pos(scene_t *scene) {
    // update pacman
    body_t *pacman = scene_get_body(scene, 0);

    vector_t pacman_vel = body_get_velocity(pacman);
    list_t *shape = body_get_shape(pacman);
    vector_t pacman_center = ((vector_t *)list_get(shape, 0))[0];
    vector_t pacman_centroid = body_get_centroid(pacman);

    // update pellets
    for (size_t i = 1; i < scene_bodies(scene); i++) {
        body_t *pellet = scene_get_body(scene, i);
        if (pacman_collision(pacman, pellet) == 1) {
            scene_remove_body(scene, i);
            i--;
        }
    }

    // wrap pacman around the screen if it hits an edge
    if (pacman_vel.x > 0 && pacman_center.x + PACMAN_RADIUS >= MAX_X) {
        body_set_centroid(pacman, vec_add(pacman_centroid, (vector_t){-1 * MAX_X, 0}));
    } else if (pacman_vel.x < 0 && pacman_center.x - PACMAN_RADIUS <= MIN_X) {
        body_set_centroid(pacman, vec_add(pacman_centroid, (vector_t){MAX_X, 0}));
    } else if (pacman_vel.y > 0 && pacman_center.y + PACMAN_RADIUS >= MAX_Y) {
        body_set_centroid(pacman, vec_add(pacman_centroid, (vector_t){0, -1 * MAX_Y}));
    } else if (pacman_vel.y < 0 && pacman_center.y - PACMAN_RADIUS <= MIN_Y) {
        body_set_centroid(pacman, vec_add(pacman_centroid, (vector_t){0, MAX_Y}));
    }

    list_free(shape);
}

void on_key(char key, key_event_type_t type, double held_time, scene_t *scene) {
    body_t *pacman = scene_get_body(scene, 0);
    list_t *shape = body_get_shape(pacman);
    vector_t center = ((vector_t *)list_get(shape, 0))[0];

    // if a key is pressed rotate pacman and change velocity accordingly
    if (type == KEY_PRESSED) {
        if (key == LEFT_ARROW) {
            body_set_rotation_about_point(pacman, M_PI, center);
            body_set_velocity(pacman,
                              (vector_t){-1 * held_time * VEL_SCALE - DEFAULT_VEL, 0});
        } else if (key == RIGHT_ARROW) {
            body_set_rotation_about_point(pacman, 0, center);
            body_set_velocity(pacman, (vector_t){held_time * VEL_SCALE + DEFAULT_VEL, 0});
        } else if (key == UP_ARROW) {
            body_set_rotation_about_point(pacman, M_PI / 2, center);
            body_set_velocity(pacman, (vector_t){0, held_time * VEL_SCALE + DEFAULT_VEL});
        } else if (key == DOWN_ARROW) {
            body_set_rotation_about_point(pacman, -1 * M_PI / 2, center);
            body_set_velocity(pacman,
                              (vector_t){0, -1 * held_time * VEL_SCALE - DEFAULT_VEL});
        }
    }
    // if a key is released revert to default velocity
    else if (type == KEY_RELEASED) {
        vector_t pacman_vel = body_get_velocity(pacman);
        if (key == LEFT_ARROW && pacman_vel.x < 0) {
            body_set_velocity(pacman, (vector_t){-1 * DEFAULT_VEL, 0});
        } else if (key == RIGHT_ARROW && pacman_vel.x > 0) {
            body_set_velocity(pacman, (vector_t){DEFAULT_VEL, 0});
        } else if (key == UP_ARROW && pacman_vel.y > 0) {
            body_set_velocity(pacman, (vector_t){0, DEFAULT_VEL});
        } else if (key == DOWN_ARROW && pacman_vel.y < 0) {
            body_set_velocity(pacman, (vector_t){0, -1 * DEFAULT_VEL});
        }
    }
    list_free(shape);
}

int main() {
    srand(time(0));
    // initialize sdl demo
    sdl_init((vector_t){MIN_X, MIN_Y}, (vector_t){MAX_X, MAX_Y});
    sdl_on_key(on_key);

    // initialize scene and components
    scene_t *scene = scene_init();
    assert(scene != NULL);
    scene_setup(scene);

    double t = SPAWN_TIME;
    double dt;

    while (!sdl_is_done(scene)) {
        // increment time
        dt = time_since_last_tick();
        t += dt;

        // spawn in new objects at given spawn time
        if (t >= SPAWN_TIME) {
            scene_add_body(scene, generate_pellet());
            t = 0.0;
        }

        // update each body in scene
        scene_tick(scene, dt);
        update_pos(scene);
        sdl_render_scene(scene);
    }
    scene_free(scene);
    return 0;
}
