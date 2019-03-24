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

#include "main.hpp"

// Surface Resolution (Pixels)
const int SURFACE_WIDTH = {400};
const int SURFACE_HEIGHT = {300};

// Libtcod Window Size (Characters)
const int WINDOW_WIDTH = {180};
const int WINDOW_HEIGHT = {100};

// FOV
const double FOV = {1.30899694};			// rad (75 deg)
const double MIN_DIST = {0.3};				// square sides / s
const double DARKEST_DIST = {6.0};			// any greater distance will not be darker
const int DARKNESS_ALPHA = {40};			// minimal lighting

// World Size
const int GRID_WIDTH = {18};
const int GRID_HEIGHT = {10};

// Graphics Data
const int TILE_WIDTH = {64};
const int TILE_HEIGHT = {64};

// Movement Data
const double DEFAULT_SPEED = {3};			// sqsides / s
const double TURN_SPEED = {M_PI}; 			// rad / s

// Fixed World Map
const int world[GRID_HEIGHT][GRID_WIDTH] =
{
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
	{ 1, 0, 2, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1 },
	{ 1, 0, 2, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1 },
	{ 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
	{ 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1 },
	{ 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1 },
	{ 1, 0, 1, 1, 0, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
	{ 1, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
};

// SDL Data
SDL_Surface* working_surface = {nullptr};
SDL_Renderer* renderer = {nullptr};
SDL_Texture* working_texture = {nullptr};
SDL_Texture* working_texture_streaming = {nullptr};
SDL_Texture* floor_texture = {nullptr};
SDL_Texture* ceiling_texture = {nullptr};

SDL_Surface* floor_surface = {nullptr};
SDL_Surface* ceiling_surface = {nullptr};
SDL_Texture* wall_textures[3] = {nullptr};

void* memory_pixels = {nullptr};

// Raycasting Data
double scr_pts [SURFACE_WIDTH] = {};		// Tangent y-coordinate for theta = 0, one for every horizontal pixel
double distortion [SURFACE_WIDTH] = {};		// Correction values for distortion


// Player Info and Movement
double player_x = {1.5};					// Initial player position
double player_y = {1.5};
double theta = {0.0};						// Initial central ray direction
int ticks = {};									// ms
int diff_ticks = {};								// ms

double speed = {0.0}; 						// sqsides/s
double turn = {0.0};  						// rad/s

// Functions
template <typename T>
auto sign(T val) -> int
{
    return (T(0) < val) - (val < T(0));
}

Uint32 get_pixel(SDL_Surface *surface, int x, int y)
{
	// Here p is the address to the pixel we want to retrieve
    int bpp = {surface->format->BytesPerPixel};
	Uint8* p = {static_cast<Uint8*>(surface->pixels + y * surface->pitch + x * bpp)};

	switch (bpp) {
	case 1:
		return *p;
		break;
	case 2:
		return *(reinterpret_cast<Uint16*>(p));
		break;
	case 3:
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
			return p[0] << 16 | p[1] << 8 | p[2];
		else
			return p[0] | p[1] << 8 | p[2] << 16;
		break;
	case 4:
		return *(reinterpret_cast<Uint32*>(p));
		break;
	default:
		return 0;
	}
}

bool is_colliding(double x, double y)
{
	int arr_x = {std::floor(x)};
	int arr_y = {GRID_HEIGHT - 1 - std::floor(y)};

	return world[arr_y][arr_x] != 0;
}

auto crop_texture(SDL_Texture* source, int x, int y) -> SDL_Texture*
{
	SDL_Texture* destination = SDL_CreateTexture(renderer, working_surface->format->format, SDL_TEXTUREACCESS_TARGET,
		TILE_WIDTH, TILE_HEIGHT);
	SDL_Rect rect = {TILE_WIDTH * x, TILE_HEIGHT * y, TILE_WIDTH, TILE_HEIGHT};
	SDL_SetRenderTarget(renderer, destination);
	SDL_RenderCopy(renderer, source, &rect, nullptr);
	SDL_SetRenderTarget(renderer, nullptr);

	return destination;
}

auto load_texture(const char* file) -> SDL_Texture*
{
	SDL_Surface* surface = {load_surface(file)};
	if (!surface)
		std::cout << "IMG_LoadPNG_RW: " << IMG_GetError() << std::endl;

	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);

	return texture;
}

auto load_surface (const char * file) -> SDL_Surface*
{
	SDL_RWops* io = {SDL_RWFromFile(file, "rb")};
	SDL_Surface* loaded = {IMG_LoadPNG_RW(io)};
	SDL_Surface* conv = {nullptr};
	if (loaded != nullptr) {
		conv = SDL_ConvertSurface(loaded, working_surface->format, 0);
		SDL_FreeSurface(loaded);
	}

	return conv;
}

auto initialise() -> int
{
	double tan_FOV = {tan (FOV / 2)};

	// A ray will be cast for every horizontal pixel
    for (int i = 0; i < SURFACE_WIDTH; i++) {
		scr_pts[i] = tan_FOV - (2 * tan_FOV * (i + 1)) / SURFACE_WIDTH;
		distortion[i] = 1.0 / sqrt(1 + scr_pts[i] * scr_pts[i]);
	}

	// Initialise SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
		return -1;
	}

	working_surface = SDL_CreateRGBSurface(0, SURFACE_WIDTH, SURFACE_HEIGHT, 32, 0, 0, 0, 0);
	if (working_surface == nullptr) {
		std::cout << "Surface could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		return -1;
	}
	renderer = SDL_CreateSoftwareRenderer(working_surface);
	if (renderer == nullptr) {
		std::cout << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		return -1;
	}
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

	working_texture = SDL_CreateTexture(renderer, working_surface->format->format, SDL_TEXTUREACCESS_TARGET,
		SURFACE_WIDTH, SURFACE_HEIGHT);
	if (working_texture == nullptr) {
		std::cout << "Texture could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		return -1;
	}

	working_texture_streaming = SDL_CreateTexture(renderer, working_surface->format->format,
		SDL_TEXTUREACCESS_STREAMING, SURFACE_WIDTH, SURFACE_HEIGHT);
	if (working_texture_streaming == nullptr) {
		std::cout << "Texture Streaming could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		return -1;
	}

	floor_texture = SDL_CreateTexture (renderer, working_surface->format->format, SDL_TEXTUREACCESS_STREAMING, 1,
		SURFACE_HEIGHT);
	if (floor_texture == nullptr) {
		std::cout << "Texture could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		return -1;
	}

	ceiling_texture = SDL_CreateTexture (renderer, working_surface->format->format,
		SDL_TEXTUREACCESS_STREAMING, 1, SURFACE_HEIGHT);
	if (ceiling_texture == nullptr) {
		std::cout << "Texture could not be created! SDL_Error:  " << SDL_GetError() << std::endl;
		return -1;
    }

    // Load Textures
	SDL_Texture* all_walls = {load_texture("res/txtrs/w3d_allwalls.png")};
	wall_textures[0] = crop_texture(all_walls, 0, 0);
	wall_textures[1] = crop_texture(all_walls, 0, 1);
	wall_textures[2] = crop_texture(all_walls, 4, 3);
	ceiling_surface = load_surface ("res/txtrs/w3d_redbrick.png");
	floor_surface = load_surface ("res/txtrs/w3d_bluewall.png");


	// Initialise libtcod
	TCODConsole::initRoot(WINDOW_WIDTH, WINDOW_HEIGHT, "Libtcod Raycaster Demo", false, TCOD_RENDERER_SDL);

    return 0;
}

auto render() -> void
{
	// For every ray (pixel column)
	for (int i = 0; i < SURFACE_WIDTH; i++) {
        double r_x = {1.0};
        double r_y = {scr_pts[i]};
		double base = {std::sqrt(r_x * r_x + r_y * r_y)};
		r_x = r_x / base;
		r_y = r_y / base;

        // Rotate this ray with theta
        double rot_x = {std::cos (theta) * r_x - std::sin (theta) * r_y};
        double rot_y = {std::sin (theta) * r_x + std::cos (theta) * r_y};

        // Step sizes
        int step_x = {sign(rot_x)};
        int step_y = {sign(rot_y)};

        // Calculate Grid lines and hitpoints
        int l_vx = {std::round(player_x + 0.5 * step_x)};
        double l_vy = {-1};
        int l_hy = {std::round(player_y + 0.5 * step_y)};
        double l_hx = {-1};

        // Find where ray hits
        double dist = {-1};
		int txt_x = {-1}; // 0..63 (texture width = 64)

        double hit_x = {0};
        double hit_y = {0};
		int wall_idx = {};
        while (dist < 0)
        {
            // Calculate the hitpoints using the Grid lines
            if (l_vy == -1 && step_x != 0)
                l_vy = player_y + (l_vx - player_x) * (rot_y / rot_x);

            if (l_hx == -1 && step_y != 0)
                l_hx = player_x + (l_hy - player_y) * (rot_x / rot_y);

            // Determine which one "wins" (i,e. the shortest distance/closest one)
            bool vertical_wins = {};
            if (l_vy != -1 && l_hx != -1) {
                vertical_wins = step_x * (l_vx - player_x) < step_x * (l_hx - player_x);
            } else {
                vertical_wins = l_vy != -1;
            }

            // Determine array indices
            int arr_x = {-1};
            int arr_y = {-1};
            if (vertical_wins) {
                hit_x = l_vx;
                hit_y = l_vy;
				txt_x = 64 * (hit_y - std::floor(hit_y));

				// If looking from the left, mirror the texture to correct
				if (step_x == 1)
					txt_x = 63 - txt_x;

                l_vx += step_x;
                l_vy = -1;
                arr_x = step_x < 0 ? hit_x - 1: hit_x;
                arr_y = GRID_HEIGHT - hit_y;
            } else {
                hit_x = l_hx;
                hit_y = l_hy;
				txt_x = 64 * (hit_x - std::floor(hit_x));

				// If looking from above, mirror the texture to correct
				if (step_y == -1)
					txt_x = 63 - txt_x;

                l_hx = -1;
                l_hy += step_y;
                arr_x = hit_x;
                arr_y = GRID_HEIGHT - (step_y < 0 ? hit_y - 1: hit_y) - 1;
            }

            // If we've hit a block
            wall_idx = world[arr_y][arr_x];
            if (wall_idx != 0) {
                double dx = {hit_x - player_x};
                double dy = {hit_y - player_y};
                dist = std::sqrt(dx * dx + dy * dy);
            }
        }

        // Correct distance and calculate height
        double corrected = {dist * distortion[i]};
        int height = {SURFACE_HEIGHT / corrected};

		int y = {(SURFACE_HEIGHT - height) / 2};
		int darkness = {255};
		if (corrected > DARKEST_DIST)
			darkness = DARKNESS_ALPHA;
		else if (corrected <= MIN_DIST)
			darkness = 255;
		else // interpolate
			darkness = std::floor((corrected - MIN_DIST) * (DARKNESS_ALPHA - 255) / (DARKEST_DIST - MIN_DIST) + 255);

		// Get the slice of the texture to render
		SDL_Rect source = {txt_x, 0, 1, 64};
		SDL_Rect destination = {i, y, 1, height};

		// Render the slice
		SDL_Texture* texture = {wall_textures[wall_idx - 1]};
		SDL_SetTextureColorMod(texture, darkness, darkness, darkness);
		SDL_RenderCopy(renderer, texture, &source, &destination);

		// And now deal with floor texture pixels
		if (y > 0) {
			Uint32 floor[y] = {};
			Uint32 ceiling[y] = {};
			Uint32* pixsflr = {reinterpret_cast<Uint32*>(floor_surface->pixels)};
			Uint32* pixsclg = {reinterpret_cast<Uint32*>(ceiling_surface->pixels)};
			for (int j = y - 1 ; j >= 0; j--) {
				double rev_height =  {SURFACE_HEIGHT - 2 * j};
				double rev_corr = {SURFACE_HEIGHT / rev_height};
				double rev_dist = {rev_corr / distortion[i]};

				double real_x = {player_x + rot_x * rev_dist};
				double real_y = {player_y + rot_y * rev_dist};
				real_x = real_x - static_cast<int>(real_x);
				real_y = real_y - static_cast<int>(real_y);
				if (real_x < 0)
					real_x += 1;
				if (real_y < 0)
					real_y += 1;
				int tx = {std::floor(real_x * 64)};
				int ty = {std::floor(real_y * 64)};

				int dark_floor = {255};
				if (rev_corr > DARKEST_DIST)
					dark_floor = DARKNESS_ALPHA;
				else if (rev_corr <= MIN_DIST)
					dark_floor = 255;
				else // interpolate
					dark_floor = std::floor((rev_corr - MIN_DIST) * (DARKNESS_ALPHA - 255) /
						(DARKEST_DIST - MIN_DIST) + 255);
				double scale = {1.0 * dark_floor / 255};

				Uint32 floor_pixel = {static_cast<Uint32>(pixsflr[64 * ty + tx])};
				Uint32 ceiling_pixel = {static_cast<Uint32>(pixsclg[64 * ty + tx])};

				Uint32 f_r = {((floor_pixel >> 16) & 0xFF) * scale};
				Uint32 f_g = {((floor_pixel >> 8) & 0xFF) * scale};
				Uint32 f_b = {((floor_pixel >> 0) & 0xFF) * scale};
				floor[y - 1 - j] = (f_r << 16) + (f_g << 8) + (f_b << 0);

				Uint32 g_r = {((ceiling_pixel >> 16) & 0xFF) * scale};
				Uint32 g_g = {((ceiling_pixel >> 8) & 0xFF) * scale};
				Uint32 g_b = {((ceiling_pixel >> 0) & 0xFF) * scale};
				ceiling[j] = (g_r << 16) + (g_g << 8) + (g_b << 0);
			}

			// And render floor/ceiling
			int pitch = {1 * sizeof(Uint32)};
			SDL_Rect rect = {0, 0, 1, y};
			if (SDL_LockTexture(floor_texture, &rect, &memory_pixels, &pitch) != 0)
				std::cout << SDL_GetError() << std::endl;

			Uint8* pixels = {reinterpret_cast<Uint8*>(memory_pixels)};
			memcpy(pixels, floor, y * pitch);
			SDL_UnlockTexture(floor_texture);
			SDL_Rect destination_floor = {i, y + height, 1, y};
			SDL_RenderCopy (renderer, floor_texture, &rect, &destination_floor);

			if (SDL_LockTexture(ceiling_texture, &rect, &memory_pixels, &pitch) != 0)
				std::cout << SDL_GetError() << std::endl;

			pixels = {reinterpret_cast<Uint8*>(memory_pixels)};
			memcpy(pixels, ceiling, y * pitch);
			SDL_UnlockTexture(ceiling_texture);

			SDL_Rect destination_ceiling = {i, 0, 1, y};
			SDL_RenderCopy (renderer, ceiling_texture, &rect, &destination_ceiling);
		}
    }
}

auto update_ticks() -> void
{
	int ticks_now = {SDL_GetTicks()};
	diff_ticks = ticks_now - ticks;
	ticks = ticks_now;
}

auto close() -> void
{
    if (renderer != nullptr)
        SDL_DestroyRenderer(renderer);
    if (working_surface != nullptr)
        SDL_FreeSurface(working_surface);
    if (working_texture != nullptr)
        SDL_DestroyTexture(working_texture);

    IMG_Quit();
    SDL_Quit();
    TCOD_quit();
}

auto update_world() -> void
{
    double coss = {std::cos(theta)};
	double sinn = {std::sin(theta)};

	// For when walking backwards
	int dsgn = {sign(speed)};
	double sx = {sign(coss) * dsgn};
	double sy = {sign(sinn) * dsgn};

	double dt = {(diff_ticks / 1000.0)};
	double dp = {dt * speed};
	double dx = {coss * dp};
	double dy = {sinn * dp};

	double player_x_new = {player_x + dx};
	double player_y_new = {player_y + dy};

	// Handle collision detection
	double cx_b = {player_x_new - sx * MIN_DIST};
	double cx_f = {player_x_new + sx * MIN_DIST};
	double cy_b = {player_y_new - sy * MIN_DIST};
	double cy_f = {player_y_new + sy * MIN_DIST};
	if (!is_colliding(cx_f, cy_f) &&	!is_colliding(cx_b, cy_f) && !is_colliding(cx_f, cy_b)) {

		// Direction where player is looking at
		player_x = player_x_new;
		player_y = player_y_new;
	} else if (!is_colliding(cx_f, player_y + MIN_DIST) && !is_colliding(cx_f, player_y - MIN_DIST)) {

		// X-Direction
		player_x = player_x_new;
	} else if (!is_colliding(player_x + MIN_DIST, cy_f) && 	!is_colliding(player_x - MIN_DIST, cy_f)) {

		// Y-Direction
		player_y = player_y_new;
	}

    // Otherwise no movement is possible (we are in a corner)
    double diffTurn = {dt * turn};
    theta += diffTurn;
}

auto main(int argc, char* args[]) -> int
{
    // Initialise
	SDL_Color rgb = {};
	Uint32 data = {};
	std::string version = {"Powered by Libtcod " + std::to_string(TCOD_MAJOR_VERSION) + "." +
		std::to_string(TCOD_MINOR_VERSION) + "." + std::to_string(TCOD_PATCHLEVEL)};

	// libtcod off-screen buffer
	std::unique_ptr<TCODConsole> offscreen = std::make_unique<TCODConsole>(SURFACE_WIDTH, SURFACE_HEIGHT);

	if (initialise () < 0)
        return -1;

	// Input Loop
	bool quit = {false};
	TCOD_key_t key_pressed = {};

	while (!quit) {

		// Get keypress
		TCODSystem::checkForEvent(TCOD_EVENT_KEY_PRESS, &key_pressed, nullptr);

		// Handle Movement
		speed = 0.0;
		turn = 0.0;
		switch (key_pressed.vk) {
			case TCODK_KP8:
			case TCODK_UP:
				speed += DEFAULT_SPEED;
				break;
			case TCODK_KP2:
			case TCODK_DOWN:
				speed -= DEFAULT_SPEED;
				break;
			case TCODK_KP4:
			case TCODK_LEFT:
				turn += TURN_SPEED;
				break;
			case TCODK_KP6:
			case TCODK_RIGHT:
				turn -= TURN_SPEED;
				break;
			default:
				break;
		}

		// Update World
        update_ticks();
        update_world();

		// Clear renderer and render
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(renderer);
        SDL_SetRenderTarget(renderer, working_texture);
        SDL_RenderClear(renderer);
        render();

        // Grab the render contents (WARNING: THIS IS VERY VERY VERY HIDEOUSLY SLOW!)
		SDL_SetRenderTarget(renderer, working_texture_streaming);
		SDL_RenderCopy(renderer, working_texture, NULL, NULL);
		SDL_Surface *source_for_libtcod = SDL_CreateRGBSurface(0, SURFACE_WIDTH , SURFACE_HEIGHT, 32, 0x00ff0000,
			0x0000ff00, 0x000000ff, 0xff000000);
		SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGB888, source_for_libtcod->pixels,
			source_for_libtcod->pitch);

        // Clear libtcod screen and buffer
        TCODConsole::root->clear();
        offscreen->setDefaultBackground(TCODColor::black);
        offscreen->clear();

        // Populate libtcod offscreen
        for (int x = 0; x < SURFACE_WIDTH; x++) {
			for (int y = 0; y < SURFACE_HEIGHT; y++) {
				data = get_pixel(source_for_libtcod, x, y);
				SDL_GetRGB(data, source_for_libtcod->format, &rgb.r, &rgb.g, &rgb.b);
				TCODColor temp(rgb.r, rgb.g, rgb.b);
                offscreen->setCharBackground(x, y, temp);
				offscreen->setCharForeground(x, y, temp);
                offscreen->putChar(x, y, 32);
			}
        }

        // Note that using the alternate refreshConsole method crashes so a new image needs to be created every frame

		// Render to libtcod
		std::unique_ptr<TCODImage> image_to_render = std::make_unique<TCODImage>(offscreen.get());
		image_to_render->scale(WINDOW_WIDTH * 2, WINDOW_HEIGHT * 2);
		image_to_render->blit2x(TCODConsole::root, 1, 1);

		// Update Text
		TCODConsole::root->setDefaultForeground(TCODColor::yellow);
		TCODConsole::root->printf(2, WINDOW_HEIGHT - 5, "Libtcod Raycaster Coplayer_yright (C) 2019 Dave Moore");
		TCODConsole::root->setDefaultForeground(TCODColor::orange);
		TCODConsole::root->printf(2, WINDOW_HEIGHT - 3, "davemoore22@gmail.com");
		TCODConsole::root->setDefaultForeground(TCODColor::red);
		TCODConsole::root->printf(2, WINDOW_HEIGHT - 1, "Code released under the GPL v3");
		TCODConsole::root->setDefaultForeground(TCODColor::silver);
		TCODConsole::root->printf(70, WINDOW_HEIGHT - 5, "Based upon 3D Raycaster by Timmos");
		TCODConsole::root->setDefaultForeground(TCODColor::cyan);
		TCODConsole::root->printf(70, WINDOW_HEIGHT - 3, "https://github.com/T1mmos/raycaster-sdl");
		TCODConsole::root->setDefaultForeground(TCODColor::silver);
		TCODConsole::root->print(70, WINDOW_HEIGHT - 1, version);
		TCODConsole::root->flush();
    }

    close();
    return 0;
}
