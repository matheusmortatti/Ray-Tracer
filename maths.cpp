#include "maths.hpp"

#pragma omp declare target
vec3 scale_vec3(vec3 v, float f) {
	vec3 res = {.x = v.x * f,
				.y = v.y * f,
				.z = v.z * f};
	
	return res;
}

vec4 scale_vec4(vec4 v, float f) {
	v.x *= f;
	v.y *= f;
	v.z *= f;
	v.a *= f;
	return v;
}

void div_vec3(vec3 *v, float f) {
	if(f != 0.0) {
		v->x /= f;
		v->y /= f;
		v->z /= f;
	}
}

void div_vec4(vec4 *v, float f) {
	if(f != 0.0) {
		v->x /= f;
		v->y /= f;
		v->z /= f;
		v->a /= f;
	}
}

vec3 add_vec3(vec3 v1, vec3 v2) {
	vec3 res;

	res.x = v1.x + v2.x;
	res.y = v1.y + v2.y;
	res.z = v1.z + v2.z;

	return res;
}

vec4 add_vec4(vec4 v1, vec4 v2) {
	vec4 res;

	res.x = v1.x + v2.x;
	res.y = v1.y + v2.y;
	res.z = v1.z + v2.z;
	res.a = v1.a + v2.a;

	return res;
}

vec3 sub_vec3(vec3 v1, vec3 v2) {
	vec3 res;
	res.x = v1.x - v2.x;
	res.y = v1.y - v2.y;
	res.z = v1.z - v2.z;

	return res;
}

vec4 sub_vec4(vec4 v1, vec4 v2) {
	vec4 res;
	res.x = v1.x - v2.x;
	res.y = v1.y - v2.y;
	res.z = v1.z - v2.z;
	res.a = v1.a - v2.a;

	return res;
}

float length_vec3(vec3 v) {
	return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}


float length_vec4(vec4 v) {
	return sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.a * v.a);
}

void normalize_vec3(vec3 *v) {
	float l = length_vec3(*v);

	if (l == 0.0) return;
	else 		  div_vec3(v, l);
}

void normalize_vec4(vec4 *v) {
	float l = length_vec4(*v);

	if (l == 0.0) return;
	else 		  div_vec4(v, l);
}

vec3 cross_vec3(vec3 v1, vec3 v2) {
	vec3 res;

	res.x = v1.y * v2.z - v1.z * v2.y;
	res.y = v1.z * v2.x - v1.x * v2.z;
	res.z = v1.x * v2.y - v1.y * v2.x;

	return res;
}

vec3 normal(triangle t) {
	vec3 a, b, c;
	a = sub_vec3(t.p2, t.p1);
	b = sub_vec3(t.p3, t.p1);
	c = cross_vec3(a, b);
	normalize_vec3(&c);

	return c;
}

float dotProduct(vec3 v1, vec3 v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

void swap(float *a, float *b) {
	float temp = *b;
	*b = *a;
	*a = temp;
}

int rayTriangleIntersects(vec3 orig, vec3 dir, triangle tg, vec3 *P) {

	float kEpsilon = 0.00001;
	vec3 N = normal(tg);
	float NdotRayDirection = dotProduct(N, dir);

	// Se o produto escalar for quase zero, são paralelos
	if(fabs(NdotRayDirection) < kEpsilon)
		return 0;

	float d = dotProduct(N, tg.p1);
	float t = (dotProduct(N, orig) + d) / NdotRayDirection;
	if (t < 0) return 0;


	*P = add_vec3(orig, scale_vec3(dir, t));
	vec3 C;

	vec3 edge0 = sub_vec3(tg.p2, tg.p1);
	vec3 vp0 = sub_vec3(*P, tg.p1);
	C = cross_vec3(edge0, vp0);
	if(dotProduct(N, C) < 0) return 0;


	vec3 edge1 = sub_vec3(tg.p3, tg.p2);
	vec3 vp1 = sub_vec3(*P, tg.p2);
	C = cross_vec3(edge1, vp1);
	if(dotProduct(N, C) < 0) return 0;


	vec3 edge2 = sub_vec3(tg.p1, tg.p3);
	vec3 vp2 = sub_vec3(*P, tg.p3);
	C = cross_vec3(edge2, vp2);
	if(dotProduct(N, C) < 0) return 0;

	return 1;
}

int raySphereIntersects(vec3 orig, vec3 dir, sphere s, vec3 *P) {
	float t0, t1;
	float kEpsilon = 0.001;
	vec3 L = sub_vec3(orig, s.orig);
	float a = dotProduct(dir, dir);
	float b = 2 * dotProduct(dir, L);
	float c = dotProduct(L, L) - s.radius * s.radius;

	// Solve quadratic equation
	float discr = b * b - 4 * a * c;
	if(discr < 0) return 0;
	else if(discr == 0) t0 = t1 = - 0.5 * b / a;
	else {
		float q = (b > kEpsilon) ? 
				  	-0.5 * (b + sqrt(discr)) :
				  	-0.5 * (b - sqrt(discr));
		t0 = q / a;
		t1 = c / q;
	}

	if(t0 >= t1) swap(&t0, &t1);

	if(t0 <= kEpsilon) {
		t0 = t1;
		if(t0 <= kEpsilon) return 0;
	}

	*P = add_vec3(orig, scale_vec3(dir, t0));

	return 1;
}
#pragma omp end declare target
////////////////////////////////////
// Debug
////////////////////////////////////

void print_vec3(vec3 v) {
	printf("vector: x = %d, y = %d, z = %d\n", (int)v.x, (int)v.y, (int)v.z);
}