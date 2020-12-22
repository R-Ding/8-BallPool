#ifndef __BALL_H__
#define __BALL_H__

#include <stdbool.h>
#include "color.h"
#include "list.h"
#include "vector.h"
#include "body.h"

typedef struct ball ball_t;

ball_t *ball_init(int number, vector_t centroid);

list_t *make_ball_shape(double radius);

double ball_get_radius(ball_t *ball);

double ball_body_get_radius(body_t *ball);

body_t *ball_get_body (ball_t *body);

void ball_add_body (ball_t *ball, vector_t centroid);

void ball_free(ball_t *ball);

int ball_get_num (ball_t *ball);

int ball_get_solid (ball_t *ball);

#endif // #ifndef __BALL_H__
