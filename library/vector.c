#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "vector.h"

const vector_t VEC_ZERO = {0, 0};

vector_t vec_add(vector_t v1, vector_t v2) {
    return (vector_t) {v1.x + v2.x, v1.y + v2.y};
}

vector_t vec_subtract(vector_t v1, vector_t v2) {
    return (vector_t) {v1.x - v2.x, v1.y - v2.y};
}

vector_t vec_negate(vector_t v) {
    return (vector_t) {-1 * v.x, -1 * v.y};
}


vector_t vec_multiply(double scalar, vector_t v) {
    return (vector_t) {scalar * v.x, scalar * v.y};
}

double vec_dot(vector_t v1, vector_t v2) {
    return (v1.x * v2.x) + (v1.y * v2.y);
}

double vec_cross(vector_t v1, vector_t v2) {
    return (v1.x * v2.y) - (v1.y * v2.x);
}

vector_t vec_rotate(vector_t v, double angle) {
    double sine = sin(angle);
    double cosine = cos(angle);
    return (vector_t) {(v.x * cosine) - (v.y * sine), (v.x * sine) + (v.y * cosine)};
}

vector_t vec_get_normal(vector_t v) {
    return (vector_t) {-1 * v.y, v.x};
}

double vec_magnitude(vector_t v) {
    return sqrt(v.x * v.x + v.y * v.y);
}

vector_t vec_normalize(vector_t v) {
    double magnitude = vec_magnitude(v);
    return (vector_t) {v.x / magnitude, v.y / magnitude};
}

bool vec_within(vector_t v, vector_t min, vector_t max) {
    if (v.x > min.x && v.x < max.x && v.y > min.y && v.y < max.y)
    {
        return true;
    }
    return false;
}
