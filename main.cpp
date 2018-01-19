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

int init_triangles(float**** tris, unsigned char*** colors);
int init_spheres(float*** spheres, float** radius, unsigned char*** colors);
int init_lights(float*** lights);

void init_SDL(SDL_Window* &window, SDL_Renderer* &renderer);
void put_pixel(SDL_Surface* screenSurface, int x, int y, vec3 color);

int main() {
	int fov = 120;
	unsigned char *frameBuffer;

	frameBuffer = new unsigned char[4 * CANVAS_HEIGHT * CANVAS_WIDTH];

	float ***tris, **spheres, *radius, **lights;
	unsigned char **color_tri, **color_sphere;

	int t_size, s_size, l_size;

	t_size = init_triangles(&tris, &color_tri);
	s_size = init_spheres(&spheres, &radius, &color_sphere);
	l_size = init_lights(&lights);

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
	std::thread render_thread(render, frameBuffer, fov, tris, color_tri, t_size, spheres, radius, color_sphere, s_size, lights, l_size);	

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

int init_triangles(float**** tris, unsigned char*** colors)
{
	(*tris) = new float**[2];
	(*colors) = new unsigned char*[2];

	(*tris)[0] = new float*[3];

	(*tris)[0][0] = new float[3]{10.0, -5.0, -2.0};
	(*tris)[0][1] = new float[3]{10.0, -5.0, -10.0};
	(*tris)[0][2] = new float[3]{-10.0, -5.0, -2.0};
	(*colors)[0]  = new unsigned char[3]{255, 255, 255};

	(*tris)[1] = new float*[3];

	(*tris)[1][0] = new float[3]{-10.0, -5.0, -2.0};
	(*tris)[1][1] = new float[3]{10.0, -5.0, -10.0};
	(*tris)[1][2] = new float[3]{-10.0, -5.0, -10.0};
	(*colors)[1]  = new unsigned char[3]{255, 255, 255};

	return 2;
}

int init_spheres(float*** spheres, float** radius, unsigned char*** colors)
{
	(*spheres) = new float*[1];
	(*radius)  = new float[1];
	(*colors) = new unsigned char*[1];

	(*spheres)[0] = new float[3]{0.0, 0.0, -4.0};
	(*colors)[0]  = new unsigned char[3]{0, 255, 0};
	(*radius)[0] = 1.0;

	return 1;
}

int init_lights(float*** lights)
{
	(*lights) = new float*[2];

	(*lights)[0] = new float[3]{2.0, 5.0, -4.0};
	(*lights)[1] = new float[3]{-2.0, 5.0, -4.0};

	return 2;
}