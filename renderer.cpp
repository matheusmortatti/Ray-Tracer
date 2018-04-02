#include "renderer.hpp"

#define KS 0.3
#define KD 0.7
#define SPEC_HIGHLIGHT 20 // the bigger, the smaller the highlight will be
#define REFLECTION 0.5
#define REFLECTION_DEPTH 3

void render(unsigned char *frameBuffer, int fov, float *tris,
            unsigned char *color_tri, int t_size, float *spheres, float *radius,
            unsigned char *color_sphere, int s_size, float *lights,
            int l_size)
{
  float aspectRatio = (float)CANVAS_WIDTH / (float)CANVAS_HEIGHT;

// #pragma omp target map(                              \
//     to                                               \
//     : fov, aspectRatio,                              \
//       tris[:t_size * 9],                             \
//                        color_tri                     \
//           [:t_size * 3],                             \
//                        spheres                       \
//           [:s_size * 3],                             \
//                        radius                        \
//           [:s_size],                                 \
//                    color_sphere                      \
//           [:s_size * 3],                             \
//                        lights                        \
//           [:l_size *                                 \
//             3]) map(from                             \
//                     : frameBuffer                    \
//                     [:4 *                            \
//                       CANVAS_HEIGHT * CANVAS_WIDTH]) \
//     device(0)
#pragma omp parallel for collapse(1) schedule(dynamic) shared(frameBuffer)
  for (int i = 0; i < CANVAS_HEIGHT; i++)
  {
#pragma omp target data map(from                              \
                            : frameBuffer [CANVAS_WIDTH * i * \
                                4:CANVAS_WIDTH * (i + 1) * 4])
    for (int j = 0; j < CANVAS_WIDTH; j++)
    {

      // Canvas to world transformation
      float px = (2 * ((j + 0.5) / (float)CANVAS_WIDTH) - 1) *
                 tan(fov * 0.5 * M_PI / 180) * aspectRatio;
      float py = (1 - 2 * ((i + 0.5) / (float)CANVAS_HEIGHT)) *
                 tan(fov * 0.5 * M_PI / 180);
      float rayP[3] = {px, py, -1.0};
      float orig[3] = {0.0, 0.0, 3.0};

      // Ray direction
      float dir[3];
      sub_vec(rayP, orig, dir);
      normalize(dir);

      // Get transformed framebuffer index
      int fb_offset = CANVAS_WIDTH * i * 4 + j * 4;

      unsigned char color[3];

      CastRay(orig, dir, tris, color_tri, t_size, spheres, radius, color_sphere, s_size, lights, l_size, color, 0);

      frameBuffer[fb_offset + 0] = color[0];
      frameBuffer[fb_offset + 1] = color[1];
      frameBuffer[fb_offset + 2] = color[2];
      frameBuffer[fb_offset + 3] = -1;
    }
  }
}

#pragma omp declare target

bool CastRay(float orig[], float dir[], float *tris,
             unsigned char *color_tri, int t_size, float *spheres, float *radius,
             unsigned char *color_sphere, int s_size, float *lights,
             int l_size, unsigned char color[], int depth)
{
  float P[3];
  int index;
  color[0] = 0;
  color[1] = 0;
  color[2] = 0;

  // Check to see if there is an intersection between the camera ray and
  // all the objects
  int check = check_intersection(tris, t_size, spheres, radius, s_size, P,
                                 &index, orig, dir);
  if (check != 0)
  {
    float n[3];
    unsigned char obj_color[3];
    switch (check)
    {
    case 1:
      normal(tris + index * 9, tris + index * 9 + 3, tris + index * 9 + 6,
             n);
      copy_array<unsigned char>(obj_color, color_tri + index * 3, 3);
      // obj_color[0] *= P[0] / 100;
      // obj_color[1] *= P[1] / 100;
      // obj_color[2] *= P[2] / 100;
      break;
    case 2:
      sub_vec(P, spheres + index * 3, n);
      copy_array<unsigned char>(obj_color, color_sphere + index * 3, 3);
      break;
    }
    normalize(n);

    // For all the lights in the world, check to see if
    // the intersection point is lit by any of them
    for (int l = 0; l < l_size; l++)
    {

      // Ray from point to light
      float rayDir[3];
      sub_vec(lights + l * 3, P, rayDir);
      normalize(rayDir);

      // std::cout << (n)[0] << " " << (n)[1] << " " << (n)[2] << std::endl;

      // Calculate angle between the normal and the ray
      // so we can calculate brightness
      float angle = dot_product(n, rayDir);
      angle = angle < 0 ? 0 : angle;
      unsigned char specular[3] = {0, 0, 0};

      float zero[3] = {0, 0, 0};
      float ml[3];
      sub_vec(zero, rayDir, ml);
      float r[3];
      reflect(ml, n, r);
      float ndir[3];
      sub_vec(zero, dir, ndir);
      float lang = dot_product(r, ndir);
      lang = lang < 0 ? 0 : lang;
      float s = pow(lang, SPEC_HIGHLIGHT);

      specular[0] = obj_color[0] * s;
      specular[1] = obj_color[1] * s;
      specular[2] = obj_color[2] * s;

      float P1[3];
      float fColor[3] = {specular[0] * KS + obj_color[0] * KD,
                         specular[1] * KS + obj_color[1] * KD,
                         specular[2] * KS + obj_color[2] * KD};

      // If there are no objects in the way of the ray, the point is lit by
      // the light
      if (check_intersection(tris, t_size, spheres, radius, s_size, P1,
                             &index, P, rayDir) == 0)
      {
        color[0] = clamp<uint16_t>(
            color[0] + fColor[0] * angle, 0, 255);
        color[1] = clamp<uint16_t>(
            color[1] + fColor[1] * angle, 0, 255);
        color[2] = clamp<uint16_t>(
            color[2] + fColor[2] * angle, 0, 255);
      }
    }

    // Compute reflection
    if (depth < REFLECTION_DEPTH - 1 && check == 2)
    {
      bool outside = dot_product(n, dir) < 0;
      unsigned char refl_color[3];
      unsigned char refr_color[3];
      float refl_vec[3];
      reflect(dir, n, refl_vec);
      float kr, kt;
      float ior = 0.2;
      fresnel(dir, n, &ior, &kr);
      kt = 1 - kr;

      CastRay(P, refl_vec, tris, color_tri, t_size, spheres, radius, color_sphere, s_size, lights, l_size, refl_color, depth + 1);

      // Compute refraction
      if (kr < 1)
      {
        float refr_vec[3];
        refract(dir, n, ior, refr_vec);
        CastRay(P, dir, tris, color_tri, t_size, spheres, radius, color_sphere, s_size, lights, l_size, refr_color, depth + 1);
      }

      color[0] = kt * refr_color[0] + kr * refl_color[0];
      color[1] = kt * refr_color[1] + kr * refl_color[1];
      color[2] = kt * refr_color[2] + kr * refl_color[2];
    }

    if (depth == REFLECTION_DEPTH - 1)
    {
      return true;
    }
  }

  return check != 0;
}

void fresnel(float I[], float N[], float *ior, float *kr)
{
  float cosi = clamp<float>(-1, 1, dot_product(I, N));
  float etai = 1, etat = *ior;
  if (cosi > 0)
  {
    std::swap(etai, etat);
  }
  // Compute sini using Snell's law
  float sint = etai / etat * sqrtf(std::max(0.f, 1 - cosi * cosi));
  // Total internal reflection
  if (sint >= 1)
  {
    *kr = 1;
  }
  else
  {
    float cost = sqrtf(std::max(0.f, 1 - sint * sint));
    cosi = fabsf(cosi);
    float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
    float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
    *kr = (Rs * Rs + Rp * Rp) / 2;
  }
  // As a consequence of the conservation of energy, transmittance is given by:
  // kt = 1 - kr;
}

void refract(float I[], float N[], float ior, float res[3])
{
  float cosi = clamp<float>(-1, 1, dot_product(I, N));
  float etai = 1, etat = ior;
  float n[3];
  copy_array<float>(n, N, 3);
  if (cosi < 0)
  {
    cosi = -cosi;
  }
  else
  {
    std::swap(etai, etat);
    n[0] = -n[0];
    n[1] = -n[1];
    n[2] = -n[2];
  }
  float eta = etai / etat;
  float k = 1 - eta * eta * (1 - cosi * cosi);

  if (k < 0)
  {
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
  }
  else
  {
    float r1[3], r2[3];
    scale_vec(n, (eta * cosi - sqrtf(k)), r1);
    scale_vec(I, eta, r2);
    add_vec(r1, r2, res);
  }
}

int check_intersection(float *tris, int t_size, float *spheres, float *radius,
                       int s_size, float *P, int *index, float *orig,
                       float *dir)
{
  int i, index_tri, index_s;

  int t_has_intersected = false;
  int s_has_intersected = false;

  float p_triangle[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
  float p_sphere[3] = {FLT_MAX, FLT_MAX, FLT_MAX};

  for (i = 0; i < t_size; i++)
  {
    if (rayTriangleIntersects(orig, dir, tris + i * 9, tris + i * 9 + 3,
                              tris + i * 9 + 6, P))
    {
      if (length(p_triangle) > length(P))
      {
        index_tri = i;
        t_has_intersected = true;

        copy_array(p_triangle, P, 3);
      }
    }
  }

  for (i = 0; i < s_size; i++)
  {
    if (raySphereIntersects(orig, dir, spheres + i * 3, radius[i], P))
    {
      if (length(p_sphere) > length(P))
      {
        index_s = i;
        s_has_intersected = true;

        copy_array(p_sphere, P, 3);
      }
    }
  }

  if (!t_has_intersected && !s_has_intersected)
    return 0;
  if (length(p_triangle) <= length(p_sphere))
  {
    copy_array(P, p_triangle, 3);
    *index = index_tri;
    return 1;
  }
  if (length(p_triangle) > length(p_sphere))
  {
    copy_array(P, p_sphere, 3);
    *index = index_s;
    return 2;
  }

  return 0;
}
#pragma omp end declare target
