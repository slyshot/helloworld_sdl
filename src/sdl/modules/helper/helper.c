#include <SDL2/SDL_events.h>
#include <SDL2/SDL.h>
void helper_eventhandle(SDL_Event * e) {
	if (e->type == SDL_WINDOWEVENT) {
		switch (e->window.event) {
			case SDL_WINDOWEVENT_CLOSE:
				exit(-1);
				break; //lol
			default:
				break;
		}
	}

}
void helper_update(int dt) {
	SDL_Delay(12);
}