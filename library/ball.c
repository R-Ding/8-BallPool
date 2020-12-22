#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "body.h"
#include "ball.h"

const double BALL_RADIUS = 10.5;
const int BALL_VERTICES = 50;
const double BALL_MASS = 171; //in grams 171

typedef struct ball {
    body_t *body;
    int num;
    int solid;
    //int color; maybe for colors game mode
} ball_t;

ball_t *ball_init(int number, vector_t centroid) {
    //use other constructer with no info or info_freer
    list_t *shape = make_ball_shape(BALL_RADIUS);
    rgb_color_t color = {0.0, 0.0, 0.0};
    char *num_str = malloc((int)((ceil((number + 1) / 10.0)) + 1)*sizeof(char));
    assert(num_str != NULL);
    sprintf(num_str, "%d", number);
    // can make the size of info more specific to save space
    char *info = malloc(50 * sizeof(char));
    assert(info != NULL);
    strcpy(info, "./assets/ball");
    strcat(info, num_str);
    strcat(info, ".png");
    body_t *body = body_init_with_info(shape, BALL_MASS, color, info, (free_func_t)free);
    body_set_centroid(body, centroid);
    ball_t *ball = malloc(sizeof(ball_t));
    assert(ball != NULL);
    ball->body = body;
    ball->num = number;
    if (number <= 8) {
        ball->solid = 1;
    }
    else {
        ball->solid = 0;
    }
    free(num_str);
    return ball;
}

list_t *make_ball_shape(double radius) {
    double angle = (2 * M_PI) / BALL_VERTICES;
    vector_t ref = {0, radius};
    list_t *points = list_init(BALL_VERTICES, (free_func_t)free);

    for (size_t i = 0; i < BALL_VERTICES; i++) {
        vector_t *curr = malloc(sizeof(vector_t));
        assert(curr != NULL);
        *curr = vec_rotate(ref, angle * i);
        list_add(points, curr);
    }
    return points;
}

double ball_get_radius(ball_t *ball) {
    return BALL_RADIUS;
}

double ball_body_get_radius(body_t *ball) {
    return BALL_RADIUS;
}

body_t *ball_get_body (ball_t *ball) {
    return ball->body;
}

void ball_add_body (ball_t *ball, vector_t centroid) {
    list_t *shape = make_ball_shape(BALL_RADIUS);
    rgb_color_t color = {0.0, 0.0, 0.0};
    body_t *body = body_init(shape, BALL_MASS, color);
    ball->body = body;
}

void ball_free(ball_t *ball) {
    //body_free(ball->body);
    free(ball);
}

int ball_get_num (ball_t *ball) {
    return ball->num;
}

int ball_get_solid (ball_t *ball) {
    return ball->solid;
}
