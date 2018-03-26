#include "maths.hpp"

#pragma omp declare target

void scale_vec(float v[], float t, float res[]) {
  res[0] = v[0] * t;
  res[1] = v[1] * t;
  res[2] = v[2] * t;
}

void add_vec(float v1[], float v2[], float res[]) {
  res[0] = v1[0] + v2[0];
  res[1] = v1[1] + v2[1];
  res[2] = v1[2] + v2[2];
}

void sub_vec(float v1[], float v2[], float res[]) {
  res[0] = v1[0] - v2[0];
  res[1] = v1[1] - v2[1];
  res[2] = v1[2] - v2[2];
}

float length(float v[]) {
  return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

void normalize(float v[]) {
  float l = length(v);

  if (l == 0.0)
    return;
  else {
    v[0] /= l;
    v[1] /= l;
    v[2] /= l;
  }
}

void normal(float p1[], float p2[], float p3[], float res[]) {
  float a[3], b[3];

  sub_vec(p2, p1, a);
  sub_vec(p3, p1, b);
  cross_product(a, b, res);
  normalize(res);
}

void cross_product(float v1[], float v2[], float res[]) {
  res[0] = v1[1] * v2[2] - v1[2] * v2[1];
  res[1] = v1[2] * v2[0] - v1[0] * v2[2];
  res[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

float dot_product(float v1[], float v2[]) {
  return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

void swap(float *a, float *b) {
  float temp = *b;
  *b = *a;
  *a = temp;
}

int rayTriangleIntersects(float orig[], float dir[], float p1[], float p2[],
                          float p3[], float P[]) {
  float kEpsilon = 0.0001;

  float N[3];
  normal(p1, p2, p3, N);
  float NdotRayDirection = dot_product(N, dir);

  // Se o produto escalar for quase zero, são paralelos
  if (fabs(NdotRayDirection) < kEpsilon) {
    return false;
  }

  float d = dot_product(N, p1);
  float t = (dot_product(N, orig) + d) / NdotRayDirection;
  if (t < 0) {
    return false;
  }

  float scl[3];
  scale_vec(dir, t, scl);
  float add[3];
  add_vec(orig, scl, add);
  copy_array(P, add, 3);

  float C[3];

  float edge0[3];
  sub_vec(p2, p1, edge0);
  float vp0[3];
  sub_vec(P, p1, vp0);
  cross_product(edge0, vp0, C);
  if (dot_product(N, C) < 0) {
    return false;
  }

  float edge1[3];
  sub_vec(p3, p2, edge1);
  float vp1[3];
  sub_vec(P, p2, vp1);
  cross_product(edge1, vp1, C);
  if (dot_product(N, C) < 0) {
    return false;
  }

  float edge2[3];
  sub_vec(p1, p3, edge2);
  float vp2[3];
  sub_vec(P, p3, vp2);
  cross_product(edge2, vp2, C);
  if (dot_product(N, C) < 0) {
    return false;
  }

  return true;
}

int raySphereIntersects(float orig[], float dir[], float s_orig[], float radius,
                        float P[]) {
  float t0, t1;
  float kEpsilon = 0.001;
  float L[3];
  sub_vec(orig, s_orig, L);
  float a = dot_product(dir, dir);
  float b = 2 * dot_product(dir, L);
  float c = dot_product(L, L) - radius * radius;

  // Solve quadratic equation
  float discr = b * b - 4 * a * c;
  if (discr < 0) {
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
      return false;
    }
  }

  float scl[3];
  scale_vec(dir, t0, scl);
  float add[3];
  add_vec(orig, scl, add);
  copy_array(P, add, 3);

  return true;
}

void reflect(float dir[], float normal[], float refl[]) {
  float dot = dot_product(dir, normal);
  float m[3];
  scale_vec(normal, -2 * dot, m);
  add_vec(m, dir, refl);
}
#pragma omp end declare target

////////////////////////////////////
// Debug
////////////////////////////////////

void print_vec3(float *v) {
  printf("vector: x = %f, y = %f, z = %f\n", v[0], v[1], v[2]);
}