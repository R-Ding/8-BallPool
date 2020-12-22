#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include "collision.h"
#include "ball.h"
#include <assert.h>

vector_t project_shape(list_t *shape, vector_t axis_before_normalized, list_t *axes) {
  vector_t axis = vec_normalize(axis_before_normalized);
  double min = vec_dot(axis, ((vector_t*)list_get(shape, 0))[0]);
  double max = min;
  for (size_t i = 1; i < list_size(shape); i++) {
    double p = vec_dot(axis, ((vector_t*)list_get(shape, i))[0]);
    if (p < min) {
      min = p;
    }
    else if (p > max) {
      max = p;
    }
  }
  vector_t proj = {min, max};
  return proj;
}

double get_overlap(vector_t p1, vector_t p2) {
    if (p1.x < p2.y && p1.x > p2.x) {
        return (p2.y - p1.x);
    }
    else {
        return (p1.y - p2.x);
    }
}

double check_overlap(collision_info_t *collision, list_t *shape1, list_t *shape2, list_t *axes, double prev_overlap) {
    bool collided = true;
    double overlap = prev_overlap;
    vector_t smallest_axis = collision->axis;
    for (size_t i = 0; i < list_size(axes); i++) {
      vector_t axis = ((vector_t*)list_get(axes, i))[0];
      vector_t p1 = project_shape(shape1, axis, axes);
      vector_t p2 = project_shape(shape2, axis, axes);
      if (!((p1.x < p2.y && p1.x > p2.x) || (p2.x < p1.y && p2.x > p1.x))) {
        collided = false;
        break;
      }
      else {
          double o = get_overlap(p1, p2);
          if (o < overlap) {
              overlap = o;
              smallest_axis = axis;
          }
      }
    }
    collision->collided = collided;
    if (collision->collided) {
      collision->axis = vec_normalize(smallest_axis);
    }
    return overlap;
}

list_t *get_axes(list_t *shape) {
    list_t *axes = list_init(list_size(shape), (free_func_t)free);
    for (size_t i = 0; i < list_size(shape); i++) {
      vector_t p1 = ((vector_t *)list_get(shape, i))[0];
      vector_t p2 = ((vector_t *)list_get(shape, i + 1 == list_size(shape) ? 0 : i + 1))[0];
      vector_t edge = vec_subtract(p2, p1);
      vector_t normal = vec_get_normal(edge);
      vector_t *norm = malloc(sizeof(vector_t));
      assert(norm != NULL);
      norm[0] = normal;
      list_add(axes, norm);
    }
    return axes;
}

collision_info_t find_collision(list_t *shape1, list_t *shape2) {
  collision_info_t *collision = malloc(sizeof(collision_info_t));
  collision->collided = true;
  list_t *axes1 = get_axes(shape1);
  list_t *axes2 = get_axes(shape2);
  double overlap = check_overlap(collision, shape1, shape2, axes1, DBL_MAX);
  check_overlap(collision, shape1, shape2, axes2, overlap);
  list_free(axes1);
  list_free(axes2);
  return *collision;
}

collision_info_t find_collision_balls(body_t *ball1, body_t *ball2) {
  collision_info_t *collision = malloc(sizeof(collision_info_t));
  vector_t b1 = body_get_centroid(ball1);
  //printf("Centroid of ball1: (%f, %f)\n", b1.x, b1.y);
  vector_t b2 = body_get_centroid(ball2);
  //printf("Centroid of ball2: (%f, %f)\n", b2.x, b2.y);
  vector_t axis = vec_subtract(b2, b1);
  double dist = vec_magnitude(axis);
  //printf("Distance between balls: %f\n", dist);
  if (dist > (ball_body_get_radius(ball1) + ball_body_get_radius(ball2)))
  {
    collision->collided = false;
  }
  else {
    //printf("Ball collision!\n");
    collision->collided = true;
    collision->axis = vec_normalize(axis);
  }
  return *collision;
}
