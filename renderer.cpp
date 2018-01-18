#include "renderer.hpp"

void render(unsigned char *frameBuffer, int fov, triangle *tris, sphere *spheres, vec3 *lights)
{
	int i,j,l;
	float aspectRatio = (float)CANVAS_WIDTH / (float)CANVAS_HEIGHT;

#pragma omp target map(to: i,j,l,fov,aspectRatio, tris[:NUM_TRIANGLES], spheres[:NUM_SPHERES], lights[:NUM_LIGHTS]) \
				   map(from: frameBuffer[:4 * CANVAS_HEIGHT * CANVAS_WIDTH]) device(0)
#pragma omp parallel for collapse(1) schedule(dynamic) private(i,j,l) shared(frameBuffer)

	for(i = 0; i < CANVAS_HEIGHT; i++) {
		for(j = 0; j < CANVAS_WIDTH; j++) {

			// Canvas to world transformation
			float px = (2* ((j + 0.5) / (float)CANVAS_WIDTH) - 1) * 
										tan(fov / 2 * M_PI / 180) * aspectRatio;
			float py = (1 - 2* ((i + 0.5) / (float)CANVAS_HEIGHT)) * tan(fov / 2 * M_PI / 180);
			vec3 rayP = {.x = px, .y = py, .z = -1.0};
			vec3 orig = {.x = 0.0, .y = 0.0, .z = 0.0};

			// Ray direction
			vec3 dir = sub_vec3(rayP, orig);
			normalize_vec3(&dir);

			// Get transformed framebuffer index
			int fb_offset = CANVAS_WIDTH * i * 4 + j * 4;

			// Initialize framebuffer pixel color
			frameBuffer[fb_offset + 0] = 0;
			frameBuffer[fb_offset + 1] = 0;
			frameBuffer[fb_offset + 2] = 0;
			frameBuffer[fb_offset + 3] = 1.0;

			vec3 P;
			int index;

			// Check to see if there is an intersection between the camera ray and
			// all the objects
			int check = check_intersection(tris, NUM_TRIANGLES, spheres, NUM_SPHERES, &P, &index, orig, dir);			
			if(check != 0) {

				// normal of the object intersected
				vec3 n = (check == 1) ? 
						  normal(tris[index]) :
						  sub_vec3(P, spheres[index].orig);
				normalize_vec3(&n);
				// Color of the object intersected
				vec3 color = (check == 1) ?
							 tris[index].color :
							 spheres[index].color;

				// For all the lights in the world, check to see if
				// the intersection point is lit by any of them
				for(l = 0; l < NUM_LIGHTS; l++) {

					// Ray from point to light
					vec3 rayDir = sub_vec3(lights[l], P);
					

					normalize_vec3(&rayDir);

					vec3 P1;

					// Check to see if there isn't any objects in the way of the ray
					check = check_intersection(tris, NUM_TRIANGLES, spheres, NUM_SPHERES, &P1, &index, P, rayDir) == 0 ? 1 : 0;
					// Calculate angle between the normal and the ray
					// so we can calculate brightness
					float angle = dotProduct(n, rayDir);

					if(angle > 0) {
						vec3 v = scale_vec3(color, angle);
						frameBuffer[fb_offset + 0] = clamp<int>(frameBuffer[fb_offset + 0] + check*(unsigned char)v.x, 0, 255);
						frameBuffer[fb_offset + 1] = clamp<int>(frameBuffer[fb_offset + 1] + check*(unsigned char)v.y, 0, 255);
						frameBuffer[fb_offset + 2] = clamp<int>(frameBuffer[fb_offset + 2] + check*(unsigned char)v.z, 0, 255);
					}
				}
			}
		}
		
	}
}

#pragma omp declare target
int check_intersection(triangle *tris, int t_size, sphere *spheres, int s_size, vec3 *P, int *index, vec3 orig, vec3 dir)
{
	int i, index_t, index_s;
	
	int t_has_intersected = false;
	int s_has_intersected = false;

	vec3 p_triangle = {.x = FLT_MAX, .y = FLT_MAX, .z = FLT_MAX};
	vec3 p_sphere = {.x = FLT_MAX, .y = FLT_MAX, .z = FLT_MAX};

	for(i = 0; i < t_size; i++) {
		if(rayTriangleIntersects(orig, dir, tris[i], P)) {
			if(length_vec3(p_triangle) > length_vec3(*P)) {
				index_t = i;
				p_triangle = *P;
				t_has_intersected = true;
			}
		}
	}

	for(i = 0; i < s_size; i++) {
		if(raySphereIntersects(orig, dir, spheres[i], P)) {
			if(length_vec3(p_sphere) > length_vec3(*P)) {
				index_s = i;
				p_sphere = *P;
				s_has_intersected = true;
			}
		}
	}

	if(!t_has_intersected && !s_has_intersected)
		return 0;
	if(length_vec3(p_triangle) <= length_vec3(p_sphere)){
		*P = p_triangle;
		*index = index_t;
		return 1;
	}
	if(length_vec3(p_triangle) > length_vec3(p_sphere)) {
		*P = p_sphere;
		*index = index_s;
		return 2;
	}

	return 0;
}
#pragma omp end declare target