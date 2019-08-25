#ifndef __FLUID_H
#define __FLUID_H

#include <stdio.h>

#define N 30				// must be N^2-2
#define SIZE ((N+2)*(N+2)*(N+2))
#define _I(x,y,z) (((z)<<10)+((y)<<5)+x)

#define SWAPFPTR(x,y) {float *t=x;x=y;y=t;}

class Fluid
{
protected:
	float buffers[10][SIZE];
public:
	float *d, *d0;			// density
	float *T, *T0;			// temperature
	float *u, *u0;			// velocity in x direction
	float *v, *v0;			// velocity in y direction
	float *w, *w0;			// velocity in z direction

protected:
	// simulation methods
		// beware: i changed stam's implementation from a destiation, source ordering
		// of the parameters to source, destiation
	void add_source(float* src, float *dst, float dt);
	void add_buoyancy(float dt);
	void set_bnd(int b, float* x);
	void diffuse(int b, float* x0, float* x, float diff, float dt);
	void advect(int b, float* x0, float* x, float* uu, float* vv, float* ww, float dt);
	void advect_cool(int b, float* x0, float* x, float* y0, float* y, float* uu, float* vv, float* ww, float dt);
	void project(void);
	void vorticity_confinement(float dt);

	void vel_step(float dt);
	void dens_step(float dt);
	void dens_temp_step(float dt);

	// utility methods
	void clear_buffer(float* x);
	void clear_sources(void);

public:
	float sd[SIZE], su[SIZE], sv[SIZE], sw[SIZE], sT[SIZE];	// sources for density and velocities
	float diffusion, viscosity, cooling, buoyancy, vc_eps;

	Fluid(void);
	~Fluid(void);

	void step(float dt);

	void store(FILE *fp);
};

#endif

