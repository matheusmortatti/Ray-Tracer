#pragma once

#include <iostream>
#include <math.h>
#include <omp.h>
#include <stdio.h>

#pragma omp declare target
struct vec3 {
  float x, y, z;
};

struct vec4 {
  float x, y, z, a;
};

struct triangle {
  vec3 p1, p2, p3, color;
};

struct sphere {
  vec3 orig, color;
  float radius;
};

template <class T> T clamp(T value, T min, T max) {
  return value < min ? min : (value > max ? max : value);
}

float *scale_vec(float *v, float t);
float *add_vec(float *v1, float *v2);
float *sub_vec(float *v1, float *v2);
float length(float *v);
void normalize(float *v);
float *normal(float *p1, float *p2, float *p3);
float *cross_product(float *v1, float *v2);
float dot_product(float *v1, float *v2);

int rayTriangleIntersects(float *orig, float *dir, float *p1, float *p2,
                          float *p3, float *P);
int raySphereIntersects(float *orig, float *dir, float *s_orig, float radius,
                        float *P);

void copy_array(float *to, float *from, int size);

#pragma omp end declare target

//////////////////////////////////////
// debug functions
//////////////////////////////////////

void print_vec3(float *v);
