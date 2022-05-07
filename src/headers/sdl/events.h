#include "SDL2/SDL_events.h"

extern void (*event_modules[]) (SDL_Event *);
extern int (*event_modules_async[]) (void *, SDL_Event *);
