#ifndef MATHS_H
#define MATHS_H

#include <stdio.h>
#include <iostream>
#include <math.h>

typedef struct vec3 {
	float x, y, z;
} vec3;

typedef struct vec4 {
	float x, y, z, a;
} vec4;

typedef struct triangle {
	vec3 p1, p2, p3, color;
} triangle;

typedef struct sphere {
	vec3 orig, color;
	float radius;
} sphere;

typedef struct object {
	triangle *t;
	int n_triangles;
} object;

typedef struct sphere_collection {
	sphere *s;
	int n_spheres;
} sphere_collection;

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

//////////////////////////////////////
// debug functions
//////////////////////////////////////

void print_vec3(vec3 v);

#endif /* MATHS_H */