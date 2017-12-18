#include <stdio.h>
#include <iostream>
#include <vector>
#include <limits.h>
#include <cfloat>
#include "omp.h"
#include "SDL.h"
#include "maths.hpp"

#define CANVAS_HEIGHT 1080
#define CANVAS_WIDTH 1920
#define NUM_OBJ 0
#define NUM_SPHERE 1
#define NUM_LIGHTS 2

std::vector<triangle> init_triangles();
std::vector<sphere> init_spheres();
std::vector<vec3> init_lights();
int check_intersection(std::vector<triangle> obj, std::vector<sphere> spheres, vec3 *P, int *index, vec3 orig, vec3 dir);
void init_SDL(SDL_Window* &window, SDL_Renderer* &renderer);
void put_pixel(SDL_Surface* screenSurface, int x, int y, vec3 color);
template<class T>
T clamp(int value, int min, int max)
{
	return value < min ? min : (value > max ? max : value);
}

int main() {
	int i, j, l;
	float aspectRatio = (float)CANVAS_WIDTH / (float)CANVAS_HEIGHT;
	int fov = 120;
	unsigned char *frameBuffer;

	frameBuffer = new unsigned char[4 * CANVAS_HEIGHT * CANVAS_WIDTH];

	// Init scene
	std::vector<triangle> tris = init_triangles();
	std::vector<sphere> spheres = init_spheres();
	std::vector<vec3> lights = init_lights();

	vec3 orig = {.x = 0.0, .y = 0.0, .z = 0.0};

	/**
	 * Create SDL Window
	 **/
	SDL_Window *window = NULL;
	SDL_Renderer *renderer = NULL;
	SDL_Texture *texture = NULL;

	init_SDL(window, renderer);

	texture = SDL_CreateTexture (
				        renderer,
				        SDL_PIXELFORMAT_ARGB8888,
				        SDL_TEXTUREACCESS_STREAMING,
				        CANVAS_WIDTH, CANVAS_HEIGHT );


#pragma omp parallel for collapse(2) schedule(dynamic) private(i,j,l) shared(frameBuffer)
	for(i = 0; i < CANVAS_HEIGHT; i++) {
		for(j = 0; j < CANVAS_WIDTH; j++) {		

			// Canvas to world transformation
			float px = (2* ((j + 0.5) / CANVAS_WIDTH) - 1) * 
										tan(fov / 2 * M_PI / 180) * aspectRatio;
			float py = (1 - 2* ((i + 0.5) / CANVAS_HEIGHT)) * tan(fov / 2 * M_PI / 180);
			vec3 rayP = {.x = px, .y = py, .z = -1.0};

			// Ray direction
			vec3 dir = sub_vec3(rayP, orig);
			normalize_vec3(&dir);

			// Get transformed framebuffer index
			int fb_offset = CANVAS_WIDTH * i * 4 + j * 4;

			// Initialize framebuffer pixel color
			frameBuffer[fb_offset + 0] = 0;
			frameBuffer[fb_offset + 1] = 0;
			frameBuffer[fb_offset + 2] = 0;
			frameBuffer[fb_offset + 3] = SDL_ALPHA_OPAQUE;

			vec3 P;
			int index;

			// Check to see if there is an intersection between the camera ray and
			// all the objects
			int check = check_intersection(tris, spheres, &P, &index, orig, dir);			
			if(check != 0) {

				// normal of the object intersected
				vec3 n = (check == 1) ? 
						  normal(tris[index]) :
						  sub_vec3(P, spheres[index].orig);
				normalize_vec3(&n);
				// Color of the object intersected
				vec3 color = (check == 1) ?
							 tris[index].color :
							 spheres[index].color;

				// For all the lights in the world, check to see if
				// the intersection point is lit by any of them
				for(l = 0; l < lights.size(); l++) {

					// Ray from point to light
					vec3 rayDir = sub_vec3(lights[l], P);
					

					normalize_vec3(&rayDir);

					vec3 P1;

					// Check to see if there isn't any objects in the way of the ray
					check = check_intersection(tris, spheres, &P1, &index, P, rayDir) == 0 ? 1 : 0;
					// Calculate angle between the normal and the ray
					// so we can calculate brightness
					float angle = dotProduct(n, rayDir);

					if(angle > 0) {
						vec3 v = scale_vec3(color, angle);
						frameBuffer[fb_offset + 0] = clamp<int>(frameBuffer[fb_offset + 0] + check*(unsigned char)v.x, 0, 255);
						frameBuffer[fb_offset + 1] = clamp<int>(frameBuffer[fb_offset + 1] + check*(unsigned char)v.y, 0, 255);
						frameBuffer[fb_offset + 2] = clamp<int>(frameBuffer[fb_offset + 2] + check*(unsigned char)v.z, 0, 255);
					}
				}
			}
		}
		
	}

	SDL_UpdateTexture(texture, NULL, &frameBuffer[0], CANVAS_WIDTH*4);

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

		SDL_RenderCopy( renderer, texture, NULL, NULL );
		SDL_RenderPresent(renderer);
	}

	SDL_DestroyRenderer( renderer );
    SDL_DestroyWindow( window );
    SDL_Quit();

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
	tris.emplace_back(t4);

	triangle t5 = {.p1 = {.x = -10.0, .y = -5.0, .z = -2.0},
				   .p2 = {.x = 10.0, .y = -5.0, .z = -10.0},
				   .p3 = {.x = -10.0, .y = -5.0, .z = -10.0},
				   .color = {.x = 255, .y = 255, .z = 255}};
	tris.emplace_back(t5);

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

int check_intersection(std::vector<triangle> tris, std::vector<sphere> spheres, vec3 *P, int *index, vec3 orig, vec3 dir) {
	int i, index_t, index_s;
	
	int t_has_intersected = false;
	int s_has_intersected = false;

	vec3 p_triangle = {.x = FLT_MAX, .y = FLT_MAX, .z = FLT_MAX};
	vec3 p_sphere = {.x = FLT_MAX, .y = FLT_MAX, .z = FLT_MAX};

	for(i = 0; i < tris.size(); i++) {
		if(rayTriangleIntersects(orig, dir, tris[i], P)) {
			if(length_vec3(p_triangle) > length_vec3(*P)) {
				index_t = i;
				p_triangle = *P;
				t_has_intersected = true;
			}
		}
	}

	for(i = 0; i < spheres.size(); i++) {
		if(raySphereIntersects(orig, dir, spheres[i], P)) {
			if(length_vec3(p_sphere) > length_vec3(*P)) {
				index_s = i;
				p_sphere = *P;
				s_has_intersected = true;
			}
		}
	}

	if(!t_has_intersected && !s_has_intersected)
		return 0;
	if(length_vec3(p_triangle) <= length_vec3(p_sphere)){
		*P = p_triangle;
		*index = index_t;
		return 1;
	}
	if(length_vec3(p_triangle) > length_vec3(p_sphere)) {
		*P = p_sphere;
		*index = index_s;
		return 2;
	}
}
