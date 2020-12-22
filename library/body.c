#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include "body.h"
#include "polygon.h"
#include <string.h>
#include <stdio.h>

typedef struct body {
    list_t *shape;
    double mass;
    rgb_color_t color;
    vector_t velocity;
    vector_t acceleration;
    vector_t centroid;
    double angle;
    vector_t force;
    vector_t impulse;
    void *info;
    void *info_freer;
    int removed;
} body_t;

body_t *body_init(list_t *shape, double mass, rgb_color_t color) {
    //use other constructer with no info or info_freer
    return body_init_with_info(shape, mass, color, NULL, NULL);
}

body_t *body_init_with_info(list_t *shape, double mass, rgb_color_t color, void *info, free_func_t info_freer) {
    body_t *body = malloc(sizeof(body_t));
    assert(body != NULL);
    assert(mass > 0.0);
    body->shape = shape;
    body->mass = mass;
    body->color = color;
    body->velocity = (vector_t){0, 0};
    body->acceleration = (vector_t){0, 0};
    body->centroid = polygon_centroid(shape);
    body->angle = 0;
    body->force = (vector_t){0, 0};
    body->impulse = (vector_t){0, 0};
    body->info = info;
    body->info_freer = info_freer;
    body->removed = 0;
    return body;
}

void body_free(body_t *body) {
    list_free(body->shape);
    if (body->info_freer != NULL) {
        ((free_func_t)(body->info_freer))(body->info);
    }
    free(body);
}

list_t *body_get_shape(body_t *body) {
    list_t *copy = list_init(list_size(body->shape), (free_func_t)free);
    assert(copy != NULL);
    for (size_t i = 0; i < list_size(body->shape); i++) {
        vector_t *vec = malloc(sizeof(vector_t));
        assert(vec != NULL);
        *vec = ((vector_t *)list_get(body->shape, i))[0];
        list_add(copy, vec);
    }
    return copy;
}

vector_t body_get_centroid(body_t *body) {
    return body->centroid;
}

vector_t body_get_velocity(body_t *body) {
    return body->velocity;
}

void body_set_angle(body_t *body, double angle) {
    body->angle = angle;
}

double body_get_angle(body_t *body) {
    return body->angle;
}

double body_get_mass(body_t *body) {
    return body->mass;
}

rgb_color_t body_get_color(body_t *body) {
    return body->color;
}

void *body_get_info(body_t *body) {
    return body->info;
}

void body_set_centroid(body_t *body, vector_t x) {
    vector_t old_centroid = body_get_centroid(body);
    polygon_translate(body->shape, vec_subtract(x, old_centroid));
    body->centroid = x;
}

void body_translate(body_t *body, vector_t x) {
    polygon_translate(body->shape, x);
    body->centroid = vec_add(body->centroid, x);
}

void body_set_velocity(body_t *body, vector_t v) {
    body->velocity = v;
}

void body_set_rotation(body_t *body, double angle) {
    double new_angle = angle - body->angle;
    polygon_rotate(body->shape, new_angle, body_get_centroid(body));
    body->angle = angle;
}

void body_set_rotation_about_point(body_t *body, double angle, vector_t point) {
    double new_angle = angle - body->angle;
    polygon_rotate(body->shape, new_angle, point);
    body->centroid = polygon_centroid(body->shape);
    body->angle = angle;
}

void body_add_force(body_t *body, vector_t force) {
    body->force = vec_add(body->force, force);
}

void body_add_impulse(body_t *body, vector_t impulse) {
    body->impulse = vec_add(body->impulse, impulse);
}

void body_reset_impulse(body_t *body) {
    body->impulse = (vector_t) {0, 0};
}

void body_tick(body_t *body, double dt) {
    //printf("body body_tick\n");
    //if (body_get_info(body) != NULL && strcmp((char *)body_get_info(body), "./assets/ball0.png") != 0) {
    //printf("ball x centroid: %f\n", body_get_centroid(body).x);
    //printf("ball x velocity: %f\n", body_get_velocity(body).x);

    vector_t ave_vel = body->velocity;
    if (body->mass != DBL_MAX) {
        vector_t old_vel = body->velocity;
        vector_t vel_change = vec_multiply(1 / body->mass, body->impulse);

        body->acceleration = vec_multiply((1 / body->mass), body->force);
        body->velocity = vec_add(old_vel, vec_multiply(dt, body->acceleration));
        body->velocity = vec_add(body->velocity, vel_change);

        double avg = 1.0 / (double)2;
        ave_vel = vec_multiply(avg, vec_add(old_vel, body->velocity));
        // Minimum cutoff for body velocity in order to prevent bodies
        // experiencing friction to slow down forever. This makes them stop
        // after a certain threshold
        if (vec_magnitude(body->velocity) <= 0.25) {
             body->velocity = (vector_t){0, 0};
        }
    }
    body_translate(body, vec_multiply(dt, ave_vel));
    body->force = (vector_t){0, 0};
    body->impulse = (vector_t){0, 0};
}

void body_remove(body_t *body) {
    //mark body for removal
    body->removed = 1;
}

bool body_is_removed(body_t *body) {
    //check whether body has been marked for removal
    return (body->removed);
}
