#include "SDL2/SDL_events.h"

extern void (*event_modules[]) (SDL_Event *);
extern void (*event_modules_async[]) (SDL_Event *);
