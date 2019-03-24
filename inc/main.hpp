// Copyright 2019 Dave Moore
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
// Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <cmath>
#include <string>
#include <iostream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "libtcod.h"

// Function Prototypes
template <typename T> auto sign(T val) -> int;
auto is_colliding (double x, double y) -> bool;
auto get_pixel(SDL_Surface* surface, int x, int y) -> Uint32;
auto load_texture(const char* file) -> SDL_Texture*;
auto load_surface(const char* file) -> SDL_Surface*;
auto crop_texture(SDL_Texture* source, int x, int y) -> SDL_Texture*;
auto initialise() -> int;
auto render() -> void;
auto update_ticks() -> void;
auto close() -> void;
auto update_world() -> void;
auto main(int argc, char* args[]) -> int;

// Types
struct Texture {
    SDL_Texture* texture;
    int width;
    int height;
};
