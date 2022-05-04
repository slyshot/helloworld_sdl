#include "SDL2/SDL_events.h"
#include "stddef.h"
#include "sdl/modules/helper/helper.h"
void (*event_modules[]) (SDL_Event *) = {helper_eventhandle,NULL};
void (*event_modules_async[]) (SDL_Event *) = {NULL};
