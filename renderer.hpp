#pragma once

#include <cfloat>
#include <iostream>
#include <limits.h>
#include <omp.h>

#include "maths.hpp"

#define CANVAS_HEIGHT 1440
#define CANVAS_WIDTH 2560
#define NUM_TRIANGLES 2
#define NUM_LIGHTS 1
#define NUM_SPHERES 900

void render(unsigned char *frameBuffer, int fov, float *tris,
            unsigned char *color_tri, int t_size, float *spheres, float *radius,
            unsigned char *color_sphere, int s_size, float *lights, int l_size);

#pragma omp declare target
bool CastRay(float orig[], float dir[], float *tris,
             unsigned char *color_tri, int t_size, float *spheres, float *radius,
             unsigned char *color_sphere, int s_size, float *lights,
             int l_size, unsigned char color[], int depth);

void fresnel(float I[], float N[], float *ior, float *kr);

void refract(float I[], float N[], float ior, float res[3]);

int check_intersection(float *tris, int t_size, float *spheres, float *radius,
                       int s_size, float *P, int *index, float *orig,
                       float *dir);
#pragma omp end declare target
