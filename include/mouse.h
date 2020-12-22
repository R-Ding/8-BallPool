#ifndef __MOUSE_H__
#define __MOUSE_H__

#include <stdbool.h>
#include "vector.h"
#include "body.h"
#include "ball.h"
#include "sdl_wrapper.h"

void mouse_handle_firing(int type, body_t *cue, ball_t *cueball, scene_t *scene, double held_time);

void mouse_handle_placing (int type, ball_t *cueball, scene_t *scene, const vector_t *area);

void mouse_handle_menu (int type, scene_t *scene, const vector_t *box1, const vector_t *box2);

bool mouse_within(vector_t min, vector_t max);

#endif // #ifndef __MOUSE_H__
