
#include <stdio.h>
#include <math.h>
#include <string>
#include <iostream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "libtcod.h"

// Function Prototypes
auto is_colliding (double x, double y) -> bool;
auto get_pixel(SDL_Surface* surface, int x, int y) -> Uint32;
template <typename T> auto sign(T val) -> int;
auto load_texture(const char* file) -> SDL_Texture*;
auto load_surface(const char* file) -> SDL_Surface*;
auto crop_texture(SDL_Texture* source, int x, int y) -> SDL_Texture*;
auto initialise() -> int;
auto render() -> void;

// Types
struct Texture {
    SDL_Texture* texture;
    int width;
    int height;
};
