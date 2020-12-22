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
const int MAX_Y = 750;
const int MAX_X = MAX_Y;
const double OBJ_SPACING = MAX_X / 10.0;
const double OBJ_RADIUS = (OBJ_SPACING * 0.85)/2;
const double OBJ_MASS = 1.0;
const int OBJ_VERTICES = 1000;
const int BULLET_Y = OBJ_RADIUS/6;
const int BULLET_X = BULLET_Y/2;
const int BULLET_VERTICES = 4;
const double ENEMY_ANGLE = M_PI/3;
const vector_t PLAYER_VEL = {MAX_X/4, 0};
const vector_t ENEMY_VEL = {MAX_X/10, 0};
const vector_t BULLET_VEL = {0, MAX_Y/3};
const int NUM_ENEMIES_PER_ROW = (int) (MAX_X / OBJ_SPACING);
const int NUM_ROWS = 3;
const rgb_color_t GREEN = {0, 0.835, 0.0836};
const rgb_color_t GREY = {0.545, 0.545, 0.545};
const size_t PLAYER_INDEX = 0;
const size_t OBJ_CENTER_INDEX = 0;
const double SHOOT_TIME = 1.0;
enum OBJ_TYPES {
    PLAYER = 0,
    ENEMY = 1,
    PLAYER_BULLET = 2,
    ENEMY_BULLET = 3
};
bool game_over = false;
double fire_time;

list_t *generate_player_points() {
    // ensure that valid number of vertices and radius is passed
    assert(OBJ_VERTICES >= 2);
    assert(OBJ_RADIUS >= 0);

    double angle = (2*M_PI)/OBJ_VERTICES;
    list_t *points = list_init(OBJ_VERTICES, (free_func_t)free);

    for (size_t i = 0; i < OBJ_VERTICES; i++) {
        vector_t *curr = malloc(sizeof(vector_t));
        assert(curr != NULL);
        curr->x = OBJ_RADIUS * cos(angle*i);
        curr->y = OBJ_RADIUS * sin(angle*i) / 2;
        list_add(points, curr);
    }
    return points;
}

list_t *generate_enemy_points() {
    // ensure that valid number of vertices and radius is passed
    assert(OBJ_VERTICES >= 2);
    assert(OBJ_RADIUS >= 0);

    // the angle of rotation between each point on the circle
    double angle = (2 * ENEMY_ANGLE) / OBJ_VERTICES;
    vector_t ref = {0, OBJ_RADIUS};

    // rotating reference vector to mouth_angle_radians/2 above the x-axis
    ref = vec_rotate(ref, -1*ENEMY_ANGLE);
    list_t *points = list_init(OBJ_VERTICES + 1, (free_func_t)free);

    for (size_t i = 0; i <= OBJ_VERTICES; i++) {
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

list_t *generate_bullet_points() {
    list_t *points = list_init(BULLET_VERTICES, (free_func_t)free);
    vector_t *v1 = malloc(sizeof(vector_t));
    assert (v1 != 0);
    v1[0] = (vector_t){BULLET_X, BULLET_Y};
    list_add(points, v1);

    vector_t *v2 = malloc(sizeof(vector_t));
    assert (v2 != 0);
    v2[0] = (vector_t){-1*BULLET_X, BULLET_Y};
    list_add(points, v2);

    vector_t *v3 = malloc(sizeof(vector_t));
    assert (v3 != 0);
    v3[0] = (vector_t){-1*BULLET_X, -1*BULLET_Y};
    list_add(points, v3);

    vector_t *v4 = malloc(sizeof(vector_t));
    assert (v4 != 0);
    v4[0] = (vector_t){BULLET_X, -1*BULLET_Y};
    list_add(points, v4);
    return points;
}

void scene_add_player(scene_t *scene, vector_t coords) {
    list_t *points = generate_player_points();
    int *val = malloc(sizeof(int));
    val[0] = PLAYER;
    body_t *player = body_init_with_info(points, OBJ_MASS, GREEN, val, (free_func_t)free);
    body_set_velocity(player, VEC_ZERO);
    body_translate(player, coords);
    scene_add_body(scene, player);
}

void scene_add_enemy(scene_t *scene, vector_t coords) {
    list_t *points = generate_enemy_points();
    int *val = malloc(sizeof(int));
    val[0] = ENEMY;
    body_t *enemy = body_init_with_info(points, OBJ_MASS, GREY, val, (free_func_t)free);
    body_set_velocity(enemy, ENEMY_VEL);
    body_translate(enemy, coords);
    scene_add_body(scene, enemy);
}

void scene_add_bullet(scene_t *scene, body_t *origin) {
    list_t *points = generate_bullet_points();
    int bullet_type = ((int*)body_get_info(origin))[0] == PLAYER ? PLAYER_BULLET : ENEMY_BULLET;
    int *type = malloc(sizeof(int));
    type[0] = bullet_type;
    rgb_color_t col = bullet_type == PLAYER_BULLET ? GREEN : GREY;
    vector_t vel = vec_multiply(bullet_type == PLAYER_BULLET ? 1 : -1, BULLET_VEL);
    body_t *bullet = body_init_with_info(points, OBJ_MASS, col, type, (free_func_t)free);
    body_set_velocity(bullet, vel);
    body_set_centroid(bullet, body_get_centroid(origin));
    for(size_t i = 0; i < scene_bodies(scene); i++) {
        body_t *body = scene_get_body(scene, i);
        int body_type = ((int*)body_get_info(body))[0];
        if (body_type == PLAYER && bullet_type == ENEMY_BULLET) {
            create_destructive_collision(scene, body, bullet);
        }
        if (body_type == ENEMY && bullet_type == PLAYER_BULLET) {
            create_destructive_collision(scene, body, bullet);
        }
    }
    scene_add_body(scene, bullet);
}

void populate_scene(scene_t *scene) {
    scene_add_player(scene, (vector_t) {MAX_X/2, OBJ_SPACING/2});
    for (int i = 0; i < NUM_ENEMIES_PER_ROW; i++) {
        for (int j = 0; j < NUM_ROWS; j++) {
            scene_add_enemy(scene, (vector_t) {OBJ_SPACING*(i+0.5), MAX_Y - OBJ_SPACING*(j+0.5)});
        }
    }
}

void update_player_pos(body_t *player) {
    list_t *shape = body_get_shape(player);
    double center = ((vector_t*)list_get(shape, OBJ_CENTER_INDEX))[0].x;
    double dist;
    if (body_get_velocity(player).x < 0) {
        dist = center - OBJ_RADIUS - MIN_X;
        if (dist < 0) {
            body_set_velocity(player, VEC_ZERO);
            body_translate(player, (vector_t) {-1*dist, 0});
        }
    }
    else if (body_get_velocity(player).x > 0) {
        dist = MAX_X - center - OBJ_RADIUS;
        if (dist < 0) {
            body_set_velocity(player, VEC_ZERO);
            body_translate(player, (vector_t) {dist, 0});
        }
    }
    list_free(shape);
}

bool update_enemy_pos(body_t *enemy) {
    list_t *shape = body_get_shape(enemy);
    double center = ((vector_t*)list_get(shape, OBJ_CENTER_INDEX))[0].x;
    double dist1 = MAX_X - center - OBJ_RADIUS;
    double dist2 = center - OBJ_RADIUS - MIN_X;
    if (dist1 <= 0) {
        body_translate(enemy, (vector_t) {dist1, -3 * OBJ_SPACING});
        body_set_velocity(enemy, vec_multiply(-1.0, body_get_velocity(enemy)));
    }
    else if (dist2 <= 0) {
        body_translate(enemy, (vector_t) {-1*dist2, -3 * OBJ_SPACING});
        body_set_velocity(enemy, vec_multiply(-1.0, body_get_velocity(enemy)));
    }
    list_free(shape);
    return (body_get_centroid(enemy).y + OBJ_RADIUS <= 0);
}

void update_bullet_pos(scene_t *scene, body_t *bullet) {
    vector_t cent = body_get_centroid(bullet);
    if (cent.y - BULLET_Y > MAX_Y || cent.y + BULLET_Y < MIN_Y) {
        body_remove(bullet);
    }
}

body_t *nearest_enemy(scene_t *scene) {
    body_t *nearest = scene_get_body(scene, 0);
    vector_t player_cent = body_get_centroid(scene_get_body(scene, 0));
    double min = DBL_MAX;
    for (size_t i = 1; i < scene_bodies(scene); i++) {
        vector_t enemy_cent = body_get_centroid(scene_get_body(scene, i));
        if (fabs(player_cent.x - enemy_cent.x) <= min && ((int*)body_get_info(scene_get_body(scene, i)))[0] == ENEMY) {
            nearest = scene_get_body(scene, i);
            min = fabs(player_cent.x - enemy_cent.x);
        }
    }
    return nearest;
}

bool update_pos(scene_t *scene) {
    int enemy_off_screen = 0;
    for (size_t i = 0; i < scene_bodies(scene); i++) {
        body_t *body = scene_get_body(scene, i);
        switch(((int*)body_get_info(body))[0]) {
            case PLAYER:
                update_player_pos(body);
                break;
            case ENEMY:
                if (update_enemy_pos(body)) {
                    enemy_off_screen = 1;
                }
                break;
            case PLAYER_BULLET:
                update_bullet_pos(scene, body);
            case ENEMY_BULLET:
                update_bullet_pos(scene, body);
            default:
                break;
        }
    }
    return enemy_off_screen;
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
        else if (key == SPACE && fire_time >= SHOOT_TIME) {
            scene_add_bullet((scene_t *)scene, player);
            fire_time = 0;
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

    double t = 0;
    fire_time = 0;
    double dt;
    while (!sdl_is_done(scene) && !game_over) {
        //increment time
        dt = time_since_last_tick();
        t += dt;
        fire_time += dt;
        //make nearest enemy shoot if past shoot time
        if (t >= SHOOT_TIME) {
            scene_add_bullet(scene, nearest_enemy(scene));
            t = 0.0;
        }
        //update scene
        scene_tick(scene, dt);
        if (((int*)body_get_info(scene_get_body(scene, 0)))[0] != PLAYER) {
            game_over = true;
        }
        if (update_pos(scene)) {
            game_over = true;
        }
        sdl_render_scene(scene);
    }
    scene_free(scene);
    return 0;
}
