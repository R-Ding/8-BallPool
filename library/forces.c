#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "body.h"
#include "ball.h"
#include "player.h"
#include "collision.h"
#include "forces.h"
#include "scene.h"

// to be used as auxiliary state
typedef struct force_bodies
{
    double force_const;
    list_t *bodies;
    collision_handler_t handler;
    void *aux;
    bool ball_collision;
    free_func_t freer;
    int collided;
} force_bodies_t;

typedef struct collision_values
{
    double elasticity;
    list_t *to_remove;
    scene_t *scene;
    ball_t *to_move;
    double scale;
} collision_values_t;

const double MIN_DIST = 5;

double get_length(vector_t v);

void force_bodies_free(force_bodies_t *fb)
{
    if (fb->aux != NULL && fb->freer != NULL)
    {
        ((free_func_t)fb->freer)(fb->aux);
    }
    //list_free(fb->bodies);
    free(fb);
}

void collision_values_free(collision_values_t *cv)
{
    if (cv->to_remove != NULL)
    {
        list_free(cv->to_remove);
    }
    free(cv);
}

void gravity(void *aux)
{
    force_bodies_t *fb = aux;
    double G = fb->force_const;
    body_t *body1 = list_get(fb->bodies, 0);
    body_t *body2 = list_get(fb->bodies, 1);
    // compute distance between bodies
    vector_t b1_centroid = body_get_centroid(body1);
    vector_t b2_centroid = body_get_centroid(body2);
    vector_t r12 = vec_subtract(b1_centroid, b2_centroid);
    double dist = get_length(r12);
    // avoids small number division
    if (dist > MIN_DIST)
    {
        // perform newtonian gravity calculations and apply to each body
        double scaling_factor =
            -1.0 * G * body_get_mass(body1) * body_get_mass(body2) / dist / dist / dist;
        vector_t force = vec_multiply(scaling_factor, r12);
        body_add_force(body1, force);
        body_add_force(body2, vec_negate(force));
    }
}

void create_newtonian_gravity(scene_t *scene, double G, body_t *body1, body_t *body2)
{
    // creates auxiliary state struct and passes to scene force creator
    force_bodies_t *aux = malloc(sizeof(force_bodies_t));
    assert(aux != NULL);
    aux->force_const = G;
    list_t *bodies = list_init(2, NULL);
    list_add(bodies, body1);
    list_add(bodies, body2);
    aux->bodies = bodies;
    aux->handler = NULL;
    aux->aux = NULL;
    aux->ball_collision = false;
    aux->freer = NULL;
    aux->collided = 0;
    scene_add_bodies_force_creator(scene, (force_creator_t)gravity, aux, bodies, (free_func_t)force_bodies_free);
}

void spring(void *aux)
{
    force_bodies_t *fb = aux;
    double k = fb->force_const;
    body_t *b1 = list_get(fb->bodies, 0);
    body_t *b2 = list_get(fb->bodies, 1);
    // compute distance between bodies
    vector_t b1_centroid = body_get_centroid(b1);
    vector_t b2_centroid = body_get_centroid(b2);
    vector_t r12 = vec_subtract(b1_centroid, b2_centroid);
    // perform hooke's law calculations and apply to each body
    vector_t force = vec_multiply(k, r12);
    body_add_force(b1, vec_negate(force));
    body_add_force(b2, force);
}

void create_spring(scene_t *scene, double k, body_t *body1, body_t *body2)
{
    // creates auxiliary state struct and passes to scene force creator
    force_bodies_t *fb = malloc(sizeof(force_bodies_t));
    assert(fb != NULL);
    fb->force_const = k;
    list_t *bodies = list_init(2, NULL);
    list_add(bodies, body1);
    list_add(bodies, body2);
    fb->bodies = bodies;
    fb->handler = NULL;
    fb->ball_collision = false;
    fb->aux = NULL;
    fb->freer = NULL;
    fb->collided = 0;
    scene_add_bodies_force_creator(scene, (force_creator_t)spring, fb, bodies, (free_func_t)force_bodies_free);
}

void drag(void *aux)
{
    force_bodies_t *fb = aux;
    double gamma = fb->force_const;
    body_t *body = list_get(fb->bodies, 0);
    // applies basic drag by scaling velocity by gamma and applies to body
    vector_t force = vec_multiply(gamma, body_get_velocity(body));
    body_add_force(body, vec_negate(force));
}

void create_drag(scene_t *scene, double gamma, body_t *body)
{
    // creates auxiliary state struct and passes to scene force creator
    force_bodies_t *fb = malloc(sizeof(force_bodies_t));
    assert(fb != NULL);
    fb->force_const = gamma;
    list_t *bodies = list_init(1, NULL);
    list_add(bodies, body);
    fb->bodies = bodies;
    fb->handler = NULL;
    fb->ball_collision = false;
    fb->aux = NULL;
    fb->freer = NULL;
    fb->collided = 0;
    scene_add_bodies_force_creator(scene, (force_creator_t)drag, fb, bodies, (free_func_t)force_bodies_free);
}

double get_length(vector_t v)
{
    double len = (v.x) * (v.x) + (v.y) * (v.y);
    return (sqrt(len));
}

void collision(void *aux)
{
    force_bodies_t *fb = aux;
    body_t *b1 = list_get(fb->bodies, 0);
    double v1 = vec_magnitude(body_get_velocity(b1));
    body_t *b2 = list_get(fb->bodies, 1);
    double v2 = vec_magnitude(body_get_velocity(b2));
    list_t *s1 = body_get_shape(b1);
    list_t *s2 = body_get_shape(b2);
    //if the two bodies collide, call the collision handler
    collision_info_t col;
    if (fb->ball_collision) {
        col = find_collision_balls(b1, b2);
    }
    else {
        col = find_collision(s1, s2);
    }
    if (col.collided && !(fb->collided))
    {
        ((collision_handler_t)fb->handler)(b1, b2, col.axis, fb->aux);
        if ((v1 != 0 || v2 != 0)) {
            fb->collided = 1;
        }
    }
    else if (!col.collided)
    {
        fb->collided = 0;
    }
    list_free(s1);
    list_free(s2);
}

void create_collision(scene_t *scene, body_t *body1, body_t *body2, collision_handler_t handler, void *aux, free_func_t freer, bool ball_collision)
{
    force_bodies_t *fb = malloc(sizeof(force_bodies_t));
    assert(fb != NULL);
    fb->force_const = 0.0;
    list_t *bodies = list_init(2, NULL);
    list_add(bodies, body1);
    list_add(bodies, body2);
    fb->bodies = bodies;
    fb->handler = handler;
    fb->ball_collision = ball_collision;
    fb->aux = aux;
    fb->freer = freer;
    fb->collided = 0;
    scene_add_bodies_force_creator(scene, (force_creator_t)collision, fb, bodies, (free_func_t)force_bodies_free);
}

void destructive_collision(body_t *body1, body_t *body2, vector_t axis, void *aux)
{
    //if the two bodies collide, mark them for removal
    body_remove(body1);
    body_remove(body2);
}

void create_destructive_collision(scene_t *scene, body_t *body1, body_t *body2)
{
    create_collision(scene, body1, body2, (collision_handler_t)destructive_collision, NULL, NULL, false);
}

void physics_collision(body_t *body1, body_t *body2, vector_t axis, void *aux)
{
    //printf("collision\n");
    collision_values_t *cv = aux;
    double elasticity = cv->elasticity;
    list_t *bodies_for_removal = cv->to_remove;
    ball_t *ball = (ball_t *)cv->to_move;
    scene_t *scene = cv->scene;

    //printf("vel1: %f\n", vec_magnitude(body_get_velocity(body1)));
    //printf("vel2: %f\n", vec_magnitude(body_get_velocity(body2)));

    double ua = vec_dot(body_get_velocity(body1), axis);
    double ub = vec_dot(body_get_velocity(body2), axis);

    double m1 = body_get_mass(body1);
    double m2 = body_get_mass(body2);
    double mass_correction = 0;

    if (m1 == INFINITY && m2 != INFINITY)
    {
        mass_correction = m2;
    }
    else if (m2 == INFINITY && m1 != INFINITY)
    {
        mass_correction = m1;
    }
    else if (m1 != INFINITY && m2 != INFINITY)
    {
        mass_correction = (m1 * m2) / (m1 + m2);
    }

    double impulse = mass_correction * (1 + elasticity) * (ub - ua);
    body_add_impulse(body1, vec_multiply(cv->scale + impulse, axis));
    body_add_impulse(body2, vec_multiply(-1 * (cv->scale + impulse), axis));
    //printf("impulse:%f\n", impulse);
    //printf("ub: %f, ua: %f\n", ub, ua);
    //printf("mass correction: %f\n", mass_correction);

    if (bodies_for_removal != NULL)
    {
        for (size_t i = 0; i < list_size(bodies_for_removal); i++)
        {
            body_remove(list_get(bodies_for_removal, i));
        }
    }
    else if (ball != NULL)
    {
        list_t *players = scene_get_players(scene);
        player_add_sunk(players, scene_get_turn(scene), ball);
        if (ball_get_num(ball) == 0)
        {
            body_set_centroid(body1, (vector_t){150, 250});
            body_reset_impulse(body1);
            body_set_velocity(body1, (vector_t){0, 0});
        }
    }
}

void create_physics_collision(scene_t *scene, double elasticity, body_t *body1, body_t *body2, bool ball_collision)
{
    collision_values_t *cv = malloc(sizeof(collision_values_t));
    assert(cv != NULL);
    cv->elasticity = elasticity;
    cv->to_remove = NULL;
    cv->scene = NULL;
    cv->to_move = NULL;
    cv->scale = 0.0;
    create_collision(scene, body1, body2, (collision_handler_t)physics_collision, cv, (free_func_t)collision_values_free, ball_collision);
}

void create_physics_collision_with_removal(scene_t *scene, double elasticity, body_t *body1, body_t *body2, list_t *bodies, double scale)
{
    collision_values_t *cv = malloc(sizeof(collision_values_t));
    assert(cv != NULL);
    cv->elasticity = elasticity;
    cv->to_remove = bodies;
    cv->scene = NULL;
    cv->to_move = NULL;
    cv->scale = scale;
    create_collision(scene, body1, body2, (collision_handler_t)physics_collision, cv, (free_func_t)collision_values_free, false);
}

void create_physics_collision_with_translation(scene_t *scene, double elasticity, body_t *body1, body_t *body2, ball_t *to_move)
{
    collision_values_t *cv = malloc(sizeof(collision_values_t));
    assert(cv != NULL);
    // get the players list from the scene
    cv->elasticity = elasticity;
    cv->to_remove = NULL;
    cv->scale = 0.0;
    cv->scene = scene;
    cv->to_move = to_move;
    create_collision(scene, body1, body2, (collision_handler_t)physics_collision, cv, (free_func_t)collision_values_free, false);
}

void ideal_friction(void *aux)
{
    force_bodies_t *fb = aux;
    double mug = fb->force_const;
    body_t *body = list_get(fb->bodies, 0);
    double mass = body_get_mass(body);
    if (vec_magnitude(body_get_velocity(body)) > 5) {
        vector_t friction = vec_multiply(mass * mug, body_get_velocity(body));
        body_add_force(body, vec_negate(friction)); //friction acts opposite to the direction of motion
    }
    else {
        body_set_velocity(body, (vector_t) {0, 0});
    }
}

void create_ideal_friction(scene_t *scene, double mug, body_t *body)
{
    force_bodies_t *fb = malloc(sizeof(force_bodies_t));
    assert(fb != NULL); // malloc worked!
    fb->force_const = mug;
    list_t *bodies = list_init(1, NULL);
    list_add(bodies, body);
    fb->bodies = bodies;
    fb->handler = NULL;
    fb->ball_collision = false;
    fb->aux = NULL;
    fb->freer = NULL;
    fb->collided = 0;
    scene_add_bodies_force_creator(scene, (force_creator_t)ideal_friction, fb, bodies, (free_func_t)force_bodies_free);
}
