#pragma once

#include <iostream>
#include "maths.hpp"
#include <limits.h>
#include <cfloat>
#include "omp.h"

#define CANVAS_HEIGHT 1080
#define CANVAS_WIDTH 1920

void render(unsigned char *frameBuffer, int fov, 
			float*** tris, unsigned char** color_tri, int t_size, 
			float** spheres, float* radius, unsigned char** color_sphere, int s_size, 
			float** lights, int l_size);

#pragma omp declare target
int check_intersection(float*** tris, int t_size, float** spheres, float* radius, int s_size, float *P, int *index, float *orig, float *dir);
#pragma omp end declare target