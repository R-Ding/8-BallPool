#ifndef __PLAYER_H__
#define __PLAYER_H__

#include <stdbool.h>
#include "ball.h"
#include "list.h"

typedef struct player player_t;

player_t *player_init (char *info, vector_t coords, char *name);

void player_add_sunk(list_t *players, int turn, ball_t *ball);

list_t *player_get_balls_sunk (player_t *player);

vector_t player_get_coords (player_t *player);

vector_t player_get_coords (player_t *player);

int player_get_turn_state(player_t *player);

void player_set_turn_state (player_t *player, int state);

void player_set_foul(player_t *player, int new_foul);

char *player_get_info(player_t *player);

char *player_get_name(player_t *player);

int player_foul(player_t *player);

void player_free (player_t *player);

#endif // #ifndef __PLAYER_H__
