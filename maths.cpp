#include "maths.hpp"

#pragma omp declare target

float *scale_vec(float *v, float t) {
  float *res = new float[3];

  res[0] = v[0] * t;
  res[1] = v[1] * t;
  res[2] = v[2] * t;

  return res;
}

float *add_vec(float *v1, float *v2) {
  float *res = new float[3];

  res[0] = v1[0] + v2[0];
  res[1] = v1[1] + v2[1];
  res[2] = v1[2] + v2[2];

  return res;
}

float *sub_vec(float *v1, float *v2) {
  float *res = new float[3];

  res[0] = v1[0] - v2[0];
  res[1] = v1[1] - v2[1];
  res[2] = v1[2] - v2[2];

  return res;
}

float length(float *v) { return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }

void normalize(float *v) {
  float l = length(v);

  if (l == 0.0)
    return;
  else {
    v[0] /= l;
    v[1] /= l;
    v[2] /= l;
  }
}

float *normal(float *p1, float *p2, float *p3) {
  float *a, *b, *c;

  a = sub_vec(p2, p1);
  b = sub_vec(p3, p1);
  c = cross_product(a, b);
  normalize(c);

  delete[] a;
  delete[] b;

  return c;
}

float *cross_product(float *v1, float *v2) {
  float *res = new float[3];

  res[0] = v1[1] * v2[2] - v1[2] * v2[1];
  res[1] = v1[2] * v2[0] - v1[0] * v2[2];
  res[2] = v1[0] * v2[1] - v1[1] * v2[0];

  return res;
}

float dot_product(float *v1, float *v2) {
  return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

void swap(float *a, float *b) {
  float temp = *b;
  *b = *a;
  *a = temp;
}

int rayTriangleIntersects(float *orig, float *dir, float *p1, float *p2,
                          float *p3, float *P) {
  float kEpsilon = 0.0001;

  float *N = normal(p1, p2, p3);
  float NdotRayDirection = dot_product(N, dir);

  // Se o produto escalar for quase zero, são paralelos
  if (fabs(NdotRayDirection) < kEpsilon) {
    delete[] N;
    return false;
  }

  float d = dot_product(N, p1);
  float t = (dot_product(N, orig) + d) / NdotRayDirection;
  if (t < 0) {
    delete[] N;
    return false;
  }

  float *scl = scale_vec(dir, t);
  float *add = add_vec(orig, scl);
  copy_array(P, add, 3);
  delete[] scl;
  delete[] add;

  float *C;

  float *edge0 = sub_vec(p2, p1);
  float *vp0 = sub_vec(P, p1);
  C = cross_product(edge0, vp0);
  if (dot_product(N, C) < 0) {
    delete[] C;
    delete[] N;
    delete[] edge0;
    delete[] vp0;
    return false;
  }
  delete[] C;

  float *edge1 = sub_vec(p3, p2);
  float *vp1 = sub_vec(P, p2);
  C = cross_product(edge1, vp1);
  if (dot_product(N, C) < 0) {
    delete[] C;
    delete[] N;
    delete[] edge0;
    delete[] vp0;
    delete[] edge1;
    delete[] vp1;
    return false;
  }
  delete[] C;

  float *edge2 = sub_vec(p1, p3);
  float *vp2 = sub_vec(P, p3);
  C = cross_product(edge2, vp2);
  if (dot_product(N, C) < 0) {
    delete[] C;
    delete[] N;
    delete[] edge0;
    delete[] vp0;
    delete[] edge1;
    delete[] vp1;
    delete[] edge2;
    delete[] vp2;
    return false;
  }

  delete[] C;
  delete[] N;
  delete[] edge0;
  delete[] vp0;
  delete[] edge1;
  delete[] vp1;
  delete[] edge2;
  delete[] vp2;

  return true;
}

int raySphereIntersects(float *orig, float *dir, float *s_orig, float radius,
                        float *P) {
  float t0, t1;
  float kEpsilon = 0.001;
  float *L = sub_vec(orig, s_orig);
  float a = dot_product(dir, dir);
  float b = 2 * dot_product(dir, L);
  float c = dot_product(L, L) - radius * radius;

  // Solve quadratic equation
  float discr = b * b - 4 * a * c;
  if (discr < 0) {
    delete[] L;
    return false;
  } else if (discr == 0)
    t0 = t1 = -0.5 * b / a;
  else {
    float q =
        (b > kEpsilon) ? -0.5 * (b + sqrt(discr)) : -0.5 * (b - sqrt(discr));
    t0 = q / a;
    t1 = c / q;
  }

  if (t0 >= t1)
    swap(&t0, &t1);

  if (t0 <= kEpsilon) {
    t0 = t1;
    if (t0 <= kEpsilon) {
      delete[] L;
      return false;
    }
  }

  float *scl = scale_vec(dir, t0);
  float *add = add_vec(orig, scl);
  copy_array(P, add, 3);

  delete[] scl;
  delete[] add;
  delete[] L;
  return true;
}

void copy_array(float *to, float *from, int size) {
  for (int i = 0; i < size; i++)
    to[i] = from[i];
}
#pragma omp end declare target

////////////////////////////////////
// Debug
////////////////////////////////////

void print_vec3(float *v) {
  printf("vector: x = %f, y = %f, z = %f\n", v[0], v[1], v[2]);
}