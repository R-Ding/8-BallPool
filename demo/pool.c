#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include "forces.h"
#include "scene.h"
#include "sdl_wrapper.h"
#include "collision.h"
#include "mouse.h"
#include "color.h"

const int MIN_Y = 0;
const int MIN_X = 0;
const int MAX_Y = 500;
const int MAX_X = 2*MAX_Y;
const int OBJ_SPACING = 10;
const int NUM_COLS = 10;
const int NUM_ROWS = 3;
const int BOX_WIDTH = (MAX_Y - OBJ_SPACING*(NUM_COLS+1)) / NUM_COLS;
const int BOX_HEIGHT = BOX_WIDTH / 3;
//const int BALL_RADIUS = BOX_HEIGHT / 2;
const int CUE_WIDTH = 10;
const int CUE_HEIGHT = 300;
const int CUE_MASS = 1000;
const double DISTANCE = 4;
const double VEL_SCALAR = 100;
const int BOX_VERTICES = 4;
const double BOX_MASS = INFINITY;
const int BALL_INDEX = 12;
const int NUM_BALLS = 16;
const double WALL_ELASTICITY = 0.8;
const double BALL_ELASTICITY = 0.92;
const double FRICTION_CONST = 0.27;//9.8*0.25;
const int WALL_POINTS = 6;
const vector_t LEFT_WALL[] = {{297, 388}, {301, 388}, {309, 379}, {309, 125}, {299, 115}, {297, 115}};
const vector_t RIGHT_WALL[] = {{948, 387}, {943, 387}, {935, 376}, {935, 127}, {944, 116}, {948, 116}};
const vector_t TOP_LEFT_WALL[] = {{322, 414}, {322, 410}, {332, 402}, {601, 402}, {605, 410}, {605, 414}};
const vector_t TOP_RIGHT_WALL[] = {{639, 414}, {639, 410}, {643, 402}, {914, 402}, {922, 409}, {922, 413}};
const vector_t BOTTOM_LEFT_WALL[] = {{321, 93}, {330, 101}, {599, 101}, {604, 92}, {604, 89}, {321, 89}};
const vector_t BOTTOM_RIGHT_WALL[] = {{639, 92}, {646, 101}, {913, 101}, {923, 93}, {923, 90}, {639, 90}};
const int POCKET_POINTS = 3;
const vector_t TOP_LEFT_POCKET[] = {{298, 393}, {317, 412}, {297, 394}};
const vector_t TOP_RIGHT_POCKET[] = {{926, 411}, {945, 381}, {941, 408}};
const vector_t BOTTOM_LEFT_POCKET[] = {{315, 89}, {295, 109}, {295, 112}};
const vector_t BOTTOM_RIGHT_POCKET[] = {{930, 89}, {950, 109}, {944, 95}};
const vector_t TOP_POCKET[] = {{606, 416}, {637, 416}, {622, 424}};
const vector_t BOTTOM_POCKET[] = {{622, 80}, {609, 82}, {623, 72}, {635, 82}};
const vector_t KITCHEN[] = {{309, 101}, {463, 402}};
const vector_t FIRST_BALL_COORDS = {778, 250};
const vector_t MENU_BUTTON_1[] = {{267, 270}, {613, 328}};
const vector_t MENU_BUTTON_2[] = {{629, 270}, {975, 328}};
const vector_t PLAYER_1_INFO_BUBBLE[] = {{29, 420}, {44, 434}, {227, 434}, {242, 420}, {242, 286}, {227, 271}, {44, 271}, {29, 286}};
const vector_t PLAYER_2_INFO_BUBBLE[] = {{29, 212}, {44, 227}, {227, 227}, {242, 212}, {242, 79}, {227, 64}, {44, 64}, {29, 79}};
const vector_t PLAYER_1_BALL_RACK = {55, 190};
const vector_t PLAYER_2_BALL_RACK = {55, 400};
const int BALL_RACK_SPACING = 3;

void scene_add_wall(scene_t *scene, const vector_t coords[], int elements) {
    list_t *points = list_init(elements, (free_func_t) free);
    for (size_t i = 0; i < elements; i++) {
        vector_t *vec = malloc(sizeof(vector_t));
        vec[0] = coords[i];
        list_add(points, vec);
    }
    body_t *wall = body_init(points, BOX_MASS, (rgb_color_t) {0,0,0});
    scene_add_body(scene, wall);
}

list_t *cue_generate_points(vector_t dimensions) {
    double width = dimensions.x;
    double height = dimensions.y;
    list_t *points = list_init(BOX_VERTICES, (free_func_t)free);
    vector_t *v1 = malloc(sizeof(vector_t));
    assert (v1 != 0);
    v1[0] = (vector_t){0.5*width, 0.5*height};
    list_add(points, v1);

    vector_t *v2 = malloc(sizeof(vector_t));
    assert (v2 != 0);
    v2[0] = (vector_t){-0.5*width, 0.5*height};
    list_add(points, v2);

    vector_t *v3 = malloc(sizeof(vector_t));
    assert (v3 != 0);
    v3[0] = (vector_t){-0.5*width, -0.5*height};
    list_add(points, v3);

    vector_t *v4 = malloc(sizeof(vector_t));
    assert (v4 != 0);
    v4[0] = (vector_t){0.5*width, -0.5*height};
    list_add(points, v4);
    return points;
}

void scene_add_ball_collisions(scene_t *scene, list_t *balls)
{
    for (int i = 0; i < list_size(balls); i++)
    {
        ball_t *curr_ball = (ball_t *)list_get(balls, i);
        body_t *curr_body = ball_get_body(curr_ball);
        create_ideal_friction(scene, FRICTION_CONST, curr_body);
        for (int j = 0; j < 6; j++)
        {
            body_t *wall = scene_get_body(scene, j);
            create_physics_collision(scene, WALL_ELASTICITY, curr_body, wall, false);
        }
        for (int l = 6; l < BALL_INDEX; l++)
        {
            // either use physics collision with removal or make a new physics collision to translate the pocketed ball
            body_t *pocket = scene_get_body(scene, l);
            if (i == 0)
            {
                create_physics_collision_with_translation(scene, WALL_ELASTICITY, curr_body, pocket, curr_ball);
            }
            else
            {
                create_physics_collision_with_translation(scene, WALL_ELASTICITY, curr_body, pocket, curr_ball);
            }
        }
        for (int k = i + 1; k < NUM_BALLS; k++)
        {
            body_t *temp_body = ball_get_body(list_get(balls, k));
            create_physics_collision(scene, BALL_ELASTICITY, curr_body, temp_body, true);
        }
    }
}

void scene_add_balls(scene_t *scene, vector_t cue_start, vector_t ball_start) {
    list_t *balls = scene_get_balls(scene);
    list_add(balls, ball_init(0, cue_start));
    double radius = ball_get_radius(list_get(balls, 0));
    vector_t curr = ball_start;
    list_add(balls, ball_init(1, ball_start));
    double x_inc = cos(M_PI/6)*2*radius;
    double y_inc = sin(M_PI/6)*2*radius;
    curr = vec_add(curr, (vector_t){x_inc, y_inc});
    vector_t loc = curr;
    for (size_t i = 2; i < 4; i++) {
        loc = vec_subtract(curr, vec_multiply((i-2), (vector_t){0,2*radius}));
        list_add(balls, ball_init(i, loc));
    }
    curr = vec_add(curr, (vector_t){x_inc, y_inc});
    for (size_t j = 4; j < 7; j++) {
        loc = vec_subtract(curr, vec_multiply((j-4), (vector_t){0,2*radius}));
        list_add(balls, ball_init(j, loc));
    }
    curr = vec_add(curr, (vector_t){x_inc, y_inc});
    for (size_t k = 7; k < 11; k++) {
        loc = vec_subtract(curr, vec_multiply((k-7), (vector_t){0,2*radius}));
        list_add(balls, ball_init(k, loc));
    }
    curr = vec_add(curr, (vector_t){x_inc, y_inc});
    for (size_t l = 11; l < NUM_BALLS; l++) {
        loc = vec_subtract(curr, vec_multiply((l-11), (vector_t){0,2*radius}));
        list_add(balls, ball_init(l, loc));
    }

    size_t cent_ball_num = 5;
    vector_t cent_loc = body_get_centroid(ball_get_body(list_get(balls, cent_ball_num)));

    float rand_swaps = ((((float)rand() / RAND_MAX) * (NUM_BALLS + 1)));
    for (size_t m = 0; m < rand_swaps; m++) {
        size_t rand1 = (size_t)((((float)rand() / RAND_MAX) * (NUM_BALLS - 1))+1);
        size_t rand2 = (size_t)((((float)rand() / RAND_MAX) * (NUM_BALLS - 1))+1);
        while (rand2 == rand1) {
            rand2 = (size_t)((((float)rand() / RAND_MAX) * (NUM_BALLS - 1))+1);
        }
        body_t *r1_body = ball_get_body(list_get(balls, rand1));
        body_t *r2_body = ball_get_body(list_get(balls, rand2));
        vector_t r1_cent = body_get_centroid(r1_body);
        vector_t r2_cent = body_get_centroid(r2_body);
        body_set_centroid(r1_body, r2_cent);
        if (r2_cent.x == cent_loc.x && r2_cent.y == cent_loc.y) {
            cent_ball_num = ball_get_num(list_get(balls, rand1));
        }
        body_set_centroid(r2_body, r1_cent);
        if (r1_cent.x == cent_loc.x && r1_cent.y == cent_loc.y) {
            cent_ball_num = ball_get_num(list_get(balls, rand2));
        }
    }
    body_t *cent_body = ball_get_body(list_get(balls, cent_ball_num));
    body_t *eight_ball = ball_get_body(list_get(balls, 8));
    vector_t eight_ball_cent = body_get_centroid(eight_ball);
    body_set_centroid(eight_ball, cent_loc);
    body_set_centroid(cent_body, eight_ball_cent);

    scene_add_ball_collisions(scene, balls);
    scene_add_ball_list(scene, balls);

}

void scene_add_pockets(scene_t *scene) {
    //adds bodies that the balls can collide with in the pockets
    scene_add_wall(scene, TOP_LEFT_POCKET, POCKET_POINTS);
    scene_add_wall(scene, TOP_POCKET, POCKET_POINTS);
    scene_add_wall(scene, TOP_RIGHT_POCKET, POCKET_POINTS);
    scene_add_wall(scene, BOTTOM_RIGHT_POCKET, POCKET_POINTS);
    scene_add_wall(scene, BOTTOM_POCKET, POCKET_POINTS);
    scene_add_wall(scene, BOTTOM_LEFT_POCKET, POCKET_POINTS);
}

void scene_add_cue(scene_t *scene, rgb_color_t col) {
    ball_t *cb = (ball_t *)list_get(scene_get_balls(scene), 0);
    body_t *cueball = ball_get_body(cb);
    vector_t coords = vec_subtract(body_get_centroid(cueball), (vector_t){ball_get_radius(cb)*2 + .5*CUE_HEIGHT, 0});
    list_t *shape = cue_generate_points((vector_t){CUE_HEIGHT, CUE_WIDTH});
    rgb_color_t color = {0.0, 0.0, 0.0};
    char *info = "./assets/cue.png";
    body_t *cue = body_init_with_info(shape, CUE_MASS, color, info, NULL);
    body_set_angle(cue, M_PI);
    body_set_centroid(cue, coords);
    scene_add_body(scene, cue);


    size_t cue_index = scene_bodies(scene)-1;
    body_t *cue_body = scene_get_body(scene, cue_index);
    list_t *for_aux = list_init(1, NULL);
    list_add(for_aux, cue_body);

    create_physics_collision_with_removal(scene, 1.0, cueball, cue_body, for_aux, 0.0);
}

void populate_scene(scene_t *scene) {
    //add walls
    scene_add_wall(scene, TOP_LEFT_WALL, WALL_POINTS);
    scene_add_wall(scene, TOP_RIGHT_WALL, WALL_POINTS);
    scene_add_wall(scene, BOTTOM_LEFT_WALL, WALL_POINTS);
    scene_add_wall(scene, BOTTOM_RIGHT_WALL, WALL_POINTS);
    scene_add_wall(scene, LEFT_WALL, WALL_POINTS);
    scene_add_wall(scene, RIGHT_WALL, WALL_POINTS);

    //ADD 6 boxes for the pockets, rotate the corner boxes
    scene_add_pockets(scene);
    //add player
    player_t *player1 = player_init("solid", PLAYER_1_BALL_RACK, "Player1");
    player_t *player2 = player_init("stripe", PLAYER_2_BALL_RACK, "Player2");
    scene_add_player(scene, player1);
    scene_add_player(scene, player2);
    //add ball
    vector_t cue_ball_start_coords = {400,250};
    scene_add_balls(scene, cue_ball_start_coords, FIRST_BALL_COORDS);

}



bool balls_moving(list_t *balls) {
    for (int i = 0; i < NUM_BALLS; i++) {
        body_t *ball = ball_get_body(list_get(balls, i));
        if (body_get_velocity(ball).x != 0.0 || body_get_velocity(ball).y != 0.0) {
            return true;
        }
    }
    return false;
}

void update_game_state(scene_t *scene) {
    list_t *balls = scene_get_balls(scene);
    // firing
    if (scene_get_state(scene) == 2) {
        bool moving = balls_moving(balls);
        if (!moving && scene_bodies(scene) == 28) {
            //printf("inside moving!\n");
            scene_add_cue(scene, (rgb_color_t) {0,0,0});
        }
    }
    // settling
    else if (scene_get_state(scene) == 3) {
        if (!balls_moving(balls) && scene_bodies(scene) == 28) {
            player_t *curr_player = scene_get_player(scene, scene_get_turn(scene));
            list_t *num_sunk = player_get_balls_sunk(curr_player);
            // if foul (hit no balls or hit cue ball)
            if (player_get_turn_state(curr_player) == 1) {
                // Player loses turn and opponent is able to place cue
                player_set_turn_state(curr_player, 0);
                scene_switch_turn(scene);
                scene_set_state(scene, 1);
            }
            // if ball not sunk or sunk other player's ball
            else if (player_get_turn_state(curr_player) == 0) {
                scene_switch_turn(scene);
                scene_set_state(scene, 2);
            }
            //if ball sunk add cue back and set to firing mode
            else if (player_get_turn_state(curr_player) == 2) {
                scene_set_state(scene, 2);
            }
            //eight ball sunk
            else if (player_get_turn_state(curr_player) == 3) {
                if (list_size(num_sunk) == 8) {
                    if (strcmp(player_get_name(curr_player), "Player1") == 0) {
                        scene_set_state(scene, 4);
                    }
                    else {
                        scene_set_state(scene, 5);
                    }
                }
                else {
                    if (strcmp(player_get_name(curr_player), "Player1") == 0) {
                        scene_set_state(scene, 5);
                    }
                    else {
                        scene_set_state(scene, 4);
                    }
                }
            }
        }

    }
}

void on_mouse(int type, void *scene, double held_time) {

    int game_state = scene_get_state((scene_t *)scene);
    if (game_state == 0) {
        mouse_handle_menu(type, (scene_t *)scene, MENU_BUTTON_1, MENU_BUTTON_2);
    }
    else if (game_state == 1) {
        mouse_handle_placing(type, (ball_t *)list_get(scene_get_balls(scene), 0), scene, KITCHEN);
    }
    else if (game_state == 2) {
        if (scene_bodies(scene) == 29) {
            mouse_handle_firing(type, scene_get_body(scene, scene_bodies(scene)-1), (ball_t *)list_get(scene_get_balls(scene), 0), scene, held_time);
        }
    }
}

void on_key(char key, key_event_type_t type, double held_time, scene_t *scene) {
    if (scene_get_state(scene) == 2) {
        body_t *cue = scene_get_body(scene, scene_bodies(scene)-1);
        if (type == KEY_PRESSED && key == SPACE) {

            double angle = body_get_angle(cue);

            double dx = DISTANCE * (cos(angle));
            double dy = DISTANCE * (sin(angle));

            (held_time > 2.0) ? body_translate(cue, (vector_t){0.0, 0.0}) : body_translate(cue, (vector_t){dx, dy});
        }
        else if (type == KEY_RELEASED && key == SPACE) {
            //printf("held time: %f\n", held_time);
            double angle = body_get_angle(cue);
            double dx = -1 * VEL_SCALAR * held_time * (cos(angle));
            double dy = -1 * VEL_SCALAR * held_time * (sin(angle));

            body_set_velocity(cue, (vector_t){dx,dy});
            scene_set_state(scene, 3);

        }
    }
}

int main() {
    srand(time(0));
    // initialize sdl demo
    sdl_init((vector_t){MIN_X, MIN_Y}, (vector_t){MAX_X, MAX_Y});
    SDL_Event *event = malloc(sizeof(*event));
    assert(event != NULL);

    sdl_on_mouse((mouse_handler_t)on_mouse);
    sdl_on_key((key_handler_t)on_key);

    // initialize scene and components
    scene_t *scene = scene_init();
    assert(scene != NULL);
    scene_set_state(scene, 0);
    populate_scene(scene);
    while (!sdl_is_done(scene)) {
        //update scene
        scene_tick(scene, time_since_last_tick());
        update_game_state(scene);
        sdl_render_scene(scene);
    }
    sdl_free_images();
    scene_free(scene);
    return 0;
}
