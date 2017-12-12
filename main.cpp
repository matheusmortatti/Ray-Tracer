#include <stdio.h>
#include <iostream>
#include <limits.h>
#include <cfloat>
#include "SDL.h"
#include "maths.hpp"

#define CANVAS_HEIGHT 512
#define CANVAS_WIDTH 723
#define NUM_OBJ 1
#define NUM_SPHERE 1
#define NUM_LIGHTS 1

object init_objects();
sphere_collection init_spheres();
vec3* init_lights();
int check_intersection(object obj, sphere_collection spheres, vec3 *P, int *index, vec3 orig, vec3 dir);
void init_SDL(SDL_Window* &window, SDL_Renderer* &renderer);
void put_pixel(SDL_Surface* screenSurface, int x, int y, vec3 color);
template<class T>
T clamp(T value, T min, T max)
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
	object obj = init_objects();
	sphere_collection spheres = init_spheres();
	vec3* lights = init_lights();

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


	for(i = 0; i < CANVAS_HEIGHT; i++) {
		for(j = 0; j < CANVAS_WIDTH; j++) {
			int fb_offset = CANVAS_WIDTH * i * 4 + j * 4;


			// Canvas to world transformation
			float px = (2* ((j + 0.5) / CANVAS_WIDTH) - 1) * 
										tan(fov / 2 * M_PI / 180) * aspectRatio;
			float py = (1 - 2* ((i + 0.5) / CANVAS_HEIGHT)) * tan(fov / 2 * M_PI / 180);
			vec3 rayP = {.x = px, .y = py, .z = -1.0};

			// Ray direction
			vec3 dir = sub_vec3(rayP, orig);
			normalize_vec3(&dir);

			frameBuffer[fb_offset + 0] = 0;
			frameBuffer[fb_offset + 1] = 0;
			frameBuffer[fb_offset + 2] = 0;
			frameBuffer[fb_offset + 4] = SDL_ALPHA_OPAQUE;

			vec3 P;
			int index;

			// Check to see if there is an intersection between the camera ray and
			// all the objects
			int check = check_intersection(obj, spheres, &P, &index, orig, dir);			
			if(check != 0) {

				// For all the lights in the world, check to see if
				// the intersection point is lit by any of them
				for(l = 0; l < NUM_LIGHTS; l++) {

					// Ray from point to light
					vec3 rayDir = sub_vec3(lights[l], P);
					// normal of the object intersectd
					vec3 n = (check == 1) ? 
							  normal(obj.t[index]) :
							  sub_vec3(P, spheres.s[index].orig);
					// Color of the object intersected
					vec3 color = (check == 1) ?
								 obj.t[index].color :
								 spheres.s[index].color;

					normalize_vec3(&rayDir);
					normalize_vec3(&n);

					// Check to see if there isn't any objects in the way of the ray
					if(!check_intersection(obj, spheres, &P, &index, P, rayDir)) {
						// Calculate angle between the normal and the ray
						// so we can calculate brightness
						float l = length_vec3(n)*length_vec3(rayDir);
						float angle = dotProduct(n, rayDir) / l;

						if(angle > 0) {
							vec3 v = scale_vec3(color, angle);
							frameBuffer[fb_offset + 0] = clamp<unsigned char>(frameBuffer[fb_offset + 0] + (unsigned char)v.x, 0, 255);
							frameBuffer[fb_offset + 1] = clamp<unsigned char>(frameBuffer[fb_offset + 1] + (unsigned char)v.y, 0, 255);
							frameBuffer[fb_offset + 2] = clamp<unsigned char>(frameBuffer[fb_offset + 2] + (unsigned char)v.z, 0, 255);
						}
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

object init_objects() {

	object obj;
	obj.n_triangles = 6;
	obj.t = new triangle[obj.n_triangles];	//malloc(obj.n_triangles * sizeof(triangle));

	triangle t0 = {.p1 = {.x = 0.0, .y = -1.0, .z = -4.0},
				   .p2 = {.x = 0.0, .y = 0.0, .z = -4.0},
				   .p3 = {.x = -1.0, .y = 0.0, .z = -4.0},
				   .color = {.x = 0, .y = 255, .z = 0}};
	obj.t[0] = t0;

	triangle t1 = {.p1 = {.x = 0.0, .y = -1.0, .z = -4.0},
				   .p2 = {.x = 1.0, .y = 0.0, .z = -4.0},
				   .p3 = {.x = 0.0, .y = 0.0, .z = -4.0},
				   .color = {.x = 0, .y = 255, .z = 0}};
	obj.t[1] = t1;

	triangle t2 = {.p1 = {.x = 1.0, .y = 0.0, .z = -4.0},
				   .p2 = {.x = 0.0, .y = 1.0, .z = -4.0},
				   .p3 = {.x = 0.0, .y = 0.0, .z = -4.0},
				   .color = {.x = 0, .y = 255, .z = 0}};
	obj.t[2] = t2;

	triangle t3 = {.p1 = {.x = -1.0, .y =0.0, .z = -4.0},
				   .p2 = {.x = 0.0, .y = 0.0, .z = -4.0},
				   .p3 = {.x = 0.0, .y = 1.0, .z = -4.0},
				   .color = {.x = 0, .y = 255, .z = 0}};
	obj.t[3] = t3;

	triangle t4 = {.p1 = {.x = 10.0, .y = -5.0, .z = -2.0},
				   .p2 = {.x = 10.0, .y = -5.0, .z = -10.0},
				   .p3 = {.x = -10.0, .y = -5.0, .z = -2.0},
				   .color = {.x = 255, .y = 255, .z = 255}};
	obj.t[4] = t4;

	triangle t5 = {.p1 = {.x = -10.0, .y = -5.0, .z = -2.0},
				   .p2 = {.x = 10.0, .y = -5.0, .z = -10.0},
				   .p3 = {.x = -10.0, .y = -5.0, .z = -10.0},
				   .color = {.x = 255, .y = 255, .z = 255}};
	obj.t[5] = t5;


	return obj;
}

sphere_collection init_spheres() {
	sphere_collection s;

	s.n_spheres = 1;
	s.s = new sphere[s.n_spheres];

	sphere s0 = {.orig = {.x = 0, .y = 0, .z = -4.0}, 
						.color = {.x = 0, .y = 255, .z = 0}, 
						.radius = 0.5};
	s.s[0] = s0;

	return s;
}

vec3* init_lights() {
	vec3 *lights = new vec3[NUM_LIGHTS];


	vec3 light0 = {.x = 0, .y = 10, .z = -1.0};
	lights[0] = light0;

	// vec3 light1 = {.x = 0, .y = 10, .z = -1.0};
	// lights[1] = light1;

	return lights;
}

int check_intersection(object obj, sphere_collection spheres, vec3 *P, int *index, vec3 orig, vec3 dir) {
	int i, index_t, index_s;
	vec3 color = {.x = 0, .y = 0, .z = 0};
	vec3 p_triangle = {.x = FLT_MAX, .y = FLT_MAX, .z = FLT_MAX};
	vec3 p_sphere = {.x = FLT_MAX, .y = FLT_MAX, .z = FLT_MAX};

	for(i = 0; i < obj.n_triangles; i++) {
		if(rayTriangleIntersects(orig, dir, obj.t[i], P)) {
			if(length_vec3(p_triangle) > length_vec3(*P)) {
				index_t = i;
				p_triangle = *P;
			}
		}
	}

	for(i = 0; i < spheres.n_spheres; i++) {
		if(raySphereIntersects(orig, dir, spheres.s[i], P)) {
			if(length_vec3(p_sphere) > length_vec3(*P)) {
				index_s = i;
				p_sphere = *P;
			}
		}
	}

	int t_has_intersected = p_triangle.x != FLT_MAX && p_triangle.y != FLT_MAX && p_triangle.z != FLT_MAX;
	int s_has_intersected = p_sphere.x != FLT_MAX && p_sphere.y != FLT_MAX && p_sphere.z != FLT_MAX;
	if(!t_has_intersected && !s_has_intersected)
		return 0;
	else if(!t_has_intersected || length_vec3(p_triangle) > length_vec3(p_sphere)) {
		*P = p_sphere;
		*index = index_s;
		return 2;
	}
	else if(!s_has_intersected || length_vec3(p_triangle) <= length_vec3(p_sphere)){
		*P = p_triangle;
		*index = index_t;
		return 1;
	}
}
