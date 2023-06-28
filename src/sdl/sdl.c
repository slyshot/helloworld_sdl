#include "SDL2/SDL.h"
#include "sdl/events.h"
#include "log/log.h"

SDL_Window *window;

void sdl_init(void) {
	SDL_Init(SDL_INIT_EVERYTHING ^ SDL_INIT_JOYSTICK ^ SDL_INIT_GAMECONTROLLER);
	//vulkan init'd here
	//temporary resizability.
	//in future, i want compile flags from SDL2/SDL.h, or another header.
	window = SDL_CreateWindow("Hello, world!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 700, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	if (window == NULL) {
		LOG_ERROR("Could not create SDL window.\n%s\n",SDL_GetError());
	}
	for (int i = 0; event_modules_async[i]; SDL_AddEventWatch((SDL_EventFilter)event_modules_async[i++], NULL) );
}

void handle_events(SDL_Event *e) {
	for (int i=0;event_modules[i];
		event_modules[i++](e)
	);
}

void sdl_update(void) {
	if (event_modules[0] != 0) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			handle_events(&event);
		}
	}
}
