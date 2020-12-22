#include <math.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "mouse.h"
#include "ball.h"



void mouse_handle_firing(int type, body_t *cue, ball_t *cueball, scene_t *scene, double held_time) {
    // if mouse moving, rotate cue appropriately
    if (type == SDL_MOUSEMOTION) {
        //Get mouse position
        int x, y;
        SDL_GetMouseState(&x, &y);

        double opposite = body_get_centroid(ball_get_body(cueball)).y - (double)y;
        double hypotenuse = sqrt((body_get_centroid(ball_get_body(cueball)).x - (double)x) *
                                (body_get_centroid(ball_get_body(cueball)).x - (double)x) +
                                (body_get_centroid(ball_get_body(cueball)).y - (double)y) *
                                (body_get_centroid(ball_get_body(cueball)).y - (double)y));
        double angle = (asin(opposite / hypotenuse));

        if (x > body_get_centroid(ball_get_body(cueball)).x) { //&& y < body_get_centroid(ball_get_body(cueball)).y) {
            angle = M_PI-angle;
        }

        angle += M_PI;

        body_set_rotation_about_point(cue, angle, body_get_centroid(ball_get_body(cueball)));
    }
}
 
bool mouse_within(vector_t min, vector_t max) {
    int x, y;
    SDL_GetMouseState(&x, &y);
    if (x > min.x && x < max.x && y > min.y && y < max.y) {
        return true;
    }
    return false;
}

void mouse_handle_placing (int type, ball_t *cueball, scene_t *scene, const vector_t *area) {

    if (type == SDL_MOUSEMOTION) {
        //Get mouse position
        int x, y;
        SDL_GetMouseState(&x, &y);
        body_set_centroid(ball_get_body(cueball), (vector_t){(double)x, (double)y});
    }
    else if (type == SDL_MOUSEBUTTONUP) {
        double radius = ball_get_radius(cueball);
        vector_t min = vec_add(area[0], (vector_t){radius, radius});
        vector_t max = vec_subtract(area[1], (vector_t){0, radius});
        vector_t centroid = body_get_centroid(ball_get_body(cueball));
        if (mouse_within(min, max) && vec_within(centroid, min, max)) {
            //printf("set state\n");
            scene_set_state(scene, 2);
        }
    }
}

bool mouse_handle_button(int type, const vector_t *box) {
    if(type == SDL_MOUSEBUTTONDOWN) {
        //Get mouse position
        return mouse_within(box[0], box[1]);
    }
    return false;
}

void mouse_handle_menu (int type, scene_t *scene, const vector_t *box1, const vector_t *box2) {
    // right now both buttons go to multiplayer
    if (mouse_handle_button(type, box1)) {
        //go to game type singleplayer
        scene_set_state(scene, 1);
        printf("Button 1 clicked!\n");
    }
    else if (mouse_handle_button(type, box2)) {
        //go to game type multiplayer
        scene_set_state(scene, 1);
        printf("Button 2 clicked!\n");
    }
}
