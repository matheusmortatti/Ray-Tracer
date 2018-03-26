#include "renderer.hpp"

#define TILE_WIDTH 300
#define TILE_HEIGHT 300
#define KS 0.3
#define KD 0.7
#define SPEC_HIGHLIGHT 20 // the bigger, the smaller the highlight will be

void render(unsigned char *frameBuffer, int fov, float *tris,
            unsigned char *color_tri, int t_size, float *spheres, float *radius,
            unsigned char *color_sphere, int s_size, float *lights,
            int l_size) {
  float aspectRatio = (float)CANVAS_WIDTH / (float)CANVAS_HEIGHT;

  int tileWidth = 300;
  int tileHeight = 300;

  unsigned char *tile_buffer = new unsigned char[4 * TILE_WIDTH * TILE_HEIGHT];
  for (int offy = 0; offy < CANVAS_HEIGHT; offy += tileHeight) {
    for (int offx = 0; offx < CANVAS_WIDTH; offx += tileWidth) {
      int height =
          offy + tileHeight > CANVAS_HEIGHT ? CANVAS_HEIGHT - offy : tileHeight;
      int width =
          offx + tileWidth > CANVAS_WIDTH ? CANVAS_WIDTH - offx : tileWidth;

#pragma omp target map(                                                        \
    to : width, height, fov, aspectRatio, offx, offy,                          \
    tris[ : t_size * 9],                                                       \
          color_tri[ : t_size * 3],                                            \
                     spheres[ : s_size * 3],                                   \
                              radius[ : s_size],                               \
                                      color_sphere[ : s_size * 3],             \
                                                    lights[ : l_size *         \
                                                            3]) map(           \
                                                        from : frameBuffer     \
                                                        [ : 4 *                \
                                                          CANVAS_HEIGHT        \
                                                              *CANVAS_WIDTH])  \
                                                            device(0)
#pragma omp parallel for collapse(1) schedule(dynamic) shared(frameBuffer)
      for (int i = 0; i < height; i++) {
#pragma omp target data map(from : frameBuffer[CANVAS_WIDTH *i *               \
                                               4 : CANVAS_WIDTH *(i + 1) * 4])
        for (int j = 0; j < width; j++) {

          // Canvas to world transformation
          float px = (2 * ((j + offx + 0.5) / (float)CANVAS_WIDTH) - 1) *
                     tan(fov * 0.5 * M_PI / 180) * aspectRatio;
          float py = (1 - 2 * ((i + offy + 0.5) / (float)CANVAS_HEIGHT)) *
                     tan(fov * 0.5 * M_PI / 180);
          float rayP[3] = {px, py, -1.0};
          float orig[3] = {0.0, 0.0, 1.0};

          // Ray direction
          float dir[3];
          sub_vec(rayP, orig, dir);
          normalize(dir);

          // Get transformed framebuffer index
          int fb_offset = width * i * 4 + j * 4;

          // Initialize framebuffer pixel color
          tile_buffer[fb_offset + 0] = 0;
          tile_buffer[fb_offset + 1] = 0;
          tile_buffer[fb_offset + 2] = 0;
          tile_buffer[fb_offset + 3] = -1;

          float P[3];
          int index;

          // Check to see if there is an intersection between the camera ray and
          // all the objects
          int check = check_intersection(tris, t_size, spheres, radius, s_size,
                                         P, &index, orig, dir);
          if (check != 0) {
            float n[3];
            unsigned char color[3];
            switch (check) {
            case 1:
              normal(tris + index * 9, tris + index * 9 + 3,
                     tris + index * 9 + 6, n);
              copy_array<unsigned char>(color, color_tri + index * 3, 3);
              break;
            case 2:
              sub_vec(P, spheres + index * 3, n);
              copy_array<unsigned char>(color, color_sphere + index * 3, 3);
              break;
            }
            normalize(n);

#if !UNLIT

            // For all the lights in the world, check to see if
            // the intersection point is lit by any of them
            for (int l = 0; l < l_size; l++) {

              // Ray from point to light
              float rayDir[3];
              sub_vec(lights + l * 3, P, rayDir);
              normalize(rayDir);

              // std::cout << (n)[0] << " " << (n)[1] << " " << (n)[2] <<
              // std::endl;

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

              specular[0] = color[0] * s;
              specular[1] = color[1] * s;
              specular[2] = color[2] * s;

              float P1[3];
              float fColor[3] = {specular[0] * KS + color[0] * KD,
                                 specular[1] * KS + color[1] * KD,
                                 specular[2] * KS + color[2] * KD};

              // If there are no objects in the way of the ray, the point is lit
              // by
              // the light
              if (check_intersection(tris, t_size, spheres, radius, s_size, P1,
                                     &index, P, rayDir) == 0) {
                tile_buffer[fb_offset + 0] = clamp<uint16_t>(
                    tile_buffer[fb_offset + 0] + fColor[0] * angle, 0, 255);
                tile_buffer[fb_offset + 1] = clamp<uint16_t>(
                    tile_buffer[fb_offset + 1] + fColor[1] * angle, 0, 255);
                tile_buffer[fb_offset + 2] = clamp<uint16_t>(
                    tile_buffer[fb_offset + 2] + fColor[2] * angle, 0, 255);
              }
            }

#else
            tile_buffer[fb_offset + 0] =
                clamp<int>(tile_buffer[fb_offset + 0] + color[0], 0, 255);
            tile_buffer[fb_offset + 1] =
                clamp<int>(tile_buffer[fb_offset + 1] + color[1], 0, 255);
            tile_buffer[fb_offset + 2] =
                clamp<int>(tile_buffer[fb_offset + 2] + color[2], 0, 255);
#endif
          }
        }
      }

      for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
          int fb_offset = CANVAS_WIDTH * (i + offy) * 4 + (j + offx) * 4;
          int tl_offset = width * i * 4 + j * 4;
          frameBuffer[fb_offset + 0] = tile_buffer[tl_offset + 0];
          frameBuffer[fb_offset + 1] = tile_buffer[tl_offset + 1];
          frameBuffer[fb_offset + 2] = tile_buffer[tl_offset + 2];
          frameBuffer[fb_offset + 3] = tile_buffer[tl_offset + 3];
        }
      }
    }
  }
  delete[] tile_buffer;
}

#pragma omp declare target

int check_intersection(float *tris, int t_size, float *spheres, float *radius,
                       int s_size, float *P, int *index, float *orig,
                       float *dir) {
  int i, index_tri, index_s;

  int t_has_intersected = false;
  int s_has_intersected = false;

  float p_triangle[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
  float p_sphere[3] = {FLT_MAX, FLT_MAX, FLT_MAX};

  for (i = 0; i < t_size; i++) {
    if (rayTriangleIntersects(orig, dir, tris + i * 9, tris + i * 9 + 3,
                              tris + i * 9 + 6, P)) {
      if (length(p_triangle) > length(P)) {
        index_tri = i;
        t_has_intersected = true;

        copy_array(p_triangle, P, 3);
      }
    }
  }

  for (i = 0; i < s_size; i++) {
    if (raySphereIntersects(orig, dir, spheres + i * 3, radius[i], P)) {
      if (length(p_sphere) > length(P)) {
        index_s = i;
        s_has_intersected = true;

        copy_array(p_sphere, P, 3);
      }
    }
  }

  if (!t_has_intersected && !s_has_intersected)
    return 0;
  if (length(p_triangle) <= length(p_sphere)) {
    copy_array(P, p_triangle, 3);
    *index = index_tri;
    return 1;
  }
  if (length(p_triangle) > length(p_sphere)) {
    copy_array(P, p_sphere, 3);
    *index = index_s;
    return 2;
  }

  return 0;
}
#pragma omp end declare target
