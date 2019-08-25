#ifndef _VIEWER_H
#define _VIEWER_H

#include <stdlib.h>

#include <vector>
#include "vec3.hpp"

class Fluid;	// forward definition

class Viewer
{
private:
	int _sx, _sy;			// screen width and height
	int _ax, _ay;			// mouse movement anchor

	float _quat[4];			// view rotation quaternion
	float _lquat[4];		// light orientation quaternion (bit of a hack...)
	float _dist;			// viewer distance to origin

	FILE* _fp;				// data file
	int _N;					// data resolution
	int _nframes;			// number of frames in data file
	int _cur_frame;

		// OpenGL variables
	unsigned char* _texture_data;
	unsigned int _txt[3];				// texture handles
	unsigned int _prog[2];				// program handles
	unsigned int _font_base;			// first display list for font
	double _persp_m[16], _ortho_m[16];	// projection matrices

	float _light_dir[3];
	int _ray_templ[4096][3];

		// draw the outline of the cube
	void draw_cube(void);

		// draw the slices. m must be the current rotation matrix.
		// if frame==true, the outline of the slices will be drawn as well
	void draw_slices(float m[][4], bool frame);
		// intersect a plane with the cube, helper function for draw_slices()
	std::vector<Vec3> intersect_edges(float a, float b, float c, float d);

	void gen_ray_templ(int edgelen);
	void cast_light(int edgelen, float* dens, unsigned char* intensity);
	inline void light_ray(int x, int y, int z, int n, float decay, float* dens, unsigned char* intensity);


	void print_string(char* s);

	void init_GL(void);
	bool init_font(void);


public:
	Viewer();
	~Viewer();

	bool _draw_cube, _draw_slice_outline;
	char* _dispstring;

	void viewport(int w, int h);			// adjust viewport
	void anchor(int x, int y);				// store given coordinates as transformation anchor
	void rotate(int x, int y);				// rotate according to given coordinates (trackball)
	void rotate_light(int x, int y);		// set light orientation according to given coordinates (trackball)
	void dolly(int y);

	bool open(char* filename);
	void load_frame();						// load one frame from data file
	void frame_from_sim(Fluid* fluid);		// generate data from simulation

	void draw(void);						// draw the volume
};


#endif
