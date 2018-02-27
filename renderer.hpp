#pragma once

#include <iostream>
#include "maths.hpp"
#include <limits.h>
#include <cfloat>
#include "omp.h"

#define CANVAS_HEIGHT 1440
#define CANVAS_WIDTH 2560
#define NUM_TRIANGLES 2
#define NUM_LIGHTS 1
#define NUM_SPHERES 900

void render(unsigned char *frameBuffer, int fov, 
			float* tris, unsigned char* color_tri, int t_size, 
			float* spheres, float* radius, unsigned char* color_sphere, int s_size, 
			float* lights, int l_size);

#pragma omp declare target
int check_intersection(float* tris, int t_size, float* spheres, float* radius, int s_size, float *P, int *index, float *orig, float *dir);
#pragma omp end declare target
