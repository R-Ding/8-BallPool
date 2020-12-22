#ifndef __STAR_H__
#define __STAR_H__

#include <stddef.h>
#include "polygon.h"
#include "color.h"

typedef struct star {
    list_t *points;
    rgb_color_t col;
    vector_t *vel;
} star_t;

/**
 * Allocates memory for a new star with a given number of vertices
 *     as well as a given radius.
 * Randomly chooses the color of the star.
 * Asserts that the required memory was allocated.
 *
 * @param vertices number of sides of the new star
 * @param radius radius of the new star
 * @param vel_x x velocity of the star
 * @param vel_y y velocity of the star
 * @return a pointer to the newly allocated star
 */
star_t *star_init(int vertices, int radius, double vel_x, double vel_y);

/**
 * Frees the allocated memory for a given star
 *
 * @param star a pointer to the star to free
 */
void star_free(star_t *star);

#endif // #ifndef __STAR_H__
