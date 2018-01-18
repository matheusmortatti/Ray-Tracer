#pragma once

#include <iostream>
#include "maths.hpp"
#include <limits.h>
#include <cfloat>
#include "omp.h"

#define CANVAS_HEIGHT 1080
#define CANVAS_WIDTH 1920
#define NUM_LIGHTS 2
#define NUM_SPHERES 1
#define NUM_TRIANGLES 3

void render(unsigned char *frameBuffer, int fov, triangle *tris, sphere *spheres, vec3 *lights);

#pragma omp declare target
int check_intersection(triangle *tris, int t_size, sphere *spheres, int s_size, vec3 *P, int *index, vec3 orig, vec3 dir);
#pragma omp end declare target