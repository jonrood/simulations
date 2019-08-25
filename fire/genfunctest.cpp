#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <SDL.h>
#include "genfunc.h"

#define N 64

SDL_Surface *surf;
float t = 0;
int frames = 0;

int EventLoop(void);

void error(const char* str)
{
#ifdef WIN32
	MessageBox(NULL, str, "Error", MB_OK | MB_ICONSTOP);
#else
	printf("Error: %s/n", str);
#endif
}

int main(int argc, char* argv[])
{
	char str[256];
	if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0 ) {
        sprintf(str, "Unable to init SDL: %s\n", SDL_GetError());
		error(str);
        exit(1);
    }
    atexit(SDL_Quit);

    surf = SDL_SetVideoMode(N*3,N*3, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if ( surf == NULL ) {
        sprintf(str, "Unable to set video: %s\n", SDL_GetError());
		error(str);
        exit(1);
    }

	srand(time(NULL));
	genparams = randfloats(123);

	return EventLoop();
}

union RGBAPixel {
	unsigned char c[4];
	unsigned int i;
};

void render(void)
{
	SDL_LockSurface(surf);

	RGBAPixel* buffer = (RGBAPixel*) surf->pixels;
	int x, y, z, xs, ys;
	float f;
	RGBAPixel c;
	int i;
	int lw = N*3;

	for (y=0; y<N; y++) {
		ys = y*3;
		for (x=0; x<N; x++) {
			xs = x*3;

			f = genfunc(x,y,N,N,t,genparams);
			if (f > 1.0f)
				c.i = 0xFF0000FF;		// overflow
			else {
				c.c[0] = c.c[1] = c.c[2] = c.c[3] = f * 255.0f;
			}

			i = ys*lw + xs;

			buffer[i++] = c; buffer[i++] = c; buffer[i] = c;
			i += lw - 2;
			buffer[i++] = c; buffer[i++] = c; buffer[i] = c;
			i += lw - 2;
			buffer[i++] = c; buffer[i++] = c; buffer[i] = c;
		}
	}

	SDL_UnlockSurface(surf);
	SDL_Flip(surf);

	t += 0.02f;
	frames++;
}

Uint32 showfps(Uint32 interval)
{
	printf("FPS: %d\n", frames);
	frames = 0;
	return interval;
}

int EventLoop(void)
{
    SDL_Event event;
	
	SDL_SetTimer(1000, showfps);

    while (1) {
		if (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYUP:
					switch (event.key.keysym.sym) {
						case SDLK_ESCAPE:
							return 0;
					}
					break;

				case SDL_QUIT:
					return 0;
			}
		}
		render();
		frames++;
    }

	return 1;
}
