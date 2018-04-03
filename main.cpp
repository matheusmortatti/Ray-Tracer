#include <algorithm>
#include <cfloat>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <thread>
#include <vector>

#include <random>
#include <limits.h>
#include <omp.h>
#ifdef USE_SDL
#include <SDL.h>
#endif // USE_SDL

using namespace std;

#include "maths.hpp"
#include "renderer.hpp"

class InputParser
{
public:
  InputParser(int &argc, char **argv)
  {
    for (int i = 1; i < argc; ++i)
      this->tokens.push_back(std::string(argv[i]));
  }
  /// @author iain
  const std::string &getCmdOption(const std::string &option) const
  {
    std::vector<std::string>::const_iterator itr;
    itr = std::find(this->tokens.begin(), this->tokens.end(), option);
    if (itr != this->tokens.end() && ++itr != this->tokens.end())
    {
      return *itr;
    }
    static const std::string empty_string("");
    return empty_string;
  }
  /// @author iain
  bool cmdOptionExists(const std::string &option) const
  {
    return std::find(this->tokens.begin(), this->tokens.end(), option) !=
           this->tokens.end();
  }

private:
  std::vector<std::string> tokens;
};

void ReadSceneFile(std::string path, float **tris, unsigned char **t_colors,
                   float **spheres, float **radius, unsigned char **s_colors,
                   float **lights, int &t_size, int &s_size, int &l_size);
int init_triangles(float **tris, unsigned char **colors);
int init_spheres(float **spheres, float **radius, unsigned char **colors,
                 int row, int col);
int init_lights(float **lights);

#ifdef USE_SDL
void init_SDL(SDL_Window *&window, SDL_Renderer *&renderer);
void put_pixel(SDL_Surface *screenSurface, int x, int y, vec3 color);
#endif // USE_SDL

int main(int argc, char **argv)
{

  InputParser input(argc, argv);
  if (input.cmdOptionExists("-h"))
  {
    cout << "Ray-Tracer\n"
         << "-f <filename> : Input file to configurate the scene (default: "
            "scene.txt)\n"
         << "-row          : Row number of the sphere matrix (default: 10)\n"
         << "-col          : Column number of the sphere matrix (default: 10)\n"
         << "-n            : No display\n"
         << "-h            : Print this message\n";
    exit(0);
  }

  const bool no_display = input.cmdOptionExists("-n");

  const std::string &filename =
      input.cmdOptionExists("-f") ? input.getCmdOption("-f") : "scene.txt";

  int col = 10;
  int row = 10;

  if (input.cmdOptionExists("-col"))
    col = stoi(input.getCmdOption("-col"));
  if (input.cmdOptionExists("-row"))
    row = stoi(input.getCmdOption("-row"));

  int fov = 90;
  unsigned char *frameBuffer;

  frameBuffer = new unsigned char[4 * CANVAS_HEIGHT * CANVAS_WIDTH];

  float *tris, *spheres, *radius, *lights;
  unsigned char *color_tri, *color_sphere;

  int t_size, s_size, l_size;

  ReadSceneFile(filename, &tris, &color_tri, &spheres, &radius, &color_sphere,
                &lights, t_size, s_size, l_size);

  s_size = init_spheres(&spheres, &radius, &color_sphere, row, col);
  t_size = init_triangles(&tris, &color_tri);

  // Create thread and start rendering
  std::thread render_thread(render, frameBuffer, fov, tris, color_tri, t_size,
                            spheres, radius, color_sphere, s_size, lights,
                            l_size);

#ifdef USE_SDL
  if (!no_display)
  {
    /**
     * Create SDL Window
     **/
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;

    init_SDL(window, renderer);

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING, CANVAS_WIDTH,
                                CANVAS_HEIGHT);

    /**
     * Keep the screen up until the user closes it
     **/
    bool quit = false;
    while (!quit)
    {
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
      SDL_RenderClear(renderer);

      SDL_Event event;
      while (SDL_PollEvent(&event))
      {
        switch (event.type)
        {
        case SDL_QUIT:
          quit = true;
          break;
        }
      }

      /* Draw the image to the texture */
      SDL_UpdateTexture(texture, NULL, &frameBuffer[0], CANVAS_WIDTH * 4);
      SDL_RenderCopy(renderer, texture, NULL, NULL);
      SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
  }
#endif // USE_SDL

  // Join thread to wait for it to end before exiting
  render_thread.join();

  if (no_display)
  {
    std::ofstream image;
    image.open("image.ppm");

    image << "P3\n"
          << CANVAS_WIDTH << " " << CANVAS_HEIGHT << " " << 255
          << std::endl;

    for (int i = 0; i < CANVAS_HEIGHT; i++)
    {
      for (int j = 0; j < CANVAS_WIDTH; j++)
      {
        int fb_offset = CANVAS_WIDTH * i * 4 + j * 4;
        int z = (int)abs(frameBuffer[fb_offset + 0]),
            y = (int)abs(frameBuffer[fb_offset + 1]),
            x = (int)abs(frameBuffer[fb_offset + 2]);

        image << x << " " << y << " " << z << " ";
      }
      image << std::endl;
    }
  }

  delete[] tris;
  delete[] color_tri;

  delete[] spheres;
  delete[] color_sphere;
  delete[] radius;

  delete[] lights;

  delete[] frameBuffer;

  return 0;
}

#ifdef USE_SDL
void put_pixel(SDL_Surface *screenSurface, int x, int y, vec3 color)
{
  Uint8 *pixels = (Uint8 *)screenSurface->pixels;
  Uint8 *pixel = pixels + y * screenSurface->pitch + x;
  *pixel = SDL_MapRGB(screenSurface->format, 255, 255, 255);
}

void init_SDL(SDL_Window *&window, SDL_Renderer *&renderer)
{

  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    std::cout << "SDL Could not initialize! SDL Error: " << SDL_GetError()
              << std::endl;
  }
  else
  {
    window = SDL_CreateWindow("RayTracer", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, CANVAS_WIDTH,
                              CANVAS_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL)
    {
      std::cout << "Window could not be created! SDL Error: " << SDL_GetError()
                << std::endl;
    }
    else
    {
      renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    }
  }
}
#endif // USE_SDL

void ReadSceneFile(std::string path, float **tris, unsigned char **t_colors,
                   float **spheres, float **radius, unsigned char **s_colors,
                   float **lights, int &t_size, int &s_size, int &l_size)
{
  std::ifstream file;
  file.open(path);

  if (!file.is_open())
  {
    std::cerr << "Could not open file '" << path << "'" << std::endl;
  }

  std::vector<float> vtriangles;
  std::vector<float> vspheres;
  std::vector<float> vlights;
  while (!file.eof())
  {
    std::string line;
    std::getline(file, line);

    if (line.front() == '/')
      continue;

    std::istringstream stream(line);
    char type;
    stream >> type;

    float val;
    unsigned char col;

    switch (type)
    {
    case 's':
      for (int i = 0; i < 7; i++)
      {
        stream >> val;
        vspheres.push_back(val);
      }
      break;

    case 't':
      for (int i = 0; i < 12; i++)
      {
        stream >> val;
        vtriangles.push_back(val);
      }
      break;

    case 'l':
      for (int i = 0; i < 3; i++)
      {
        stream >> val;
        vlights.push_back(val);
      }
      break;
    }
  }

  t_size = vtriangles.size() / 12;
  s_size = vspheres.size() / 7;
  l_size = vlights.size() / 3;

  (*tris) = new float[t_size * 9];
  (*t_colors) = new unsigned char[t_size * 3];
  (*spheres) = new float[s_size * 3];
  (*radius) = new float[s_size * 1];
  (*s_colors) = new unsigned char[s_size * 3];
  (*lights) = new float[l_size * 3];

  for (int i = 0; i < t_size; i++)
  {
    (*tris)[i * 9 + 0] = vtriangles[i * 12 + 0];
    (*tris)[i * 9 + 1] = vtriangles[i * 12 + 1];
    (*tris)[i * 9 + 2] = vtriangles[i * 12 + 2];
    (*tris)[i * 9 + 3] = vtriangles[i * 12 + 3];
    (*tris)[i * 9 + 4] = vtriangles[i * 12 + 4];
    (*tris)[i * 9 + 5] = vtriangles[i * 12 + 5];
    (*tris)[i * 9 + 6] = vtriangles[i * 12 + 6];
    (*tris)[i * 9 + 7] = vtriangles[i * 12 + 7];
    (*tris)[i * 9 + 8] = vtriangles[i * 12 + 8];
    (*t_colors)[i * 3 + 0] = (unsigned char)vtriangles[i * 12 + 9];
    (*t_colors)[i * 3 + 1] = (unsigned char)vtriangles[i * 12 + 10];
    (*t_colors)[i * 3 + 2] = (unsigned char)vtriangles[i * 12 + 11];
  }

  for (int i = 0; i < s_size; i++)
  {
    (*radius)[i * 1 + 0] = vspheres[i * 7 + 0];
    (*spheres)[i * 3 + 0] = vspheres[i * 7 + 1];
    (*spheres)[i * 3 + 1] = vspheres[i * 7 + 2];
    (*spheres)[i * 3 + 2] = vspheres[i * 7 + 3];
    (*s_colors)[i * 3 + 0] = (unsigned char)vspheres[i * 7 + 4];
    (*s_colors)[i * 3 + 1] = (unsigned char)vspheres[i * 7 + 5];
    (*s_colors)[i * 3 + 2] = (unsigned char)vspheres[i * 7 + 6];
  }

  for (int i = 0; i < l_size; i++)
  {
    (*lights)[i * 3 + 0] = vlights[i * 3 + 0];
    (*lights)[i * 3 + 1] = vlights[i * 3 + 1];
    (*lights)[i * 3 + 2] = vlights[i * 3 + 2];
  }
}

int init_triangles(float **tris, unsigned char **colors)
{
  int t_size = 5;
  int scale = 20;

  (*tris) = new float[t_size * t_size * 9 * 2];
  (*colors) = new unsigned char[t_size * t_size * 3 * 2];

  for (int i = 0; i < t_size; i++)
  {
    for (int j = 0; j < t_size; j++)
    {
      (*tris)[(i * t_size + j) * 18 + 0] = (j * 2 - t_size) / 2 * scale;
      (*tris)[(i * t_size + j) * 18 + 1] = (i * 2 - t_size) / 2 * scale;
      (*tris)[(i * t_size + j) * 18 + 2] = -100;
      (*tris)[(i * t_size + j) * 18 + 3] = ((j * 2 - t_size) / 2 * scale - scale);
      (*tris)[(i * t_size + j) * 18 + 4] = (i * 2 - t_size) / 2 * scale;
      (*tris)[(i * t_size + j) * 18 + 5] = -100;
      (*tris)[(i * t_size + j) * 18 + 6] = (j * 2 - t_size) / 2 * scale;
      (*tris)[(i * t_size + j) * 18 + 7] = ((i * 2 - t_size) / 2 * scale - scale);
      (*tris)[(i * t_size + j) * 18 + 8] = -100;
      (*colors)[(i * t_size + j) * 6 + 0] = rand() % 256;
      (*colors)[(i * t_size + j) * 6 + 1] = rand() % 256;
      (*colors)[(i * t_size + j) * 6 + 2] = rand() % 256;

      (*tris)[(i * t_size + j) * 18 + 9] = (j * 2 - t_size) / 2 * scale;
      (*tris)[(i * t_size + j) * 18 + 10] = ((i * 2 - t_size) / 2 * scale - scale);
      (*tris)[(i * t_size + j) * 18 + 11] = -100;
      (*tris)[(i * t_size + j) * 18 + 12] = ((j * 2 - t_size) / 2 * scale - scale);
      (*tris)[(i * t_size + j) * 18 + 13] = (i * 2 - t_size) / 2 * scale;
      (*tris)[(i * t_size + j) * 18 + 14] = -100;
      (*tris)[(i * t_size + j) * 18 + 15] = (j * 2 - t_size) / 2 * scale - scale;
      (*tris)[(i * t_size + j) * 18 + 16] = ((i * 2 - t_size) / 2 * scale - scale);
      (*tris)[(i * t_size + j) * 18 + 17] = -100;
      (*colors)[(i * t_size + j) * 6 + 3] = rand() % 256;
      (*colors)[(i * t_size + j) * 6 + 4] = rand() % 256;
      (*colors)[(i * t_size + j) * 6 + 5] = rand() % 256;
    }
  }

  return t_size * t_size * 2;
}

int init_spheres(float **spheres, float **radius, unsigned char **colors,
                 int row, int col)
{
  float rad = 1.0f;

#ifdef BENCHMIN
  row = 30;
  col = 30;
#endif

#ifdef BENCHMID
  row = 70;
  col = 70;
#endif

#ifdef BENCHMAX
  row = 100;
  col = 100;
#endif

  (*spheres) = new float[row * col * 3];
  (*colors) = new unsigned char[row * col * 3];
  (*radius) = new float[row * col];

  for (int i = 0; i < row; i++)
  {
    for (int j = 0; j < col; j++)
    {
      (*spheres)[(i * col + j) * 3 + 0] = 2 * (j * 2 - col) * rad;
      (*spheres)[(i * col + j) * 3 + 1] = 2 * (i * 2 - row) * rad;
      (*spheres)[(i * col + j) * 3 + 2] = -50 + rand() % 3 - 1;
      (*radius)[(i * col + j)] = rad;

      (*colors)[(i * col + j) * 3 + 0] = 0;   //rand() % 256;
      (*colors)[(i * col + j) * 3 + 1] = 255; //rand() % 256;
      (*colors)[(i * col + j) * 3 + 2] = 0;   //rand() % 256;
    }
  }

  return row * col;
}

int init_lights(float **lights)
{
  int l_size = 2;

  (*lights) = new float[l_size * 3];

  float v[2][3] = {{2.0, 5.0, -4.0}, {-2.0, 5.0, -4.0}};

  for (int i = 0; i < l_size; i++)
  {
    (*lights)[i * 3 + 0] = v[i][0];
    (*lights)[i * 3 + 1] = v[i][1];
    (*lights)[i * 3 + 2] = v[i][2];
  }

  return l_size;
}
