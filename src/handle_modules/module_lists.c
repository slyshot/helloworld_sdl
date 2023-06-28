#include <stddef.h>
#include "sdl/sdl_module.h"
#include "sdl/modules/helper/helper.h"
#include "sdl/modules/vulkan/vulkan.h"
#include "game/triangle.h"
//default module lists.
//null-terminated
void (*init_modules[]) (void) = {sdl_init,vulkan_before_buffers_init,triangle_init,NULL};
void (*update_modules[]) (int) = {sdl_update,helper_update,triangle_update,vulkan_update,NULL};
void (*cleanup_modules[]) (void) = {NULL};
