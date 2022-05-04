#include "SDL2/SDL.h"
#include "sdl/events.h"


SDL_Window *window;

void sdl_init(void) {
	SDL_Init(SDL_INIT_EVERYTHING ^ SDL_INIT_JOYSTICK ^ SDL_INIT_GAMECONTROLLER);
	//vulkan init'd here
	window = SDL_CreateWindow("Hello, world!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 700, 600, SDL_WINDOW_VULKAN);
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
