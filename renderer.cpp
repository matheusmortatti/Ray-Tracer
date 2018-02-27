#include "renderer.hpp"

void render(unsigned char *frameBuffer, int fov,
			float* tris, unsigned char* color_tri, int t_size,
			float* spheres, float* radius, unsigned char* color_sphere, int s_size,
			float* lights, int l_size)
{
	float aspectRatio = (float)CANVAS_WIDTH / (float)CANVAS_HEIGHT;

 #pragma omp target map(to: fov,aspectRatio, \
 					   tris[:NUM_TRIANGLES*9], color_tri[:NUM_TRIANGLES*3], \
 					   spheres[:NUM_SPHERES*3], radius[:NUM_SPHERES], color_sphere[:NUM_SPHERES*3], \
 					   lights[:NUM_LIGHTS*3]) \
 				   map(from: frameBuffer[:4 * CANVAS_HEIGHT * CANVAS_WIDTH]) device(0)
#pragma omp parallel for collapse(1) schedule(dynamic) shared(frameBuffer)
	for(int i = 0; i < CANVAS_HEIGHT; i++) {
		for(int j = 0; j < CANVAS_WIDTH; j++) {

			// Canvas to world transformation
			float px = (2* ((j + 0.5) / (float)CANVAS_WIDTH) - 1) *
										tan(fov * 0.5 * M_PI/180) * aspectRatio;
			float py = (1 - 2* ((i + 0.5) / (float)CANVAS_HEIGHT)) * tan(fov * 0.5 * M_PI/180);
			float rayP[3] = {px, py, -1.0};
			float orig[3] = {0.0, 0.0, 0.0};

			// Ray direction
			float* dir = sub_vec(rayP, orig);
			normalize(dir);

			// Get transformed framebuffer index
			int fb_offset = CANVAS_WIDTH * i * 4 + j * 4;

			// Initialize framebuffer pixel color
			frameBuffer[fb_offset + 0] = 0;
			frameBuffer[fb_offset + 1] = 0;
			frameBuffer[fb_offset + 2] = 0;
			frameBuffer[fb_offset + 3] = -1;

			float P[3];
			int index;

			// Check to see if there is an intersection between the camera ray and
			// all the objects
			int check = check_intersection(tris, t_size, spheres, radius, s_size, P, &index, orig, dir);
			if(check != 0) {

				// normal of the object intersected
				float* n = (check == 1) ?
						  normal(tris + index * 9, tris + index * 9 + 3, tris + index * 9 + 6) :
						  sub_vec(P, spheres + index * 3);
				normalize(n);
				// Color of the object intersected
				unsigned char* color = (check == 1) ?
							   		color_tri + index * 3 :
							   		color_sphere + index * 3;

			#if !UNLIT

				// For all the lights in the world, check to see if
				// the intersection point is lit by any of them
				for(int l = 0; l < l_size; l++) {

					// Ray from point to light
					float* rayDir = sub_vec(lights + l * 3, P);
					normalize(rayDir);


					// std::cout << (n)[0] << " " << (n)[1] << " " << (n)[2] << std::endl;

					// Calculate angle between the normal and the ray
					// so we can calculate brightness
					float angle = dot_product(n, rayDir);
					angle = angle < 0 ? 0 : angle;

					float P1[3];
					
					// If there are no objects in the way of the ray, the point is lit by the light
					if(check_intersection(tris, t_size, spheres, radius, s_size, P1, &index, P, rayDir) == 0)
					{
						frameBuffer[fb_offset + 0] = clamp<uint16_t>(frameBuffer[fb_offset + 0] + color[0]*angle, 0, 255);
						frameBuffer[fb_offset + 1] = clamp<uint16_t>(frameBuffer[fb_offset + 1] + color[1]*angle, 0, 255);
						frameBuffer[fb_offset + 2] = clamp<uint16_t>(frameBuffer[fb_offset + 2] + color[2]*angle, 0, 255);
					}
					delete[] rayDir;
				}

			#else
				frameBuffer[fb_offset + 0] = clamp<int>(frameBuffer[fb_offset + 0] + color[0], 0, 255);
				frameBuffer[fb_offset + 1] = clamp<int>(frameBuffer[fb_offset + 1] + color[1], 0, 255);
				frameBuffer[fb_offset + 2] = clamp<int>(frameBuffer[fb_offset + 2] + color[2], 0, 255);
			#endif

				delete[] n;
			}
			delete[] dir;
		}
	}
}

#pragma omp declare target

int check_intersection(float* tris, int t_size, float* spheres, float* radius, int s_size, float *P, int *index, float *orig, float *dir)
{
	int i, index_tri, index_s;

	int t_has_intersected = false;
	int s_has_intersected = false;

	float p_triangle[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
	float p_sphere[3] 	= {FLT_MAX, FLT_MAX, FLT_MAX};

	for(i = 0; i < t_size; i++) {
		if(rayTriangleIntersects(orig, dir, tris + i * 9, tris + i * 9 + 3, tris + i * 9 + 6, P)) {
			if(length(p_triangle) > length(P)) {
				index_tri = i;
				t_has_intersected = true;

				copy_array(p_triangle, P, 3);
			}
		}
	}	

	for(i = 0; i < s_size; i++) {
		if(raySphereIntersects(orig, dir, spheres + i * 3, radius[i], P)) {
			if(length(p_sphere) > length(P)) {
				index_s = i;
				s_has_intersected = true;

				copy_array(p_sphere, P, 3);
			}
		}
	}

	if(!t_has_intersected && !s_has_intersected)
		return 0;
	if(length(p_triangle) <= length(p_sphere)){
		copy_array(P, p_triangle, 3);
		*index = index_tri;
		return 1;
	}
	if(length(p_triangle) > length(p_sphere)) {
		copy_array(P, p_sphere, 3);
		*index = index_s;
		return 2;
	}

	return 0;
}
#pragma omp end declare target
