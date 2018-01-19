#include "renderer.hpp"

void render(unsigned char *frameBuffer, int fov, 
			float*** tris, unsigned char** color_tri, int t_size, 
			float** spheres, float* radius, unsigned char** color_sphere, int s_size, 
			float** lights, int l_size)
{
	int i,j,l;
	float aspectRatio = (float)CANVAS_WIDTH / (float)CANVAS_HEIGHT;

// #pragma omp target map(to: i,j,l,fov,aspectRatio, tris[:NUM_TRIANGLES], spheres[:NUM_SPHERES], lights[:NUM_LIGHTS]) \
// 				   map(from: frameBuffer[:4 * CANVAS_HEIGHT * CANVAS_WIDTH]) device(0)
#pragma omp parallel for collapse(1) schedule(dynamic) private(i,j,l) shared(frameBuffer)
	for(i = 0; i < CANVAS_HEIGHT; i++) {
		for(j = 0; j < CANVAS_WIDTH; j++) {

			// Canvas to world transformation
			float px = (2* ((j + 0.5) / (float)CANVAS_WIDTH) - 1) * 
										tan(fov / 2 * M_PI / 180) * aspectRatio;
			float py = (1 - 2* ((i + 0.5) / (float)CANVAS_HEIGHT)) * tan(fov / 2 * M_PI / 180);
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
			frameBuffer[fb_offset + 3] = 1.0;

			float P[3];
			int index;

			// Check to see if there is an intersection between the camera ray and
			// all the objects
			int check = check_intersection(tris, t_size, spheres, radius, s_size, P, &index, orig, dir);			
			if(check != 0) {

				// normal of the object intersected
				float* n = (check == 1) ? 
						  normal(tris[index][0], tris[index][1], tris[index][2]) :
						  sub_vec(P, spheres[index]);
				normalize(n);
				// Color of the object intersected
				unsigned char* color = (check == 1) ?
							   		color_tri[index] :
							   		color_sphere[index];

				// For all the lights in the world, check to see if
				// the intersection point is lit by any of them
				for(l = 0; l < l_size; l++) {

					// Ray from point to light
					float* rayDir = sub_vec(lights[l], P);
					

					normalize(rayDir);

					float P1[3];

					// Check to see if there isn't any objects in the way of the ray
					check = check_intersection(tris, t_size, spheres, radius, s_size, P1, &index, P, rayDir) == 0 ? 1 : 0;
					// Calculate angle between the normal and the ray
					// so we can calculate brightness
					float angle = dot_product(n, rayDir);

					if(angle > 0) {						
						frameBuffer[fb_offset + 0] = clamp<int>(frameBuffer[fb_offset + 0] + check*(unsigned char)color[0]*angle, 0, 255);
						frameBuffer[fb_offset + 1] = clamp<int>(frameBuffer[fb_offset + 1] + check*(unsigned char)color[1]*angle, 0, 255);
						frameBuffer[fb_offset + 2] = clamp<int>(frameBuffer[fb_offset + 2] + check*(unsigned char)color[2]*angle, 0, 255);
					}
				}
			}
		}
	}
}

#pragma omp declare target

int check_intersection(float*** tris, int t_size, float** spheres, float* radius, int s_size, float *P, int *index, float *orig, float *dir)
{
	int i, index_t, index_s;
	
	int t_has_intersected = false;
	int s_has_intersected = false;

	float p_triangle[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
	float p_sphere[3] 	= {FLT_MAX, FLT_MAX, FLT_MAX};

	for(i = 0; i < t_size; i++) {
		if(rayTriangleIntersects(orig, dir, tris[i][0], tris[i][1], tris[i][2], P)) {
			if(length(p_triangle) > length(P)) {
				index_t = i;				
				t_has_intersected = true;

				copy_array(p_triangle, P, 3);
			}
		}
	}

	for(i = 0; i < s_size; i++) {
		if(raySphereIntersects(orig, dir, spheres[i], radius[i], P)) {
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
		*index = index_t;
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