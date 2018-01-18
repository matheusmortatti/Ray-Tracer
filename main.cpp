#include <stdio.h>
#include <iostream>
#include <vector>
#include <limits.h>
#include <cfloat>
#include <thread>
#include "omp.h"
#include "SDL.h"
#include "renderer.hpp"
#include "maths.hpp"

std::vector<triangle> init_triangles();
std::vector<sphere> init_spheres();
std::vector<vec3> init_lights();

void init_SDL(SDL_Window* &window, SDL_Renderer* &renderer);
void put_pixel(SDL_Surface* screenSurface, int x, int y, vec3 color);

int main() {
	int fov = 120;
	unsigned char *frameBuffer;

	frameBuffer = new unsigned char[4 * CANVAS_HEIGHT * CANVAS_WIDTH];

	// Init scene
	std::vector<triangle> 	t 	= init_triangles();
	std::vector<sphere>		s   = init_spheres();
	std::vector<vec3> 		lg 	= init_lights();

	// Convert it to normal arrays for omp compatibility
	triangle *tris 	= t.data();
	sphere *spheres = s.data();
	vec3 *lights 	= lg.data();

	int t_size = t.size(), s_size = s.size(), l_size = lg.size();


	/**
	 * Create SDL Window
	 **/
	SDL_Window 	  *window 	= NULL;
	SDL_Renderer  *renderer = NULL;
	SDL_Texture   *texture 	= NULL;

	init_SDL(window, renderer);

	texture = SDL_CreateTexture (
				        renderer,
				        SDL_PIXELFORMAT_ARGB8888,
				        SDL_TEXTUREACCESS_STREAMING,
				        CANVAS_WIDTH, CANVAS_HEIGHT );


	// Create thread and start rendering
	std::thread render_thread(render,frameBuffer,fov,tris,spheres,lights);

	/**
	 * Keep the screen up until the user closes it
	 **/
	bool quit = false;
	while(!quit)
	{
		SDL_SetRenderDrawColor( renderer, 0, 0, 0, SDL_ALPHA_OPAQUE );
        SDL_RenderClear( renderer );

		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
				case SDL_QUIT:
					quit = true;
					break;
			}
		}

		/* Draw the image to the texture */
		SDL_UpdateTexture(texture, NULL, &frameBuffer[0], CANVAS_WIDTH*4);
		SDL_RenderCopy( renderer, texture, NULL, NULL );
		SDL_RenderPresent(renderer);
	}

	SDL_DestroyRenderer( renderer );
    SDL_DestroyWindow( window );
    SDL_Quit();

    // Join thread to wait for it to end before exiting
    render_thread.join();

	return 0;
}

void put_pixel(SDL_Surface* screenSurface, int x, int y, vec3 color)
{
	Uint8 *pixels = (Uint8*)screenSurface->pixels;
	Uint8 *pixel  = pixels + y * screenSurface->pitch + x;
	*pixel = SDL_MapRGB(screenSurface->format, 255, 255, 255);
}

void init_SDL(SDL_Window* &window, SDL_Renderer* &renderer)
{

	if(SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		std::cout << "SDL Could not initialize! SDL Error: " << SDL_GetError() << std::endl;
	}
	else
	{
		window = SDL_CreateWindow(
							"RayTracer", 
							SDL_WINDOWPOS_UNDEFINED, 
							SDL_WINDOWPOS_UNDEFINED,
							CANVAS_WIDTH, 
							CANVAS_HEIGHT,
							SDL_WINDOW_SHOWN);
		if(window == NULL)
		{
			std::cout << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
		}
		else
		{
			renderer = SDL_CreateRenderer (
						        window,
						        -1,
						        SDL_RENDERER_ACCELERATED
						        );
		}
	}
}

std::vector<triangle> init_triangles() {

	std::vector<triangle> tris;

	triangle t4 = {.p1 = {.x = 10.0, .y = -5.0, .z = -2.0},
				   .p2 = {.x = 10.0, .y = -5.0, .z = -10.0},
				   .p3 = {.x = -10.0, .y = -5.0, .z = -2.0},
				   .color = {.x = 255, .y = 255, .z = 255}};
	tris.push_back(t4);

	triangle t5 = {.p1 = {.x = -10.0, .y = -5.0, .z = -2.0},
				   .p2 = {.x = 10.0, .y = -5.0, .z = -10.0},
				   .p3 = {.x = -10.0, .y = -5.0, .z = -10.0},
				   .color = {.x = 255, .y = 255, .z = 255}};
	tris.push_back(t5);

	return tris;
}

std::vector<sphere> init_spheres() {
	std::vector<sphere> s;

	sphere s0 = {.orig = {.x = 0, .y = 0, .z = -4.0}, 
				 .color = {.x = 0, .y = 255, .z = 0}, 
				 .radius = 1.0};
	s.push_back(s0);

	return s;
}

std::vector<vec3> init_lights() {
	std::vector<vec3> lights;


	vec3 light0 = {.x = 2, .y = 5, .z = -4.0};
	lights.push_back(light0);

	vec3 light1 = {.x = -2, .y = 5, .z = -4.0};
	lights.push_back(light1);

	return lights;
}