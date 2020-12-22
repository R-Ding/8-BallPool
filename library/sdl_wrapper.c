#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "sdl_wrapper.h"

const char WINDOW_TITLE[] = "Pageboy Pool";
const int WINDOW_WIDTH = 1000;
const int WINDOW_HEIGHT = 500;
const double MS_PER_S = 1e3;

/**
 * The coordinate at the center of the screen.
 */
vector_t center;
/**
 * The coordinate difference from the center to the top right corner.
 */
vector_t max_diff;
/**
 * The SDL window where the scene is rendered.
 */
SDL_Window *window;
/**
 * The renderer used to draw the scene.
 */
SDL_Renderer *renderer;
/**
 * A list of textures to be used by the renderer.
 */
 list_t *textures;
/**
* A list of surfaces to be used by the renderer.
*/
list_t *surfaces;
/**
 * The keypress handler, or NULL if none has been configured.
 */
key_handler_t key_handler = NULL;

mouse_handler_t mouse_handler = NULL;
/**
 * SDL's timestamp when a key was last pressed or released.
 * Used to mesasure how long a key has been held.
 */
uint32_t key_start_timestamp;

uint32_t mouse_down_start_timestamp;
/**
 * The value of clock() when time_since_last_tick() was last called.
 * Initially 0.
 */
clock_t last_clock = 0;

//You already know what it is
TTF_Font *comic_sans;

/** Computes the center of the window in pixel coordinates */
vector_t get_window_center(void) {
    int *width = malloc(sizeof(*width)),
        *height = malloc(sizeof(*height));
    assert(width != NULL);
    assert(height != NULL);
    SDL_GetWindowSize(window, width, height);
    vector_t dimensions = {.x = *width, .y = *height};
    free(width);
    free(height);
    return vec_multiply(0.5, dimensions);
}

/**
 * Computes the scaling factor between scene coordinates and pixel coordinates.
 * The scene is scaled by the same factor in the x and y dimensions,
 * chosen to maximize the size of the scene while keeping it in the window.
 */
double get_scene_scale(vector_t window_center) {
    // Scale scene so it fits entirely in the window
    double x_scale = window_center.x / max_diff.x,
           y_scale = window_center.y / max_diff.y;
    return x_scale < y_scale ? x_scale : y_scale;
}

/** Maps a scene coordinate to a window coordinate */
vector_t get_window_position(vector_t scene_pos, vector_t window_center) {
    // Scale scene coordinates by the scaling factor
    // and map the center of the scene to the center of the window
    vector_t scene_center_offset = vec_subtract(scene_pos, center);
    double scale = get_scene_scale(window_center);
    vector_t pixel_center_offset = vec_multiply(scale, scene_center_offset);
    vector_t pixel = {
        .x = round(window_center.x + pixel_center_offset.x),
        // Flip y axis since positive y is down on the screen
        .y = round(window_center.y - pixel_center_offset.y)
    };
    return pixel;
}

/**
 * Converts an SDL key code to a char.
 * 7-bit ASCII characters are just returned
 * and arrow keys are given special character codes.
 */
char get_keycode(SDL_Keycode key) {
    switch (key) {
        case SDLK_LEFT:  return LEFT_ARROW;
        case SDLK_UP:    return UP_ARROW;
        case SDLK_RIGHT: return RIGHT_ARROW;
        case SDLK_DOWN:  return DOWN_ARROW;
        case SDLK_SPACE: return SPACE;
        default:
            // Only process 7-bit ASCII characters
            return key == (SDL_Keycode) (char) key ? key : '\0';
    }
}

void sdl_init(vector_t min, vector_t max) {
    // Check parameters
    assert(min.x < max.x);
    assert(min.y < max.y);

    center = vec_multiply(0.5, vec_add(min, max));
    max_diff = vec_subtract(max, center);
    SDL_Init(SDL_INIT_EVERYTHING);
    if (TTF_Init() == -1)
    {
        printf("TTF_Init: %s\n", TTF_GetError());
        exit(2);
    }
    window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_RESIZABLE
    );
    renderer = SDL_CreateRenderer(window, -1, 0);
    textures = list_init(15, NULL);
    surfaces = list_init(15, NULL);
    comic_sans = TTF_OpenFont("./assets/comic.ttf", 24);
    if (!comic_sans)
    {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
        // handle error
    }
}

bool sdl_is_done(scene_t *scene) {
    SDL_Event *event = malloc(sizeof(*event));
    assert(event != NULL);
    while (SDL_PollEvent(event)) {
        switch (event->type) {
            case SDL_QUIT:
                free(event);
                return true;
            case SDL_KEYDOWN: {
                if (key_handler == NULL) break;
                char key = get_keycode(event->key.keysym.sym);
                if (key == '\0') break;

                uint32_t timestamp = event->key.timestamp;
                if (!event->key.repeat) {
                    key_start_timestamp = timestamp;
                }
                key_event_type_t type =
                    event->type == SDL_KEYDOWN ? KEY_PRESSED : KEY_RELEASED;
                double held_time = (timestamp - key_start_timestamp) / MS_PER_S;
                key_handler(key, type, held_time, scene);
                break;
            }
            case SDL_KEYUP: {
                // Skip the keypress if no handler is configured
                // or an unrecognized key was pressed

                if (key_handler == NULL) break;
                char key = get_keycode(event->key.keysym.sym);
                if (key == '\0') break;

                uint32_t timestamp = event->key.timestamp;
                key_event_type_t type =
                    event->type == SDL_KEYDOWN ? KEY_PRESSED : KEY_RELEASED;
                double held_time = (timestamp - key_start_timestamp) / MS_PER_S;
                key_handler(key, type, held_time, scene);
                break;
            }
            case SDL_MOUSEMOTION:
                mouse_handler(SDL_MOUSEMOTION, scene, 0.0);
                break;
            case SDL_MOUSEBUTTONUP:
            {
                uint32_t timestamp = event->button.timestamp;

                double held_time = (timestamp - mouse_down_start_timestamp) / MS_PER_S;
                //printf("mouse case up\n");
                mouse_down_start_timestamp = 0;
                mouse_handler(SDL_MOUSEBUTTONUP, scene, held_time);
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
            {
                uint32_t timestamp = event->button.timestamp;
                double held_time = (timestamp - mouse_down_start_timestamp) / MS_PER_S;

                if (mouse_down_start_timestamp == 0) {
                    mouse_down_start_timestamp = timestamp;
                }
                //printf("mouse case down\n");
                mouse_handler(SDL_MOUSEBUTTONDOWN, scene, held_time);
                break;
            }
        }
    }
    free(event);
    return false;
}

void sdl_clear(void) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
}

void sdl_draw_polygon(list_t *points, rgb_color_t color) {
    // Check parameters
    size_t n = list_size(points);
    assert(n >= 3);
    assert(0 <= color.r && color.r <= 1);
    assert(0 <= color.g && color.g <= 1);
    assert(0 <= color.b && color.b <= 1);

    vector_t window_center = get_window_center();

    // Convert each vertex to a point on screen
    int16_t *x_points = malloc(sizeof(*x_points) * n),
            *y_points = malloc(sizeof(*y_points) * n);
    assert(x_points != NULL);
    assert(y_points != NULL);
    for (size_t i = 0; i < n; i++) {
        vector_t *vertex = list_get(points, i);
        vector_t pixel = get_window_position(*vertex, window_center);
        x_points[i] = pixel.x;
        y_points[i] = pixel.y;
    }

    // Draw polygon with the given color
    filledPolygonRGBA(
        renderer,
        x_points, y_points, n,
        color.r * 255, color.g * 255, color.b * 255, 255
    );
    free(x_points);
    free(y_points);
}

//coords is center of image
void sdl_draw_image(const char *file, vector_t coords) {
    SDL_Texture *img = IMG_LoadTexture(renderer, file);
    assert(img != NULL);
    int w, h;
    SDL_QueryTexture(img, NULL, NULL, &w, &h);
    SDL_Rect rect;
    rect.w = w;
    rect.h = h;
    rect.x = coords.x - w/2;
    rect.y = coords.y - h/2;
    SDL_RenderCopy(renderer, img, NULL, &rect);
    list_add(textures, img);
}

void sdl_draw_cue(const char *file, vector_t coords, double angle, vector_t center) {
    SDL_Texture *img = IMG_LoadTexture(renderer, file);
    assert(img != NULL);
    int w, h;
    SDL_QueryTexture(img, NULL, NULL, &w, &h);
    SDL_Rect rect;
    rect.w = w;
    rect.h = h;
    rect.x = coords.x - w/2;
    rect.y = coords.y - h/2;
    double angle_deg = (double)((angle-M_PI) * 180.0) / M_PI;
    //SDL_Point pt = (SDL_Point){coords.x, coords.y};
    //printf("angle: %f\n", angle_deg);
    //printf("centroid: %d, %d\n", pt.x, pt.y);
    //angle_deg
    SDL_RenderCopyEx(renderer, img, NULL, &rect, angle_deg, NULL, SDL_FLIP_NONE);
    //printf("just drew cue\n");
    list_add(textures, img);
}

//coords is top left corner of text
void sdl_draw_text(const char *text, vector_t coords) {
    SDL_Color black = {0,0,0};
    SDL_Surface *surface = TTF_RenderText_Blended(comic_sans, text, black);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    int w, h;
    SDL_QueryTexture(texture, NULL, NULL, &w, &h);
    SDL_Rect rect;
    rect.w = w;
    rect.h = h;
    rect.x = coords.x;
    rect.y = coords.y;
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    assert(surface != NULL);
    assert(texture != NULL);
    list_add(textures, texture);
    list_add(surfaces, surface);
}

//coords is top left corner
void sdl_draw_player_info(player_t *player, vector_t coords)
{
    const char *name = player_get_name(player);
    //printf("NAME: %s\n", name);
    const char *info = player_get_info(player);
    list_t *sunk = player_get_balls_sunk(player);
    sdl_draw_text(name, coords);
    sdl_draw_text(info, vec_add(coords, (vector_t){0, 35}));
    sdl_draw_text("Balls sunk:", vec_add(coords, (vector_t){0, 70}));
    //printf("Number of balls sunk: %zu\n", list_size(sunk));
    for (size_t i = 0; i < list_size(sunk); i++)
    {
        ball_t *ball = (ball_t *)list_get(sunk, i);
        //printf("Ball #%d\n", ball_get_num(ball));
        body_t *body = ball_get_body(ball);
        double radius = ball_get_radius(ball);
        // add spacing as an argument to function
        double x_addition = i * (2.0 * radius + 3);
        //printf("x_addition: %f\n", x_addition);
        body_set_velocity(body, (vector_t){0, 0});
        body_reset_impulse(body);
        body_set_centroid(body, vec_add(coords, (vector_t){x_addition + 10, 127}));
    }
}

void sdl_show(void) {
    // Draw boundary lines
    vector_t window_center = get_window_center();
    vector_t max = vec_add(center, max_diff),
             min = vec_subtract(center, max_diff);
    vector_t max_pixel = get_window_position(max, window_center),
             min_pixel = get_window_position(min, window_center);
    SDL_Rect *boundary = malloc(sizeof(*boundary));
    boundary->x = min_pixel.x;
    boundary->y = max_pixel.y;
    boundary->w = max_pixel.x - min_pixel.x;
    boundary->h = min_pixel.y - max_pixel.y;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, boundary);
    free(boundary);

    SDL_RenderPresent(renderer);
}

void sdl_clear_images() {
    //destroy surfaces
    for (size_t i = 0; i < list_size(surfaces); i++) {
        SDL_FreeSurface((SDL_Surface *) list_remove(surfaces, i));
    }
    //destroy textures
    for (size_t i = 0; i < list_size(textures); i++) {
        SDL_DestroyTexture((SDL_Texture *) list_remove(textures, i));
    }
}

void sdl_render_scene(scene_t *scene) {
    sdl_clear();
    size_t body_count = scene_bodies(scene);
    //draw background based on game state
    if (scene_get_state(scene) == MENU) {
        sdl_draw_image("./assets/menu_background.png", get_window_center());
    }
    else if (scene_get_state(scene) == 4) {
        sdl_draw_image("./assets/game_over_1.png", get_window_center());
    }
    else if (scene_get_state(scene) == 5) {
        sdl_draw_image("./assets/game_over_2.png", get_window_center());
    }
    else {
        sdl_draw_image("./assets/game_background.png", get_window_center());
        //draw balls
        vector_t cue_center;
        for (size_t i = 0; i < 16; i++) {
            body_t *body = ball_get_body(list_get(scene_get_balls(scene), i));
            vector_t centroid = body_get_centroid(body);
            if (i == 0) {
                cue_center = centroid;
            }
            char *info = (char *)body_get_info(body);
            sdl_draw_image(info, centroid);
        }
        //draw non-ball objects as either polygons or images based on body info
        for (size_t i = 0; i < body_count; i++) {
            body_t *body = scene_get_body(scene, i);
            char *info = (char *)body_get_info(body);
            if (info == NULL) {
                /*list_t *shape = body_get_shape(body);
                sdl_draw_polygon(shape, body_get_color(body));
                list_free(shape);*/
            }
            else if (strcmp(info, "./assets/cue.png") == 0) {
                char *filename = (char *) body_get_info(body);
                vector_t coords = body_get_centroid(body);
                sdl_draw_cue(filename, coords, body_get_angle(body), cue_center);
            }
            else if (strcmp(info, "test") != 0) {
                char *filename = (char *) body_get_info(body);
                vector_t coords = body_get_centroid(body);
                sdl_draw_image(filename, coords);
            }
        }
        int turn = scene_get_turn(scene);
        int players = list_size(scene_get_players(scene));
        //render player 1 text
        sdl_draw_player_info(scene_get_player(scene, turn), (vector_t) {42, 69 - 2});
        //render player 2 text
        int turn2 = (turn + 1) % players;
        sdl_draw_player_info(scene_get_player(scene, turn2), (vector_t) {42, 275});
        //clear textures and surfaces
    }
    sdl_clear_images();
    sdl_show();
}

void sdl_on_key(key_handler_t handler) {
    key_handler = handler;
}

void sdl_on_mouse(mouse_handler_t handler) {
    mouse_handler = handler;
}

double time_since_last_tick(void) {
    clock_t now = clock();
    double difference = last_clock
        ? (double) (now - last_clock) / CLOCKS_PER_SEC
        : 0.0; // return 0 the first time this is called
    last_clock = now;
    return difference;
}

void sdl_free_images() {
    list_free(surfaces);
    list_free(textures);
}
