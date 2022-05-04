#include <stddef.h>
#include "sdl/sdl_module.h"
#include "sdl/modules/helper/helper.h"
#include "sdl/modules/vulkan/vulkan.h"
//default module lists.
//null-terminated
void (*init_modules[]) (void) = {sdl_init,vulkan_init,NULL};
void (*update_modules[]) (int) = {sdl_update,helper_update,vulkan_update};
void (*cleanup_modules[]) (void) = {NULL};
