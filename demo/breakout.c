#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include "forces.h"
#include "scene.h"
#include "sdl_wrapper.h"
#include "collision.h"

const int MIN_Y = 0;
const int MIN_X = 0;
const int MAX_Y = 1000;
const int MAX_X = 2*MAX_Y;
const int OBJ_SPACING = 10;
const int NUM_COLS = 10;
const int NUM_ROWS = 3;
const int BOX_WIDTH = (MAX_Y - OBJ_SPACING*(NUM_COLS+1)) / NUM_COLS;
const int BOX_HEIGHT = BOX_WIDTH / 3;
const int BALL_RADIUS = BOX_HEIGHT / 2;
const int BALL_VERTICES = 1000;
const int BOX_VERTICES = 4;
const vector_t BALL_BASE_VEL = {60, 100};
const vector_t PLAYER_VEL = {220, 0};
const double BOX_MASS = INFINITY;
const double BALL_MASS = 10.0;
const rgb_color_t PINK = {1.0,0.75,0.79};
const rgb_color_t RED = {1,0,0};
const rgb_color_t ORANGE = {0.99,0.49,0.05};
const rgb_color_t YELLOW = {0.99,0.78,0.05};
const rgb_color_t GREEN = {0,1,0};
const rgb_color_t BABY_BLUE = {0.05,0.96,0.99};
const rgb_color_t BLUE = {0.05,0.53,0.99};
const rgb_color_t DARK_BLUE = {0.05,0.17,0.99};
const rgb_color_t INDIGO = {0.15,0,0.4};
const rgb_color_t VIOLET = {0.56,0,0.70};
const rgb_color_t BLACK = {0,0,0};
const int PLAYER_INDEX = 4;
const int BALL_INDEX = 5;
//const int TOTAL_BODIES = 6 + NUM_COLS*NUM_ROWS;

list_t *generate_box_points(vector_t dimensions) {
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

list_t *generate_circle_points() {
    double angle = (2 * M_PI) / BALL_VERTICES;
    vector_t ref = {0, BALL_RADIUS};
    list_t *points = list_init(BALL_VERTICES, (free_func_t)free);

    for (size_t i = 0; i < BALL_VERTICES; i++) {
        vector_t *curr = malloc(sizeof(vector_t));
        assert(curr != NULL);
        *curr = vec_rotate(ref, angle * i);
        list_add(points, curr);
    }
    return points;
}

void scene_add_box(scene_t *scene, vector_t coords, rgb_color_t col) {
    list_t *points = generate_box_points((vector_t){BOX_WIDTH, BOX_HEIGHT});
    body_t *box = body_init(points, BOX_MASS, col);
    body_translate(box, coords);
    scene_add_body(scene, box);
}

void scene_add_wall(scene_t *scene, vector_t coords, vector_t dimensions, rgb_color_t col) {
    list_t *points = generate_box_points(dimensions);
    body_t *wall = body_init(points, BOX_MASS, col);
    body_translate(wall, coords);
    scene_add_body(scene, wall);
}

void populate_scene(scene_t *scene) {
    //add walls
    vector_t side_wall_dims = {MAX_Y/2, MAX_Y};
    vector_t top_wall_dims = {MAX_Y, MAX_Y/2};
    scene_add_wall(scene, (vector_t) {MAX_Y/4, MAX_Y/2}, side_wall_dims, BLACK);
    scene_add_wall(scene, (vector_t) {7*MAX_Y/4, MAX_Y/2}, side_wall_dims, BLACK);
    scene_add_wall(scene, (vector_t) {MAX_Y, 5*MAX_Y/4}, top_wall_dims, BLACK);
    scene_add_wall(scene, (vector_t) {MAX_Y, -MAX_Y/4}, top_wall_dims, BLACK);
    //add player
    vector_t start_coords = {MAX_Y, OBJ_SPACING + BOX_HEIGHT/2};
    //list_t *points = generate_box_points((vector_t){BOX_WIDTH, BOX_HEIGHT});
    scene_add_box(scene, start_coords, RED);
    //add ball
    start_coords = vec_add(start_coords, (vector_t) {0, OBJ_SPACING+BOX_HEIGHT+BALL_RADIUS});
    list_t *ball_points = generate_circle_points();
    body_t *ball = body_init(ball_points, BALL_MASS, RED);
    scene_add_body(scene, ball);
    body_set_centroid(ball, start_coords);
    body_set_velocity(ball, BALL_BASE_VEL);
    for (size_t i = 0; i <= PLAYER_INDEX; i++) {
        body_t *wall = scene_get_body(scene, i);

        create_physics_collision(scene, 1.0, ball, wall);
    }

    //add blocks
    rgb_color_t colors[10] = {PINK, RED, ORANGE, YELLOW, GREEN,
                                    BABY_BLUE, BLUE, DARK_BLUE, INDIGO, VIOLET};
    for (size_t r = 0; r < NUM_ROWS; r++) {
        for (size_t c = 0; c < NUM_COLS; c++) {
            double x = MAX_Y / 2 + OBJ_SPACING * ((double)c+1.0) + BOX_WIDTH*((double)c+0.5);
            double y = MAX_Y - (OBJ_SPACING * ((double)r+1.0) + BOX_HEIGHT*((double)r+0.5));
            scene_add_box(scene, (vector_t) {x, y}, colors[c]);
            //add force between box and ball
            body_t *ball = scene_get_body(scene, BALL_INDEX);
            int box_index = 6 + r*NUM_COLS + c;
            body_t *box = scene_get_body(scene, box_index);
            list_t *for_aux = list_init(1, NULL);
            list_add(for_aux, box);
            create_physics_collision_with_removal(scene, 1.0, ball, box, for_aux, 1.15);
        }
    }
}

bool update_pos(scene_t *scene) {
    //return any gameover state
    body_t *ball = scene_get_body(scene, BALL_INDEX);
    body_t *player = scene_get_body(scene, PLAYER_INDEX);
    vector_t centroid = body_get_centroid(player);
    double d1 = centroid.x - BOX_WIDTH / 2 - MAX_Y / 2;
    double d2 = 3 * MAX_Y / 2 - (centroid.x + BOX_WIDTH / 2);
    if (d1 < 0) {
        body_translate(player, (vector_t) {-1*d1, 0});
        body_set_velocity(player, (vector_t) {0.0, 0.0});
    }
    else if (d2 < 0) {
        body_translate(player, (vector_t) {d2, 0});
        body_set_velocity(player, (vector_t) {0.0, 0.0});
    }
    //list_t *player_points = body_get_shape(player);
    //collision_info_t collision_info = find_collision(ball_points, player_points);
    return (body_get_centroid(ball).y-BALL_RADIUS <= MIN_Y);
}

void on_key(char key, key_event_type_t type, double held_time, void *scene) {
    body_t *player = scene_get_body((scene_t *)scene, PLAYER_INDEX);
    if (type == KEY_PRESSED) {
        if (key == LEFT_ARROW) {
            body_set_velocity(player, vec_multiply(-1, PLAYER_VEL));
        }
        else if (key == RIGHT_ARROW) {
            body_set_velocity(player, PLAYER_VEL);
        }
    }
    else if (type == KEY_RELEASED) {
        if (key == LEFT_ARROW || key == RIGHT_ARROW) {
            body_set_velocity(player, VEC_ZERO);
        }
    }
}

int main() {
    srand(time(0));
    // initialize sdl demo
    sdl_init((vector_t){MIN_X, MIN_Y}, (vector_t){MAX_X, MAX_Y});
    sdl_on_key((key_handler_t)on_key);
    // initialize scene and components
    scene_t *scene = scene_init();
    assert(scene != NULL);
    populate_scene(scene);
    while (!sdl_is_done(scene) && !update_pos(scene)) {
        //update scene
        scene_tick(scene, time_since_last_tick());
        sdl_render_scene(scene);
    }
    scene_free(scene);
    return 0;
}
