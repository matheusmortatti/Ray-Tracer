#pragma once

#include <stdio.h>
#include <iostream>
#include <math.h>
#include "omp.h"

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

template<class T>
T clamp(int value, int min, int max)
{
	return value < min ? min : (value > max ? max : value);
}

float* scale_vec(float* v, float t);
float* add_vec(float* v1, float* v2);
float* sub_vec(float* v1, float* v2);
float length(float* v);
void normalize(float *v);
float* normal(float* p1, float* p2, float* p3);
float* cross_product(float* v1, float* v2);
float dot_product(float* v1, float* v2);
int rayTriangleIntersects(float* orig, float* dir, float* p1, float* p2, float* p3, float* P);
int raySphereIntersects(float* orig, float* dir, float* s_orig, float radius, float* P);

vec3 scale_vec3(vec3 v, float f);
vec4 scale_vec4(vec4 v, float f);

void div_vec3(vec3 *v, float f);
void div_vec4(vec4 *v, float f);

vec3 add_vec3(vec3 v1, vec3 v2);
vec4 add_vec4(vec4 v1, vec4 v2);

vec3 sub_vec3(vec3 v1, vec3 v2);
vec4 sub_vec4(vec4 v1, vec4 v2);

float length_vec3(vec3 v);
float length_vec4(vec4 v);

void normalize_vec3(vec3 *v);
void normalize_vec4(vec4 *v);

vec3 cross_vec3(vec3 v1, vec3 v2);

vec3 normal(triangle t);

float dotProduct(vec3 v1, vec3 v2);

int rayTriangleIntersects(vec3 orig, vec3 dir, triangle tg, vec3 *P);
int raySphereIntersects(vec3 orig, vec3 dir, sphere s, vec3 *P);
#pragma omp end declare target

//////////////////////////////////////
// debug functions
//////////////////////////////////////

void print_vec3(vec3 v);