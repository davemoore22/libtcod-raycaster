/* Main executable and game loop. */

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <stdio.h>
#include <math.h>
#include <string>
#include <iostream>
#include "libtcod.h"


#define sgn(a) (a < 0 ? -1 : a > 0 ? +1 : 0)

bool isColliding (double x, double y);

SDL_Texture* loadTexture (const char * file);
SDL_Surface* loadSurface (const char * file);
SDL_Texture* cropTexture (SDL_Texture* src, int x, int y);

struct Texture {
    SDL_Texture* texture;
    int width;
    int height;
};


// resolution (and in windowed mode, the screen size)
const int SURFACE_WIDTH = 400;		// horizontal resolution
const int SURFACE_HEIGHT = 300;		// vertical resolution
const int WINDOW_WIDTH = 180;		// horizontal resolution
const int WINDOW_HEIGHT = 100;		// vertical resolution

// the field of view
const double FOV = 1.30899694; 		// rad (75 deg)
const double MIN_DIST = 0.3;		// square sides / s
const double DARKEST_DIST = 6.0;	// any greater distance will not be darker
const int DARKNESS_ALPHA = 40;		// minimal lighting

// world size
const int GRID_WIDTH = 18;
const int GRID_HEIGHT = 10;

const double DEFAULT_SPEED = 3; 	// sqsides/s
const double TURN_SPEED = M_PI; 	// rad/s


void * mPixels;


Uint32 getpixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

switch (bpp)
{
    case 1:
        return *p;
        break;

    case 2:
        return *(Uint16 *)p;
        break;

    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;
        break;

        case 4:
            return *(Uint32 *)p;
            break;

        default:
            return 0;       /* shouldn't happen, but avoids warnings */
      }
}


// the fixed map
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

//The window we'll be rendering to
//SDL_Window* window = NULL;

//The surface contained by the window
SDL_Surface* gSurface = NULL;

SDL_Renderer* gRenderer = NULL;
#include <SDL2/SDL_ttf.h>
SDL_Texture* gTexture = NULL;

SDL_Texture* gFloorTexture = NULL;
SDL_Texture* gCeilgTexture = NULL;


// a GPU accelerated wall texture
SDL_Texture* txt_walls [3];

SDL_Surface* srf_floor;
SDL_Surface* srf_ceilg;


std::unique_ptr<TCODConsole> offscreen;

// the initial player position
double px = 1.5, py = 1.5;

// the intial central ray direction
double teta = 0.0;

// the tangent y-coordinate for teta=0, one for every horizontal pixel
double scr_pts [SURFACE_WIDTH];

// correction values for distortion
double distortion [SURFACE_WIDTH];

int ticks;			// ms
int diffTicks;		// ms

double speed = 0.0; // sqsides/s
double turn = 0.0;  // rad/s


int init ()
{
    double tan_FOV = tan (FOV / 2);

    // a ray will be casted for every horizontal pixel
    for (int i = 0; i < SURFACE_WIDTH; i++){
        scr_pts[i] = tan_FOV - (2 * tan_FOV * (i + 1)) / SURFACE_WIDTH;
        distortion[i] = 1.0 / sqrt(1 + scr_pts[i] * scr_pts[i]);
    }

    if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 )
    {
        printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
        return -1;
    }

	TCODConsole::initRoot(WINDOW_WIDTH, WINDOW_HEIGHT, "Raycaster Demo", false, TCOD_RENDERER_SDL);

	offscreen = std::make_unique<TCODConsole>(SURFACE_WIDTH, SURFACE_HEIGHT);

	gSurface = SDL_CreateRGBSurface(0, SURFACE_WIDTH, SURFACE_HEIGHT, 32, 0, 0, 0, 0);
    if ( gSurface == NULL )
    {
        printf( "Surface could not be created! SDL_Error: %s\n", SDL_GetError() );
        return -1;
    }
	gRenderer = SDL_CreateSoftwareRenderer(gSurface);
    if ( gRenderer == NULL )
    {
        printf( "Renderer could not be created! SDL_Error: %s\n", SDL_GetError() );
        return -1;
    }
    SDL_SetRenderDrawBlendMode (gRenderer, SDL_BLENDMODE_NONE);

    gTexture = SDL_CreateTexture(gRenderer, gSurface->format->format,
        SDL_TEXTUREACCESS_TARGET, SURFACE_WIDTH, SURFACE_HEIGHT);
    if ( gTexture == NULL )
    {
        printf( "Texture could not be created! SDL_Error: %s\n", SDL_GetError() );
        return -1;
    }

	gFloorTexture = SDL_CreateTexture (gRenderer, gSurface->format->format,
		SDL_TEXTUREACCESS_STREAMING, 1, SURFACE_HEIGHT);
	if ( gFloorTexture == NULL )
    {
        printf( "Texture could not be created! SDL_Error: %s\n", SDL_GetError() );
        return -1;
    }

	gCeilgTexture = SDL_CreateTexture (gRenderer, gSurface->format->format,
		SDL_TEXTUREACCESS_STREAMING, 1, SURFACE_HEIGHT);
	if ( gCeilgTexture == NULL )
    {
        printf( "Texture could not be created! SDL_Error: %s\n", SDL_GetError() );
        return -1;
    }

	SDL_Texture* allWalls = loadTexture("res/txtrs/w3d_allwalls.png");

	txt_walls[0] = cropTexture (allWalls, 0, 0);
	txt_walls[1] = cropTexture (allWalls, 0, 1);
	txt_walls[2] = cropTexture (allWalls, 4, 3);

	srf_ceilg = loadSurface ("res/txtrs/w3d_redbrick.png");
	srf_floor = loadSurface ("res/txtrs/w3d_bluewall.png");

    return 0;
}

SDL_Texture* cropTexture (SDL_Texture* src, int x, int y)
{
	SDL_Texture* dst = SDL_CreateTexture(gRenderer, gSurface->format->format, SDL_TEXTUREACCESS_TARGET, 64, 64);
	SDL_Rect rect = {64 * x, 64 * y, 64, 64};
	SDL_SetRenderTarget(gRenderer, dst);
	SDL_RenderCopy(gRenderer, src, &rect, NULL);
	SDL_SetRenderTarget(gRenderer, NULL);

	return dst;
}

SDL_Texture* loadTexture (const char * file)
{
	SDL_Surface* srfc = loadSurface(file);
	if(!srfc) {
		printf("IMG_LoadPNG_RW: %s\n", IMG_GetError());
		// handle error
	}

	SDL_Texture* txt = SDL_CreateTextureFromSurface(gRenderer, srfc);

	SDL_FreeSurface(srfc);

	return txt;
}

SDL_Surface* loadSurface (const char * file)
{
	SDL_RWops* rwop = SDL_RWFromFile(file, "rb");
	SDL_Surface* loaded = IMG_LoadPNG_RW(rwop);
	SDL_Surface* conv=NULL;
	if (loaded != NULL)
	{
		conv = SDL_ConvertSurface(loaded, gSurface->format, 0);
		SDL_FreeSurface(loaded);
	}
	return conv;
}

void renderScene ()
{

	for (int i = 0; i < SURFACE_WIDTH; i++)
	{
        double r_x = 1.0;
        double r_y = scr_pts[i]; // precalculated
		double base = sqrt(r_x * r_x + r_y * r_y);
		r_x = r_x / base;
		r_y = r_y / base;

        // rotate this ray with teta
        double rot_x = cos (teta) * r_x - sin (teta) * r_y;
        double rot_y = sin (teta) * r_x + cos (teta) * r_y;

        // step sizes
        int step_x = sgn(rot_x);
        int step_y = sgn(rot_y);

        // grid lines and hitpoints to calculate
        int l_vx = round(px + 0.5 * step_x);
        double l_vy = -1;
        int l_hy = round(py + 0.5 * step_y);
        double l_hx = -1;

        // find hitpoint
        double dist = -1;
		int txt_x = -1; // 0..63 (texture width = 64)

        double hit_x = 0, hit_y = 0;
		int wall_idx;
        while (dist < 0)
        {
            // calculate the hitpoints with the grid lines
            if (l_vy == -1 && step_x != 0)
                l_vy = py + (l_vx - px) * (rot_y / rot_x);

            if (l_hx == -1 && step_y != 0)
                l_hx = px + (l_hy - py) * (rot_x / rot_y);

            // determine which one "wins" (= shortest distance)
            bool vertWin;
            if (l_vy != -1 && l_hx != -1)
            {    // 2 candidates, choose closest one
                vertWin = step_x * (l_vx - px) < step_x * (l_hx - px);
            }
            else
            {    // one candidate
                vertWin = l_vy != -1;
            }

            // determine array indices
            int arr_x = -1, arr_y = -1;
            if (vertWin)
            {
                hit_x = l_vx;
                hit_y = l_vy;

				txt_x = 64 * (hit_y - (int) hit_y);
				if ( step_x == 1)
				{ 	// // looking from the left, mirror the texture to correct
					txt_x = 63 - txt_x;
				}

                l_vx += step_x;
                l_vy = -1;

                arr_x = step_x < 0 ? hit_x - 1: hit_x;
                arr_y = GRID_HEIGHT - hit_y;
            }
            else
            {
                hit_x = l_hx;
                hit_y = l_hy;

				txt_x = 64 * (hit_x - (int) hit_x);
				if ( step_y == -1)
				{ 	// looking from above, mirror the texture to correct
					txt_x = 63 - txt_x;
				}

                l_hx = -1;
                l_hy += step_y;

                arr_x = hit_x;
                arr_y = GRID_HEIGHT - (step_y < 0 ? hit_y - 1: hit_y) - 1;
            }

            wall_idx = world[arr_y][arr_x];
            if (wall_idx != 0)
            {    // we've hit a block
                double dx = hit_x - px;
                double dy = hit_y - py;
                dist = sqrt (dx * dx + dy * dy);
            }
        }

        // correct distance and calculate height
        double corrected = dist * distortion[i];
        int height = SURFACE_HEIGHT / corrected;

		int y = (SURFACE_HEIGHT - height) / 2;
		int darkness = 255;
		if (corrected > DARKEST_DIST)
			darkness = DARKNESS_ALPHA;
		else if (corrected <= MIN_DIST)
			darkness = 255;
		else // interpolate
			darkness = (int) ( (corrected - MIN_DIST) * (DARKNESS_ALPHA - 255) / (DARKEST_DIST - MIN_DIST) + 255);

		SDL_Rect src = { txt_x, 0, 1, 64 };
		SDL_Rect dst = { i, y, 1, height };

		if (wall_idx - 1 >= 3)
		{
			printf("%d\n", wall_idx);
		}
		SDL_Texture* txt = txt_walls[wall_idx - 1];

		SDL_SetTextureColorMod( txt, darkness, darkness, darkness );
		SDL_RenderCopy(gRenderer, txt, &src, &dst);

		// get floor texture pixels

		if (y > 0)
		{
			Uint32 floor [y];
			Uint32 ceilg [y];
			Uint32* pixsflr = (Uint32*) srf_floor->pixels;
			Uint32* pixsclg = (Uint32*) srf_ceilg->pixels;
			for (int j = y - 1 ; j >= 0; j--)
			{
				double rev_height =  SURFACE_HEIGHT - 2 * j;
				double rev_corr = SURFACE_HEIGHT / rev_height;
				double rev_dist = rev_corr / distortion[i];

				double real_x = px + rot_x * rev_dist;
				double real_y = py + rot_y * rev_dist;

				real_x = real_x - (int) real_x;
				real_y = real_y - (int) real_y;
				if (real_x < 0) real_x += 1;
				if (real_y < 0) real_y += 1;
				int tx = (int)(real_x * 64);
				int ty = (int)(real_y * 64);

				int darkflr = 255;
				if (rev_corr > DARKEST_DIST)
					darkflr = DARKNESS_ALPHA;
				else if (rev_corr <= MIN_DIST)
					darkflr = 255;
				else // interpolate
					darkflr = (int) ( (rev_corr - MIN_DIST) * (DARKNESS_ALPHA - 255) / (DARKEST_DIST - MIN_DIST) + 255);
				double scale = 1.0 * darkflr / 255;

				Uint32 pixflr = (Uint32) pixsflr [64 * ty + tx];
				Uint32 pixclg = (Uint32) pixsclg [64 * ty + tx];

				Uint32 f_r = (( pixflr >> 16 ) & 0xFF ) * scale;
				Uint32 f_g = (( pixflr >> 8 ) & 0xFF ) * scale;
				Uint32 f_b = (( pixflr >> 0 ) & 0xFF ) * scale;
				floor[y - 1 - j] = (f_r << 16) + (f_g << 8) + (f_b << 0);

				Uint32 g_r = (( pixclg >> 16 ) & 0xFF ) * scale;
				Uint32 g_g = (( pixclg >> 8 ) & 0xFF ) * scale;
				Uint32 g_b = (( pixclg >> 0 ) & 0xFF ) * scale;
				ceilg[j] = (g_r << 16) + (g_g << 8) + (g_b << 0);
			}

			int pitch = 1 * sizeof(Uint32);
			SDL_Rect rect = {0, 0, 1, y};

			if (SDL_LockTexture(gFloorTexture, &rect, &mPixels, &pitch) != 0){
				printf("Error: %s\n", SDL_GetError() );
			}
			Uint8* pixels = (Uint8 *) mPixels;
			memcpy(pixels, floor, y * pitch);
			SDL_UnlockTexture(gFloorTexture);
			SDL_Rect dstflr = {i, y + height, 1, y};
			SDL_RenderCopy (gRenderer, gFloorTexture, &rect, &dstflr);

			if (SDL_LockTexture(gCeilgTexture, &rect, &mPixels, &pitch) != 0){
				printf("Error: %s\n", SDL_GetError() );
			}
			pixels = (Uint8 *) mPixels;
			memcpy(pixels, ceilg, y * pitch);
			SDL_UnlockTexture(gCeilgTexture);

			SDL_Rect dstclg = {i, 0, 1, y};
			SDL_RenderCopy (gRenderer, gCeilgTexture, &rect, &dstclg);
		}

    }
}

void updateTicks (){
    int ticksNow = SDL_GetTicks();
    diffTicks = ticksNow - ticks;
    ticks = ticksNow;
}


void close ()
{
    if (gRenderer != NULL)
        SDL_DestroyRenderer(gRenderer);
    if (gSurface != NULL)
        SDL_FreeSurface(gSurface);
    //if (window != NULL)
    //   SDL_DestroyWindow(window);
    if (gTexture != NULL)
        SDL_DestroyTexture(gTexture);

    IMG_Quit();
    SDL_Quit();
}

void updateWorld (){

    double coss = cos(teta);
	double sinn = sin(teta);

	int dsgn = sgn(speed); // for when walking backwards
	double sx = sgn(coss) * dsgn;
	double sy = sgn(sinn) * dsgn;

	double dt = (diffTicks / 1000.0);
	double dp = dt * speed;
	double dx = coss * dp;
	double dy = sinn * dp;

	double px_new = px + dx;
	double py_new = py + dy;

	// collision detection
	double cx_b = px_new - sx * MIN_DIST;
	double cx_f = px_new + sx * MIN_DIST;
	double cy_b = py_new - sy * MIN_DIST;
	double cy_f = py_new + sy * MIN_DIST;
	if (	!isColliding(cx_f, cy_f)
		&&	!isColliding(cx_b, cy_f)
		&&	!isColliding(cx_f, cy_b) )
	{	// direction where playing is looking at
		px = px_new;
		py = py_new;
	}
	else if (!isColliding(cx_f, py + MIN_DIST)
		&& 	 !isColliding(cx_f, py - MIN_DIST))
	{ // X-direction
		px = px_new;
	}
	else if (!isColliding(px + MIN_DIST, cy_f)
		&& 	 !isColliding(px - MIN_DIST, cy_f))
	{ // Y-direction
		py = py_new;
	}
	// else: no movement possible (corner)

    double diffTurn = dt * turn;
    teta += diffTurn;
}

bool isColliding (double x, double y)
{
	int arr_x = (int) x;
	int arr_y = GRID_HEIGHT - 1 - (int) y;

	return world [arr_y][arr_x] != 0;
}

int main( int argc, char* args[] )
{
    if (init () < 0)
        return -1;

    bool quit = false;

    TCOD_key_t key_pressed = {};

    while (!quit) {

		TCODSystem::checkForEvent(TCOD_EVENT_KEY_PRESS, &key_pressed, nullptr);

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

        updateTicks();
        updateWorld();


        //Fill the surface black
        SDL_SetRenderDrawColor ( gRenderer, 0x00, 0x00, 0x00, 0xFF );
        SDL_RenderClear(gRenderer);
        SDL_SetRenderTarget(gRenderer, gTexture);
        SDL_RenderClear(gRenderer);

        renderScene();

        SDL_SetRenderTarget(gRenderer, NULL);
        SDL_RenderCopy(gRenderer, gTexture, NULL, NULL);


		SDL_Surface *source_for_libtcod = SDL_CreateRGBSurface(0, SURFACE_WIDTH , SURFACE_HEIGHT, 32,
			0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
		SDL_RenderReadPixels(gRenderer, NULL, SDL_PIXELFORMAT_RGB888, source_for_libtcod->pixels,
			source_for_libtcod->pitch);

        TCODConsole::root->clear();

        offscreen->setDefaultBackground(TCODColor::black);
        offscreen->clear();
        SDL_Color rgb;
        Uint32 data;
        for (int x = 0; x < SURFACE_WIDTH; x++) {
			for (int y = 0; y < SURFACE_HEIGHT; y++) {
				data = getpixel(source_for_libtcod, x, y);
				SDL_GetRGB(data, source_for_libtcod->format, &rgb.r, &rgb.g, &rgb.b);

                TCODColor temp(std::stoi(std::to_string(rgb.r)), std::stoi(std::to_string(rgb.g)),
					std::stoi(std::to_string(rgb.b)));

                offscreen->setCharBackground(x, y, temp);
				offscreen->setCharForeground(x, y, temp);
                offscreen->putChar(x, y, 32);
			}
        }

        TCODImage *image_to_render = new TCODImage(offscreen.get());
		image_to_render->scale(WINDOW_WIDTH * 2, WINDOW_HEIGHT * 2);
		image_to_render->blit2x(TCODConsole::root, 1, 1);

		TCODConsole::root->setDefaultForeground(TCODColor::yellow);
		TCODConsole::root->print(2, WINDOW_HEIGHT - 5, "Libtcod Raycaster Copyright (C) 2019 Dave Moore");

		TCODConsole::root->setDefaultForeground(TCODColor::orange);
		TCODConsole::root->print(2, WINDOW_HEIGHT - 3, "davemoore22@gmail.com");

		TCODConsole::root->setDefaultForeground(TCODColor::red);
		TCODConsole::root->print(2, WINDOW_HEIGHT - 1, "Code released under the GPL v3");

		TCODConsole::root->setDefaultForeground(TCODColor::silver);
		TCODConsole::root->print(70, WINDOW_HEIGHT - 5, "Based upon 3D Raycaster by Timmos");

		TCODConsole::root->setDefaultForeground(TCODColor::cyan);
		TCODConsole::root->print(70, WINDOW_HEIGHT - 3, "https://github.com/T1mmos/raycaster-sdl");

		TCODConsole::root->setDefaultForeground(TCODColor::silver);
		std::string version = "Powered by Libtcod " + std::to_string(TCOD_MAJOR_VERSION) + "." +
			std::to_string(TCOD_MINOR_VERSION) + "." + std::to_string(TCOD_PATCHLEVEL);
		TCODConsole::root->print(70, WINDOW_HEIGHT - 1, version);

		TCODConsole::root->flush();
    }
    close();


    return 0;
}
