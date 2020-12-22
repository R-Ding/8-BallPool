#include "scene.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "forces.h"
#include "player.h"
#include "ball.h"
#include "sdl_wrapper.h"

const int BODIES = 50;
const int FORCE_CREATORS = 5;
const int BALLS = 16;
const int PLAYERS = 2;


//struct to hold a force, its argument pointer, freer, and list of bodies
typedef struct force_struct {
    force_creator_t force;
    void *arg;
    free_func_t freer;
    list_t *bodies;
} force_struct_t;

typedef struct scene {
    list_t *bodies;
    list_t *forces;
    int state;
    list_t *balls;
    list_t *players;
    int turn;
} scene_t;



void force_struct_free (force_struct_t *st);

scene_t *scene_init(void) {
    scene_t *sc = malloc(sizeof(scene_t));
    assert(sc != NULL);
    sc->bodies = list_init(BODIES, (free_func_t)body_free);
    sc->forces = list_init(FORCE_CREATORS, (free_func_t)force_struct_free);
    sc->state = 1;
    sc->balls = list_init(BALLS, (free_func_t)ball_free);
    sc->players = list_init(PLAYERS, (free_func_t)player_free);
    sc->turn = 0;
    return sc;
}

int scene_get_state(scene_t *scene) {
    return scene->state;
}

int scene_set_state(scene_t *scene, int st) {
    int game_state = scene->state;
    scene->state = st;
    return game_state;
}

int scene_get_turn(scene_t *scene) {
    return scene->turn;
}

list_t *scene_get_players(scene_t *scene) {
    return scene->players;
}

void scene_add_player(scene_t *scene, player_t *player) {
    return list_add(scene->players, player);
}

player_t *scene_get_player(scene_t *scene, int index) {
    return list_get(scene->players, index);
}

void scene_switch_turn (scene_t *scene) {
    scene->turn = (scene->turn + 1) % list_size(scene->players);
    player_t *player_1 = scene_get_player(scene, 0);
    player_set_turn_state(player_1, 0);
    player_t *player_2 = scene_get_player(scene, 1);
    player_set_turn_state(player_2, 0);
}

list_t *scene_get_balls (scene_t *scene) {
    return scene->balls;
}

void scene_free(scene_t *scene) {
    list_free(scene->bodies);
    list_free(scene->forces);
    list_free(scene->balls);
    list_free(scene->players);
    free(scene);
}

size_t scene_bodies(scene_t *scene) {
    return list_size(scene->bodies);
}

body_t *scene_get_body(scene_t *scene, size_t index) {
    assert(index < scene_bodies(scene));
    return (body_t *)list_get(scene->bodies, index);
}

void scene_add_body(scene_t *scene, body_t *body) {
    list_add(scene->bodies, body);
}

void scene_remove_body(scene_t *scene, size_t index) {
    assert(index < scene_bodies(scene));
    body_remove(list_get(scene->bodies, index));
}

void scene_add_force_creator(scene_t *scene, force_creator_t forcer, void *aux,
                             free_func_t freer) {
    scene_add_bodies_force_creator(scene, forcer, aux, NULL, freer);
}

void force_struct_free (force_struct_t *st) {
    //free the arguments
    if (st->arg != NULL && st->freer != NULL) {
        (st->freer)(st->arg);
    }
    //free the list of bodies
    if (st->bodies != NULL) {
        list_free(st->bodies);
    }
    free(st);
}

void scene_add_bodies_force_creator(scene_t *scene, force_creator_t forcer, void *aux,
                                    list_t *bodies, free_func_t freer) {
    force_struct_t *frc = malloc(sizeof(force_struct_t));
    assert(frc != NULL);
    frc->force = forcer;
    frc->arg = aux;
    frc->freer = freer;
    frc->bodies = bodies;
    list_add(scene->forces, frc);
}


void scene_add_ball_list(scene_t *scene, list_t *balls) {
    scene->balls = balls;
    for (int i = 0; i < list_size(balls); i++) {
        list_add(scene->bodies, ball_get_body(list_get(balls, i)));
    }
}

void scene_tick(scene_t *scene, double dt) {
    //apply all forces in the scene
    for (size_t j = 0; j < list_size(scene->forces); j++) {
        force_struct_t *fstruct = list_get(scene->forces, j);
        (fstruct->force)(fstruct->arg);
    }
    // tick each body in the scene
    for (size_t i = 0; i < scene_bodies(scene); i++) {
        body_t *curr =(body_t *)list_get(scene->bodies, i);
        body_tick(curr, dt);
    }

    //remove all forces that contain a body marked for removal
    for (size_t j = 0; j < list_size(scene->forces); j++) {
        int removed = 0;
        force_struct_t *fstruct = list_get(scene->forces, j);
        if (fstruct->bodies != NULL) {
            for (size_t k = 0; k < list_size(fstruct->bodies); k++) {
                body_t *body = list_get(fstruct->bodies, k);
                if (body_is_removed(body) == 1) {
                    removed = 1;
                }
            }
        }

        if (removed == 1) {
            force_struct_free(list_remove(scene->forces, j));
            j--;
        }
    }

    //remove all bodies marked for removal
    for (size_t i = 0; i < scene_bodies(scene); i++) {
        body_t *curr =(body_t *)list_get(scene->bodies, i);
        if (body_is_removed(curr) == 1) {
            body_t *curr_body = list_remove(scene->bodies, i);
            i--;
            body_free(curr_body);
        }
    }
}
