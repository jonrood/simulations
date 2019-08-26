#define GL_SILENCE_DEPRECATION
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>

#include "fluid.h"
#include "viewer.h"
#include "genfunc.h"

SDL_Window* screen = NULL;
SDL_Thread* simthread = NULL;

Fluid* fluid = NULL;
Viewer* viewer = NULL;

char infostring[256];
int frames = 0, simframes = 0;
float fps = 0.0f, sfps = 0.0f;

float t = 0;
float* gfparams;

bool paused = false, quitting = false, redraw = true, update = true, wasupdate = false;

int EventLoop();

void error(const char* str)
{
	printf("Error: %s/n", str);
}

bool init(void)
{
	char str[256];

	if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0 ) {
        	sprintf(str, "Unable to init SDL: %s\n", SDL_GetError());
		error(str);
        	return false;
	}
	atexit(SDL_Quit);

	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

        screen = SDL_CreateWindow("Fire", 0, 0, 640, 480, SDL_WINDOW_OPENGL);
	if ( screen == NULL ) {
        	sprintf(str, "Unable to set video: %s\n", SDL_GetError());
		error(str);
        	return false;
	}
        SDL_GLContext Context = SDL_GL_CreateContext(screen);

	fluid = new Fluid();
	fluid->diffusion = 0.00001f;
	fluid->viscosity = 0.000f;
	fluid->buoyancy  = 1.5f;
	fluid->cooling   = 1.0f;
	fluid->vc_eps    = 4.0f;

	gfparams = randfloats(256);

	viewer = new Viewer();
	viewer->viewport(640, 480);

	return true;
}

int simulate(void* bla)
{
	assert(fluid != NULL);

	float dt = 0.1;
	float f;

	while (!quitting)
	{
		//if (!paused && !update) {
		if (1) {
			for (int i=6; i<26; i++)
			{
				for (int j=6; j<26; j++)
				{
					f = genfunc(i-6,j-6,24-6,24-6,t,gfparams);
					fluid->sd[_I(i,28,j)] = 1.0f;
					fluid->sT[_I(i,28,j)] = (float)(rand()%100)/50.0f + f *10.0f;
				}
			}

			fluid->step(dt);
			update = true;
			t += dt;
			simframes++;
		}
	}

	return 0;
}

SDL_UserEvent nextframe = { SDL_USEREVENT, 0, 0, 0};

Uint32 timer_proc(Uint32 interval, void* bla)
{
	/*if (!paused)
		SDL_PushEvent((SDL_Event*) &nextframe);*/
	wasupdate = update;
	update = true;
	return interval;
}

Uint32 showfps(Uint32 interval, void* bla)
{
	static int lastf = 0, lastsf = 0;

	sprintf(infostring, "FPS: %d, sFPS: %d, simframes: %d",
		frames - lastf,
		simframes - lastsf,
		simframes);

	lastf = frames;
	lastsf = simframes;

	return interval;
}

int EventLoop()
{
	SDL_Event event;
	unsigned int ts;

	paused = false;

	while (1) {
		//ts = SDL_GetTicks();
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYUP:
					switch (event.key.keysym.sym) {
						case SDLK_ESCAPE:
							quitting = true;
							return 0;
						case SDLK_c:
							viewer->_draw_cube = !viewer->_draw_cube;
							break;
						case SDLK_i:
							if (viewer->_dispstring != NULL)
								viewer->_dispstring = NULL;
							else
								viewer->_dispstring = infostring;
							break;
						case SDLK_s:
							viewer->_draw_slice_outline = !viewer->_draw_slice_outline;
							break;
						case SDLK_SPACE:
							paused = !paused;
							break;
					}
					break;

				case SDL_QUIT:
					quitting = true;
					return 0;

			/*	case SDL_WINDOWEVENT_RESIZED:
					screen = SDL_SetWindowSize(event->window.windowID, event->window.data1, event->window.data2);
					viewer->viewport(event->window.data1, event->window.data2);
					redraw = true;
					break;*/
				case SDL_MOUSEWHEEL:
					if (event.wheel.y > 0){
						viewer->anchor(0,0);
						viewer->dolly(-480/10);
						redraw = true;
					} else if (event.wheel.y < 0){
						viewer->anchor(0,0);
						viewer->dolly(480/10);
						redraw = true;
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					switch (event.button.button)
					{
						default:
							viewer->anchor(event.button.x, event.button.y);
							break;
					}
					break;
				case SDL_MOUSEMOTION:
					switch (event.motion.state) {
						case SDL_BUTTON(SDL_BUTTON_LEFT):
							viewer->rotate(event.motion.x, event.motion.y);
							redraw = true;
							break;
						case SDL_BUTTON(SDL_BUTTON_RIGHT):
							viewer->dolly(event.motion.y);
							redraw = true;
							break;
					}
					break;
				/*case SDL_USEREVENT:
					viewer->load_frame();
					redraw = true;
					break;*/
			}
		}

		if (update)
		{
			viewer->frame_from_sim(fluid);
			redraw = true;
			update = wasupdate;
			wasupdate = false;
		}

		if (redraw)
		{
			viewer->draw();
			SDL_GL_SwapWindow(screen);
			redraw = false;
			frames++;
		}
    }

	return 1;
}

int main(int argc, char* argv[])
{
	SDL_TimerID fpstimer;
	if (!init()) return 1;
	simthread = SDL_CreateThread(simulate, NULL, NULL);
	fpstimer = SDL_AddTimer(1000, showfps, NULL);
	EventLoop();
	SDL_RemoveTimer(fpstimer);
	quitting = true;
	SDL_WaitThread(simthread, NULL);
}
