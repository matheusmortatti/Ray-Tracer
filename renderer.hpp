#pragma once

#include <cfloat>
#include <iostream>
#include <limits.h>
#include <omp.h>

#include "maths.hpp"

#define CANVAS_HEIGHT 1440
#define CANVAS_WIDTH 2560

void render(unsigned char *frameBuffer, int fov, float *tris,
            unsigned char *color_tri, int t_size, float *spheres, float *radius,
            unsigned char *color_sphere, int s_size, float *lights, int l_size);

void render_moving_camera(float speed[3], int iterations,
                          unsigned char *frameBuffer, int fov, float *tris,
                          unsigned char *color_tri, int t_size, float *spheres,
                          float *radius, unsigned char *color_sphere,
                          int s_size, float *lights, int l_size);

#pragma omp declare target
int check_intersection(float *tris, int t_size, float *spheres, float *radius,
                       int s_size, float *P, int *index, float *orig,
                       float *dir);
#pragma omp end declare target
