#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "player.h"
#include "vector.h"


//needs an "info" for solids stripes color etc (string), list of balls that have
//been sunk
typedef struct player {
    char *info;
    list_t *balls;
    //1 is hard scratch, 2 is one or more of the player's balls sunk, 3 is 8 ball sunk
    int turn_state;
    //true if foul has occurred during the turn
    int foul;
    vector_t coords;
    char *name;
    //int color; maybe for colors game mode
} player_t;

player_t *player_init (char *info, vector_t coords, char*name) {
    player_t *pl = malloc(sizeof(player_t));
    assert(pl != NULL);
    pl->info = info;
    pl->balls = list_init(16, (free_func_t)ball_free);
    pl->turn_state = 0;
    pl->foul = 0;
    pl->coords = coords;
    pl->name = name;
    return pl;
}

void player_add_sunk(list_t *players, int turn, ball_t *ball) {
    int num = ball_get_num(ball);
    player_t *p_turn = (player_t *)list_get(players, turn);

    if (num == 0) {
        p_turn->turn_state = 1;
    }
    else if (num == 8) {
        p_turn->turn_state = 3;
        list_add(p_turn->balls, ball);
    }
    else if ((strcmp(p_turn->info,"solid") == 0 && ball_get_solid(ball)) || (strcmp(p_turn->info,"stripe") == 0 && !ball_get_solid(ball)) ) {
        list_add(p_turn->balls, ball);
        if (!p_turn->foul) {
            p_turn->turn_state = 2;
        }
    }
    else {
        int other = (turn + 1) % list_size(players);
        player_t *other_p = list_get(players, other);
        list_add(other_p->balls, ball);
        p_turn->foul = 1;
        p_turn->turn_state = 0;
    }
}

vector_t player_get_coords (player_t *player) {
    return player->coords;
}

char *player_get_name(player_t *player) {
    return player->name;
}

list_t *player_get_balls_sunk (player_t *player) {
    return player->balls;
}

int player_get_turn_state(player_t *player) {
    return player->turn_state;
}

void player_set_turn_state (player_t *player, int state) {
    player->turn_state = state;
}

void player_set_foul(player_t *player, int new_foul) {
    player->foul = new_foul;
}

char *player_get_info(player_t *player) {
    return player->info;
}

int player_foul(player_t *player) {
    return player->foul;
}

void player_free (player_t *player) {
    free(player->balls);
    free(player);
}
