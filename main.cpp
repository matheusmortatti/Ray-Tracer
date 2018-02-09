#include <stdio.h>
#include <iostream>
#include <vector>
#include <limits.h>
#include <cfloat>
#include <thread>
#include <fstream>
#include <sstream>
//#include "SDL.h"
#include "omp.h"
#include "renderer.hpp"
#include "maths.hpp"

#define WITH_SDL false

void ReadSceneFile(std::string path, float** tris, unsigned char** t_colors, 
									 float** spheres, float** radius, unsigned char** s_colors,
									 float** lights,
									 int& t_size, int& s_size, int& l_size);
int init_triangles(float** tris, unsigned char** colors);
int init_spheres(float** spheres, float** radius, unsigned char** colors);
int init_lights(float** lights);

// void init_SDL(SDL_Window* &window, SDL_Renderer* &renderer);
// void put_pixel(SDL_Surface* screenSurface, int x, int y, vec3 color);

int main() {
	int fov = 120;
	unsigned char *frameBuffer;

	frameBuffer = new unsigned char[4 * CANVAS_HEIGHT * CANVAS_WIDTH];

	float *tris, *spheres, *radius, *lights;
	unsigned char *color_tri, *color_sphere;

	int t_size, s_size, l_size;

	ReadSceneFile( "scene.txt", &tris, &color_tri,
								&spheres, &radius, &color_sphere,
								&lights,
								t_size, s_size, l_size );

	//t_size = init_triangles(&tris, &color_tri);
	//s_size = init_spheres(&spheres, &radius, &color_sphere);
	//l_size = init_lights(&lights);

	// Create thread and start rendering
	std::thread render_thread(render, frameBuffer, fov, tris, color_tri, t_size, spheres, radius, color_sphere, s_size, lights, l_size);	


	// if (WITH_SDL)
	// {
	// 	/**
	// 	 * Create SDL Window
	// 	 **/
	// 	SDL_Window 	  *window 	= NULL;
	// 	SDL_Renderer  *renderer = NULL;
	// 	SDL_Texture   *texture 	= NULL;

	// 	init_SDL(window, renderer);

	// 	texture = SDL_CreateTexture (
	// 				        renderer,
	// 				        SDL_PIXELFORMAT_ARGB8888,
	// 				        SDL_TEXTUREACCESS_STREAMING,
	// 				        CANVAS_WIDTH, CANVAS_HEIGHT );

	// 	/**
	// 	 * Keep the screen up until the user closes it
	// 	 **/
	// 	bool quit = false;
	// 	while(!quit)
	// 	{
	// 		SDL_SetRenderDrawColor( renderer, 0, 0, 0, SDL_ALPHA_OPAQUE );
	//         SDL_RenderClear( renderer );

	// 		SDL_Event event;
	// 		while(SDL_PollEvent(&event))
	// 		{
	// 			switch(event.type)
	// 			{
	// 				case SDL_QUIT:
	// 					quit = true;
	// 					break;
	// 			}
	// 		}

	// 		/* Draw the image to the texture */
	// 		SDL_UpdateTexture(texture, NULL, &frameBuffer[0], CANVAS_WIDTH*4);
	// 		SDL_RenderCopy( renderer, texture, NULL, NULL );
	// 		SDL_RenderPresent(renderer);
	// 	}

	// 	SDL_DestroyRenderer( renderer );
	//     SDL_DestroyWindow( window );
	//     SDL_Quit();
	// }

    // Join thread to wait for it to end before exiting
    render_thread.join();

    if(!WITH_SDL)
    {
    	std::ofstream image;		
    	image.open("image.ppm");		
	    
	    image << "P3\n" << CANVAS_WIDTH << " " << CANVAS_HEIGHT << " " << 255 << std::endl;

	    for(int i = 0; i < CANVAS_HEIGHT; i++) {
			for(int j = 0; j < CANVAS_WIDTH; j++) {
				int fb_offset = CANVAS_WIDTH * i * 4 + j * 4;
				int z = (int)abs(frameBuffer[fb_offset + 0]),
					y = (int)abs(frameBuffer[fb_offset + 1]),
					x = (int)abs(frameBuffer[fb_offset + 2]);
				
				image << x << " " << y << " " << z << " ";
			}
			image << std::endl;
		}
	}

    delete[] tris;
    delete[] color_tri;

    delete[] spheres;
    delete[] color_sphere;
    delete[] radius;

    delete[] lights;

    delete[] frameBuffer;

	return 0;
}

// void put_pixel(SDL_Surface* screenSurface, int x, int y, vec3 color)
// {
// 	Uint8 *pixels = (Uint8*)screenSurface->pixels;
// 	Uint8 *pixel  = pixels + y * screenSurface->pitch + x;
// 	*pixel = SDL_MapRGB(screenSurface->format, 255, 255, 255);
// }

// void init_SDL(SDL_Window* &window, SDL_Renderer* &renderer)
// {

// 	if(SDL_Init(SDL_INIT_VIDEO) < 0)
// 	{
// 		std::cout << "SDL Could not initialize! SDL Error: " << SDL_GetError() << std::endl;
// 	}
// 	else
// 	{
// 		window = SDL_CreateWindow(
// 							"RayTracer", 
// 							SDL_WINDOWPOS_UNDEFINED, 
// 							SDL_WINDOWPOS_UNDEFINED,
// 							CANVAS_WIDTH, 
// 							CANVAS_HEIGHT,
// 							SDL_WINDOW_SHOWN);
// 		if(window == NULL)
// 		{
// 			std::cout << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
// 		}
// 		else
// 		{
// 			renderer = SDL_CreateRenderer (
// 						        window,
// 						        -1,
// 						        SDL_RENDERER_ACCELERATED
// 						        );
// 		}
// 	}
// }

void ReadSceneFile(std::string path, float** tris, unsigned char** t_colors, 
									 float** spheres, float** radius, unsigned char** s_colors,
									 float** lights,
									 int& t_size, int& s_size, int& l_size)
{
	std::ifstream file;
	file.open( path );

	if( !file.is_open() )
	{
		std::cerr << "Could not open file '" << path << "'" << std::endl;
	}

	std::vector<float> vtriangles;
	std::vector<float> vspheres;
	std::vector<float> vlights;
	while( !file.eof() )
	{
		std::string line;
		std::getline( file, line );

		if( line.front() == '/' )
			continue;
		
		std::istringstream stream(line);
		char type;
		stream >> type;

		float val;
		unsigned char col;

		switch( type )
		{
			case 's':
				stream >> val;
				vspheres.push_back( val );
				stream >> val;
				vspheres.push_back( val );
				stream >> val;
				vspheres.push_back( val );
				stream >> val;
				vspheres.push_back( val );
				stream >> val;
				vspheres.push_back( val );
				stream >> val;
				vspheres.push_back( val );
				stream >> val;
				vspheres.push_back( val );
				break;
			
			case 't':
				stream >> val;
				vtriangles.push_back( val );
				stream >> val;
				vtriangles.push_back( val );
				stream >> val;
				vtriangles.push_back( val );
				stream >> val;
				vtriangles.push_back( val );
				stream >> val;
				vtriangles.push_back( val );
				stream >> val;
				vtriangles.push_back( val );
				stream >> val;
				vtriangles.push_back( val );
				stream >> val;
				vtriangles.push_back( val );
				stream >> val;
				vtriangles.push_back( val );
				stream >> val;
				vtriangles.push_back( val );
				stream >> val;
				vtriangles.push_back( val );
				stream >> val;
				vtriangles.push_back( val );
				break;

			case 'l':
				stream >> val;
				vlights.push_back( val );
				stream >> val;
				vlights.push_back( val );
				stream >> val;
				vlights.push_back( val );
				break;
		}		
	}

	t_size = vtriangles.size() / 12;
	s_size = vspheres.size() / 7;
	l_size = vlights.size() / 3;	

	(*tris) 	= new float[t_size * 9];
	(*t_colors)	= new unsigned char[t_size * 3];
	(*spheres)  = new float[s_size * 3];
	(*radius)   = new float[s_size * 1];
	(*s_colors) = new unsigned char[s_size * 3];
	(*lights)   = new float[l_size * 3];

	for( int i = 0; i < t_size; i++ )
	{
		(*tris)[i * 9 + 0] = vtriangles[i * 12 + 0];
		(*tris)[i * 9 + 1] = vtriangles[i * 12 + 1];
		(*tris)[i * 9 + 2] = vtriangles[i * 12 + 2];
		(*tris)[i * 9 + 3] = vtriangles[i * 12 + 3];
		(*tris)[i * 9 + 4] = vtriangles[i * 12 + 4];
		(*tris)[i * 9 + 5] = vtriangles[i * 12 + 5];
		(*tris)[i * 9 + 6] = vtriangles[i * 12 + 6];
		(*tris)[i * 9 + 7] = vtriangles[i * 12 + 7];
		(*tris)[i * 9 + 8] = vtriangles[i * 12 + 8];
		(*t_colors)[i * 3 + 0] = (unsigned char)vtriangles[i * 12 + 9];
		(*t_colors)[i * 3 + 1] = (unsigned char)vtriangles[i * 12 + 10];
		(*t_colors)[i * 3 + 2] = (unsigned char)vtriangles[i * 12 + 11];
	}

	for( int i = 0; i < s_size; i++ )
	{
		(*radius)[i * 1 + 0] = vspheres[i * 7 + 0];
		(*spheres)[i * 3 + 0] = vspheres[i * 7 + 1];
		(*spheres)[i * 3 + 1] = vspheres[i * 7 + 2];
		(*spheres)[i * 3 + 2] = vspheres[i * 7 + 3];
		(*s_colors)[i * 3 + 0] = (unsigned char)vspheres[i * 7 + 4];
		(*s_colors)[i * 3 + 1] = (unsigned char)vspheres[i * 7 + 5];
		(*s_colors)[i * 3 + 2] = (unsigned char)vspheres[i * 7 + 6];			
	}

	for( int i = 0; i < l_size; i++ )
	{		
		(*lights)[i * 3 + 0] = vlights[i * 3 + 0];
		(*lights)[i * 3 + 1] = vlights[i * 3 + 1];
		(*lights)[i * 3 + 2] = vlights[i * 3 + 2];						
	}
}

int init_triangles(float** tris, unsigned char** colors)
{
	int t_size = 2;

	(*tris) 	= new float[t_size * 9];
	(*colors) 	= new unsigned char[t_size];

	float v[2][9] = {
						    {
						    	10.0, -5.0, -2.0,
						    	10.0, -5.0, -10.0,
						    	-10.0, -5.0, -2.0,
						    },
						    {
						    	-10.0, -5.0, -2.0,
						    	10.0, -5.0, -10.0,
						    	-10.0, -5.0, -10.0,
						    }
					    };

	unsigned char c[2][3] = {
							    {
							    	255, 125, 125
							    },
							    {
							    	255, 125, 125
							    }
						    };

	for(int i = 0; i < t_size; i++)
	{
		(*tris)[i * 9 + 0] = v[i][0];
		(*tris)[i * 9 + 1] = v[i][1];
		(*tris)[i * 9 + 2] = v[i][2];
		(*tris)[i * 9 + 3] = v[i][3];
		(*tris)[i * 9 + 4] = v[i][4];
		(*tris)[i * 9 + 5] = v[i][5];
		(*tris)[i * 9 + 6] = v[i][6];
		(*tris)[i * 9 + 7] = v[i][7];
		(*tris)[i * 9 + 8] = v[i][8];
		(*colors)[i * 3 + 0] = c[i][0];
		(*colors)[i * 3 + 1] = c[i][1];
		(*colors)[i * 3 + 2] = c[i][2];
	}

	return t_size;
}

int init_spheres(float** spheres, float** radius, unsigned char** colors)
{
	int s_size = 1;

	(*spheres) = new float[s_size * 3];
	(*radius)  = new float[s_size];
	(*colors) = new unsigned char[s_size * 3];

	float v[1][3] = {
					    {
					    	0.0, 0.0, -4.0
					    }
				    };

	float r[1] = { 1.0 };

	unsigned char c[1][3] = {
							    {
							    	0, 255, 0
							    }
						    };

	for(int i = 0; i < s_size; i++)
	{
		(*spheres)[i * 3 + 0] = v[i][0];
		(*spheres)[i * 3 + 1] = v[i][1];
		(*spheres)[i * 3 + 2] = v[i][2];

		(*radius)[i] = r[i];

		(*colors)[i * 3 + 0] = c[i][0];
		(*colors)[i * 3 + 1] = c[i][1];
		(*colors)[i * 3 + 2] = c[i][2];
	}

	return s_size;
}

int init_lights(float** lights)
{
	int l_size = 2;

	(*lights) = new float[l_size * 3];

	float v[2][3] = {
					    {
					    	2.0, 5.0, -4.0
					    },
					    {
					    	-2.0, 5.0, -4.0
					    }
				    };

	for(int i = 0; i < l_size; i++)
	{
		(*lights)[i * 3 + 0] = v[i][0];
		(*lights)[i * 3 + 1] = v[i][1];
		(*lights)[i * 3 + 2] = v[i][2];
	}

	return l_size;
}