#include "log/log.h"

#define VK_ENABLE_BEHTA_EXTENSIONS
#include <vulkan/vulkan.h>
#include "sdl/modules/vulkan/vkstate.h"
// #include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include "sdl/sdl.h"
#include "log/log.h"
#include <stdlib.h>
#include <string.h>
// #include <cglm/vec2.h>
#include <cglm/cglm.h>
// #define STORE(NON_STRUCT_PTR,TYPE,STRUCT) do { TYPE * ptr = malloc(sizeof(TYPE)); memcpy(ptr,NON_STRUCT_PTR,sizeof(TYPE)); STRUCT.NON_STRUCT_PTR = ptr; } while 0
#define CLAMP(x, lo, hi)    ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))
//this is entirely a struct to store all objects created and information that was required to create them.
//if a info struct has a pointer to another struct within it, only the outermost struct must be put here.

/*
TODO:

	1.
		Move all the major steps, right now categorized through brackets into their own functions.
		Have them take as variables vkstate and any information regarding whatever it is they are creating (create infos, primarily)
	2.
		Make "template" create info generator(s), and put them in a different file.
		This way, you will be able to make a swapchain that fills the vkstate with gen_swapchain(&vkstruct, default_swapchain_template(vkstate) ),
		but if I find another swapchain info I want to use instead, gen_swapchain can be called with some other swapchain_info as the second argument.

		It must be a generator because it has to reference things that have been dealt with before in the vkstate.
		For instance, many functions require a logical device. Instead of passing everything, I pass a createinfo generated with vkstate available
		to pull things like log_dev and imageviews whenever needed.
	3.
		Make cleanup functions, and set up clean recreation of the different objects (That is, recreate the object and everything of which their validitiy requires
			recreation because of the first object being recreated)
		I'm considering doing this with a dependency graph, since sometimes things which seem like they could be dependant happen not to be,
		but that may be an overcomplication, and I may just make a specific ordering for the gen and cleanup functions to run in. 
	4.
		Set up dynamic state, and descriptors
	5.
		Change makefile to compile shaders with glslc if they are found.
	6.
		Do something about the compile-time shaders paths
	7.
		Use a more descriptive name than 'i' for your for loops.
	8.
		Since I want this to be a tool I can use for other things, and other people can use if they dare,
		I want the buffer objects to be something the user requests from a callback. Probably, information regarding that and other things may be populated before vulkan_init is even called.
		Regardless, I want the buffer objects, as they are, to not be hardcoded but to be pointers.

*/

/*
In order to properly store memory buffers in vulkan, some things need to be worked out:
first of all, there must be a good interface between the way things are stored in vulkan vs in the game.
There are a variety of reasons to do this:
1. Memory arrangements which are good for efficient, easy-to-understand, modular, extensible features in programming design
aren't going to be efficient in GPU memory
2. there ought to be seperation from graphics AI and logic; the ability to 'drag-and-drop' something like openGL 
in place of vulkan ought to be promoted by the design

Things which are reccomended in graphics programming aren't reccomended in regular programming. For instance, 
it seems best to keep memory allocations minimal and big. Rather than allocate 10 buffers, allocate one big one, with 10
different kinds of data in it.
The problems that arise when dealing with a buffer that stores data with different purposes are generally solved by things like
the 'stride' perameter, and other ease-of-life features.

As mentioned later in comments, each generation function should have a secondary perameter of an incomplete struct and bitmask of inputted values.
That way, you could have a complete deviation (Basically just setting the info and having whatever setup needs to go around that be default) or setting just one or so options in the infe








*/

#define VERTEX_SHADER_PATH "./vert.spv"
#define FRAGMENT_SHADER_PATH "./frag.spv"
vk_state vkstate;
//future speaking: THIS is what I decide to make it's own function?!
//Verify that, given an extension list and a requested extension, that the requested extension is in the extension list.
void read_file(char *path, int *_size, uint32_t **out) {
	FILE * file=  fopen(path, "rb");
	fseek(file, 0, SEEK_END);
	long int size = ftell(file);
	if (_size != NULL) *_size = size;
	fseek(file, 0, SEEK_SET);
	uint32_t * file_data = malloc(sizeof(uint32_t)*(size/4));
	fread(file_data, sizeof(uint32_t), size/4, file );
	fclose(file);
	*out = file_data;
}

int has(VkExtensionProperties *properties, uint32_t property_count, char* requested_extension) {
	for (uint32_t i = 0; i < property_count; i++) {
		if (strcmp(properties[i].extensionName, requested_extension) == 0) {
			return 1;
		}
	}
	return 0;
}


void generate_instance(vk_state *vkstate) {
	{ //instance info (Add it to vkstate)
		VkInstanceCreateInfo *instance_info = calloc(1,sizeof(VkInstanceCreateInfo));
		{ //application info
			VkApplicationInfo *appinfo = calloc(1,sizeof(VkApplicationInfo));
			appinfo->sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			appinfo->pApplicationName = "Hi world.";
			// 040?
			appinfo->applicationVersion = 040;
			appinfo->apiVersion = VK_MAKE_VERSION(1, 3, 0);
			instance_info->pApplicationInfo = appinfo;
		}
		instance_info->sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		{  // Getting and verifying the instance extensions
			
			char **req_exts = malloc(sizeof(char*)*1);
			// req_exts[0] = "VK_EXT_debug_report";
			uint32_t enabled_extension_count = 0;

			uint32_t property_count;

			VkResult result = vkEnumerateInstanceExtensionProperties(NULL, &property_count, NULL);
			if (result != VK_SUCCESS) {
				LOG_ERROR("Error, can't get number of extensions. Error num: %d\n",result);
			}
			VkExtensionProperties * props = malloc(sizeof(VkExtensionProperties)*property_count);
			result = vkEnumerateInstanceExtensionProperties(NULL, &property_count, props);
			if (result != VK_SUCCESS) {
				LOG_ERROR("Error, can't enumerate extensions. Error num: %d\n",result);
			}

			{ //prepare requested_extensions and set enabled_extension_count
				{ //detect missing extensions
					int crash = 0;
					for (unsigned i = 0; i < enabled_extension_count; i++) {
						if(!has(props,property_count,req_exts[i])) {
							crash = 1;
							LOG_ERROR("Error: Missing extension %s\n",req_exts[i]);
						}
					}
					if (crash) {
						exit(-1);
					}
					free(props);
				}
				{ //add SDL extensions
					unsigned int pcount;
					if (SDL_Vulkan_GetInstanceExtensions(window, &pcount,NULL) == SDL_FALSE) {
						LOG_ERROR("Error, could not get SDL Vulkan extensions\n");
						exit(-1);
					}
					char **pnames = malloc(sizeof(char*)*pcount);
					SDL_Vulkan_GetInstanceExtensions(window,&pcount,(const char **)pnames);
					req_exts = realloc(req_exts,sizeof(char*)*(enabled_extension_count + pcount));
					for (uint32_t i = 0; i < pcount; i++) {
						//add one for null character not counted as part of the string length
						req_exts[i+enabled_extension_count] = malloc(strlen(pnames[i]) + 1);
						memcpy(req_exts[i+enabled_extension_count], pnames[i], strlen(pnames[i]) + 1);
						// printf("%s\n",req_exts[i+enabled_extension_count]);
					}
					enabled_extension_count += pcount;
					free(pnames);
				}
			}
			instance_info->ppEnabledExtensionNames = (const char **)req_exts;
			instance_info->enabledExtensionCount = enabled_extension_count;


		}

		{ //iterate through every validation layer trying to figure out why the FUCK the layer isn't working
			uint32_t layerCount;
			vkEnumerateInstanceLayerProperties(&layerCount,NULL);
			VkLayerProperties *availableLayers = malloc(sizeof(VkLayerProperties)*layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);
			// for (int layer_index = 0; layer_index < layerCount; layer_index++) {
			// 	// for (int i = 0; i < availableLayers[layer_index]; i++) {
			// 	printf("available: %s\n",availableLayers[layer_index].layerName);
			// 	// }
			// }
		}
		{
			//This doesn't work with my current system.
			//I remember a day when it did, and every online source I can find, and every discord, has pretty much confirmed that it /should/ work.
			//This gives me reason to believe I am simply cursed, and there is no solution to my problem. On my one particular computer, I'll use vkconfig to set up
			//validation layers until I find a witch to lift my curse. But. This should be fine. :))))
			vkstate->layername = "VK_LAYER_KHRONOS_validation";
			instance_info->ppEnabledLayerNames = &vkstate->layername;
			instance_info->enabledLayerCount = 1;
		}
		vkstate->instance_info = instance_info;
		// printf("%s\n",vkstate->instance_info->ppEnabledLayerNames[0]);
	}
	VkResult result = vkCreateInstance(vkstate->instance_info, NULL, &vkstate->instance);
	if (result != VK_SUCCESS) {
		LOG_ERROR("Could not create instance. Error num %d\n", result);
		exit(-1);
	}
}
void generate_surface(vk_state *vkstate) {
	if (SDL_Vulkan_CreateSurface(window,vkstate->instance,&vkstate->surface) == SDL_FALSE) {
		LOG_ERROR("Error, can't create surface, SDL_VulkanCreateSurface failed.\n");
		exit(-1);
	}
}
void generate_physical_device(vk_state *vkstate) {
	uint32_t dev_count;
	// vkEnumeratePhysicalDevices(NULL, NULL, NULL);
	VkPhysicalDevice * devices;
	{ //get device list & count
		VkResult result = vkEnumeratePhysicalDevices(vkstate->instance, &dev_count, NULL);
		if (result != VK_SUCCESS) {
			LOG_ERROR("Error, can't get number of physical devices. Error num: %d\n",result);
			exit(-1);
		}
		devices = malloc(sizeof(VkPhysicalDevice)*dev_count);
		result = vkEnumeratePhysicalDevices(vkstate->instance, &dev_count, devices);
		if (result != VK_SUCCESS) {
			LOG_ERROR("Error, can't enumerate physical devices. Error num: %d\n",result);
			exit(-1);
		}
	}
	{ //select suitable device
		//TODO: When turning things into functions, a suitability function for devices?
		//this way, as dev understanding of what is and isn't required changes, they just need to modify that, rather than
		//deal with the code that gets the physical device alltogether.
		int found_device = 0;
		for (uint32_t i = 0; i < dev_count; i++) {
			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(devices[i], &props);
			uint32_t queuefamilycount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queuefamilycount, NULL);
			VkBool32 surface_supported = 0;
			for (uint32_t j = 0; j < queuefamilycount; j++) {
				vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], j, vkstate->surface, &surface_supported);
				if (surface_supported && props.deviceType & VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
					found_device = 1;
					vkstate->phys_dev = devices[i];
					break;
				}
			}
		}
		if (!found_device) {
			LOG_ERROR("No suitable devices found\n");
			exit(-1);
		}
	}
}
void generate_logical_device(vk_state *vkstate) {
	{ // Device create info
		VkDeviceQueueCreateInfo *device_queue_create_info;
		{ //Device queue create info
			{ //Find suitable family
				//TODO:
				// similarly as described and reasoned earlier in the physical device part,
				// have your own function for gauging the suitibility of a queue family.
				// in the case of queue families, however, they have another issue. It really depends what you're going to be using the queue for,
				// and therefore the requirements need to be passed as an argument, or a new function would have to be made for every set of requirements
				// for this reason, i have to reccomend holding off on this until more research about what the most practical solution would be is done.

				//my personal ignorance makes this a similar task to asking children which gun is best for hunting. The easiest answer is: Don't ask them.
				//for this reason, I will hold this task off as long as possible, in hopes that my ignorance will be solved with prayer or some kind of collective-uncoscious osmosis

				uint32_t property_count;
				VkQueueFamilyProperties *props;
				vkGetPhysicalDeviceQueueFamilyProperties(vkstate->phys_dev, &property_count, NULL);
				props = malloc(sizeof(VkQueueFamilyProperties)*property_count);
				vkGetPhysicalDeviceQueueFamilyProperties(vkstate->phys_dev, &property_count, props);
				int found_one = 0;
				for (uint32_t index = 0; index < property_count; index++) {
					if (props[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
						vkstate->queue_family_index = index;
						found_one = 1;
						break;
					}
				}
				free(props);
				if (!found_one) {
					LOG_ERROR("Error, no device queue with graphics capabilities found!\n");
					exit(-1);
				}
			}
			float *priority = malloc(sizeof(float));
			*priority = 1.0;
			device_queue_create_info = calloc(sizeof(VkDeviceQueueCreateInfo), 1);
			device_queue_create_info->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			device_queue_create_info->queueFamilyIndex = vkstate->queue_family_index;
			device_queue_create_info->queueCount = 1;
			device_queue_create_info->pQueuePriorities = priority;
		}
		// char **device_extensions;
		uint32_t enabled_extension_count;
		char **req_exts;
		{ //get & verify device extensions
			req_exts = malloc(sizeof(char*)*1);
			req_exts[0] = "VK_KHR_swapchain";
			enabled_extension_count = 1;
			// 	check for a way to abstract this and the other one into one function

			uint32_t property_count;
			VkResult result = vkEnumerateDeviceExtensionProperties(vkstate->phys_dev, NULL, &property_count, NULL);
			if (result != VK_SUCCESS) {
				LOG_ERROR("Cannot count physical device extension properties\n");
				exit(-1);
			}
			VkExtensionProperties * properties = malloc(sizeof(VkExtensionProperties)*property_count);
			result = vkEnumerateDeviceExtensionProperties(vkstate->phys_dev, NULL, &property_count, NULL);
			if (result != VK_SUCCESS) {
				LOG_ERROR("Cannot enumerate physical device extension property\n");
				exit(-1);
			}
			int crash = 0;
			for (long unsigned i = 0; i < enabled_extension_count; i++) {
				if (!has(properties,property_count,req_exts[i])) {
					LOG_ERROR("Error: Missing extension %s\n",req_exts[i]);
					crash = 1;
				}
			}
			free(properties);
			if(crash) {
				exit(-1);
			}
		}


		VkDeviceCreateInfo *device_create_info = calloc(sizeof(VkDeviceCreateInfo),1);
		device_create_info->sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_create_info->queueCreateInfoCount = 1;
		device_create_info->pQueueCreateInfos = device_queue_create_info;
		device_create_info->ppEnabledExtensionNames = (const char**) req_exts;
		device_create_info->enabledExtensionCount = enabled_extension_count;
		device_create_info->pEnabledFeatures = NULL;
		vkstate->device_create_info = device_create_info;
	}
	vkCreateDevice(vkstate->phys_dev,vkstate->device_create_info,NULL,&vkstate->log_dev);
	{// get queue
		vkstate->device_queue = malloc(sizeof(vkstate->device_queue));
		vkGetDeviceQueue(vkstate->log_dev, vkstate->queue_family_index, 0, vkstate->device_queue);
	}
}
void generate_swapchain(vk_state *vkstate) {
	{ //swap chain info
		vkstate->swapchain_info = calloc(sizeof(VkSwapchainCreateInfoKHR),1);
		vkstate->swapchain_info->sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		vkstate->swapchain_info->surface = vkstate->surface;
		vkstate->swapchain_info->minImageCount = 3;
		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkstate->phys_dev, vkstate->surface, &capabilities);
		int width,height;
		SDL_Vulkan_GetDrawableSize(window, &width, &height);
		width = CLAMP((uint32_t)width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		height = CLAMP((uint32_t)height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		vkstate->swapchain_info->imageExtent = (VkExtent2D){.width=width,.height=height};
		vkstate->swapchain_info->imageArrayLayers = 1;
		vkstate->swapchain_info->imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		vkstate->swapchain_info->imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		vkstate->swapchain_info->preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		vkstate->swapchain_info->compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		vkstate->swapchain_info->presentMode = VK_PRESENT_MODE_FIFO_KHR;
		vkstate->swapchain_info->clipped = VK_TRUE;
		vkstate->swapchain_info->oldSwapchain = VK_NULL_HANDLE;
		VkSurfaceFormatKHR *formats;
		{//get formats
			uint32_t formats_len = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(vkstate->phys_dev, vkstate->surface, &formats_len, NULL);
			formats = malloc(sizeof(VkSurfaceFormatKHR)*formats_len);	
			vkGetPhysicalDeviceSurfaceFormatsKHR(vkstate->phys_dev, vkstate->surface, &formats_len, formats);
			if (formats_len == 0) {
				LOG_ERROR("No image formats supported for this device.\n");
				exit(-1);
			}
		}
		vkstate->swapchain_info->imageFormat = formats[1].format;
		vkstate->swapchain_info->imageColorSpace = formats[1].colorSpace;
	}
	vkstate->swapchain = malloc(sizeof(VkSwapchainKHR));
	vkCreateSwapchainKHR(vkstate->log_dev, vkstate->swapchain_info,NULL, vkstate->swapchain);
	{ //get swapchain images
		vkGetSwapchainImagesKHR(vkstate->log_dev, *vkstate->swapchain, &vkstate->swapchain_imagecount, NULL);
		vkstate->swapchain_images = malloc(sizeof(VkImage)*vkstate->swapchain_imagecount);
		vkGetSwapchainImagesKHR(vkstate->log_dev, *vkstate->swapchain, &vkstate->swapchain_imagecount, vkstate->swapchain_images);
	}
}
void generate_renderpass(vk_state *vkstate) {
	{ //render pass info
		VkAttachmentDescription *attachments;
		{ //attachment description
			attachments = calloc(sizeof(VkAttachmentDescription),1);
			attachments->format = vkstate->swapchain_info->imageFormat;
			attachments->samples = VK_SAMPLE_COUNT_1_BIT;
			attachments->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments->finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		}
		VkSubpassDescription * subpasses;
		{ //subpass description
			subpasses = calloc(sizeof(VkSubpassDescription),1);
			subpasses->pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpasses->colorAttachmentCount = 1;
			VkAttachmentReference * ref = malloc(sizeof(VkAttachmentReference));
			ref->layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			ref->attachment = 0;
			subpasses->pColorAttachments = ref;
		}
		VkSubpassDependency * dependency;
		{ //subpass dependency
			dependency = calloc(sizeof(VkSubpassDependency),1);
			dependency->srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency->dstSubpass = 0;
			dependency->srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency->dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency->srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			dependency->dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		}
		VkRenderPassCreateInfo *render_pass_info = calloc(sizeof(VkRenderPassCreateInfo),1);
		render_pass_info->sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info->attachmentCount = 1;
		render_pass_info->pAttachments = attachments;
		render_pass_info->subpassCount = 1;
		render_pass_info->pSubpasses = subpasses;
		render_pass_info->dependencyCount = 1;
		render_pass_info->pDependencies = dependency;
		vkstate->render_pass_info = render_pass_info;
	}
	vkstate->render_pass = malloc(sizeof(VkRenderPass));
	vkCreateRenderPass(vkstate->log_dev,vkstate->render_pass_info,NULL,vkstate->render_pass);
}
void generate_imageviews(vk_state *vkstate) {
	vkstate->imageview_infos = calloc(sizeof(VkImageViewCreateInfo)*vkstate->swapchain_imagecount,1);
	vkstate->imageviews = malloc(sizeof(VkImageView)*vkstate->swapchain_imagecount);
	for (uint32_t i = 0; i < vkstate->swapchain_imagecount; i++) {
		vkstate->imageview_infos[i].sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		vkstate->imageview_infos[i].image = vkstate->swapchain_images[i];
		vkstate->imageview_infos[i].viewType = VK_IMAGE_VIEW_TYPE_2D;
		vkstate->imageview_infos[i].format = VK_FORMAT_B8G8R8A8_SRGB;
		vkstate->imageview_infos[i].components = (VkComponentMapping){
			.r=VK_COMPONENT_SWIZZLE_IDENTITY,
			.g=VK_COMPONENT_SWIZZLE_IDENTITY,
			.b=VK_COMPONENT_SWIZZLE_IDENTITY,
			.a=VK_COMPONENT_SWIZZLE_IDENTITY};
		vkstate->imageview_infos[i].subresourceRange = (VkImageSubresourceRange) {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		};
		vkCreateImageView(vkstate->log_dev, &vkstate->imageview_infos[i], NULL, &(vkstate->imageviews[i]));
	}
}
void generate_framebuffers(vk_state *vkstate) {
	vkstate->framebuffer_infos = calloc(sizeof(VkFramebufferCreateInfo)*vkstate->swapchain_imagecount,1);
	vkstate->framebuffers = malloc(sizeof(VkFramebuffer)*vkstate->swapchain_imagecount);
	for (uint32_t i = 0; i < vkstate->swapchain_imagecount; i++) {
		vkstate->framebuffer_infos[i].sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		vkstate->framebuffer_infos[i].renderPass = *vkstate->render_pass;
		vkstate->framebuffer_infos[i].attachmentCount = 1;
		vkstate->framebuffer_infos[i].pAttachments = &(vkstate->imageviews[i]);
		vkstate->framebuffer_infos[i].width = vkstate->swapchain_info->imageExtent.width;
		vkstate->framebuffer_infos[i].height = vkstate->swapchain_info->imageExtent.height;
		vkstate->framebuffer_infos[i].layers = 1;
		vkCreateFramebuffer(vkstate->log_dev, &(vkstate->framebuffer_infos[i]), NULL, &(vkstate->framebuffers[i]));
	}
}
void generate_command_pool(vk_state *vkstate) {
	{ //command pool create info
		vkstate->command_pool_create_info = calloc(sizeof(VkCommandPoolCreateInfo),1);
		vkstate->command_pool_create_info->sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		vkstate->command_pool_create_info->queueFamilyIndex = vkstate->queue_family_index;
		vkstate->command_pool_create_info->flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	}
	vkCreateCommandPool(vkstate->log_dev, vkstate->command_pool_create_info, NULL, &vkstate->command_pool);
}
void generate_command_buffers(vk_state *vkstate) {
	{ //infos
		vkstate->cmd_allocate_info = calloc(sizeof(VkCommandBufferAllocateInfo),vkstate->swapchain_imagecount);
		vkstate->cmd_allocate_info->sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		vkstate->cmd_allocate_info->commandPool = vkstate->command_pool;
		vkstate->cmd_allocate_info->level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		vkstate->cmd_allocate_info->commandBufferCount = vkstate->swapchain_imagecount;
	}
	vkstate->cmd_buffers = malloc(sizeof(VkCommandBuffer)*vkstate->cmd_allocate_info->commandBufferCount);
	vkAllocateCommandBuffers(vkstate->log_dev, vkstate->cmd_allocate_info, vkstate->cmd_buffers);
}
void generate_graphics_pipeline(vk_state *vkstate) {
	vkstate->graphics_pipeline_create_info = calloc(sizeof(VkGraphicsPipelineCreateInfo),1);
	vkstate->graphics_pipeline_create_info->sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	{ //shader stage
		vkstate->graphics_pipeline_create_info->stageCount = 2;
		//TOD definitiely abstract these into one function
		//vertex shader
		{
			int size;
			uint32_t * vertex_code;
			read_file(VERTEX_SHADER_PATH, &size , &vertex_code);
			vkstate->vertex_shader_module_create_info = calloc(sizeof(VkShaderModuleCreateInfo),1);
			vkstate->vertex_shader_module_create_info->sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			vkstate->vertex_shader_module_create_info->codeSize = size;
			vkstate->vertex_shader_module_create_info->pCode = vertex_code;

			vkCreateShaderModule(vkstate->log_dev, vkstate->vertex_shader_module_create_info, NULL, &vkstate->vertex_shader_module);

			vkstate->vertex_pipeline_shader_stage_create_info = calloc(sizeof(VkPipelineShaderStageCreateInfo),1);
			vkstate->vertex_pipeline_shader_stage_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vkstate->vertex_pipeline_shader_stage_create_info->stage = VK_SHADER_STAGE_VERTEX_BIT;
			vkstate->vertex_pipeline_shader_stage_create_info->module = vkstate->vertex_shader_module;
			vkstate->vertex_pipeline_shader_stage_create_info->pName = "main";
		}
		//fragment shader
		{
			int size;
			uint32_t * fragment_code;
			read_file(FRAGMENT_SHADER_PATH, &size , &fragment_code);

			vkstate->fragment_shader_module_create_info = calloc(sizeof(VkShaderModuleCreateInfo),1);
			vkstate->fragment_shader_module_create_info->sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			vkstate->fragment_shader_module_create_info->codeSize = size;
			vkstate->fragment_shader_module_create_info->pCode = fragment_code;

			vkCreateShaderModule(vkstate->log_dev, vkstate->fragment_shader_module_create_info, NULL, &vkstate->fragment_shader_module);

			vkstate->fragment_pipeline_shader_stage_create_info = calloc(sizeof(VkPipelineShaderStageCreateInfo),1);
			vkstate->fragment_pipeline_shader_stage_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vkstate->fragment_pipeline_shader_stage_create_info->stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			vkstate->fragment_pipeline_shader_stage_create_info->module = vkstate->fragment_shader_module;
			vkstate->fragment_pipeline_shader_stage_create_info->pName = "main";
		}
	}
	{ //vertex input state create info
		vkstate->pipeline_vertex_input_state_create_info = calloc(sizeof(VkPipelineVertexInputStateCreateInfo),1);
		vkstate->pipeline_vertex_input_state_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		//highly temproray attribute descriptions and bindings
		VkVertexInputBindingDescription *binding_descs = malloc(sizeof(VkVertexInputBindingDescription)*2);
		binding_descs[0].binding = 0;
		binding_descs[0].stride = sizeof(vec2);
		binding_descs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		binding_descs[1].binding = 1;
		binding_descs[1].stride = sizeof(vec3);
		binding_descs[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription *attribute_descs= malloc(sizeof(VkVertexInputAttributeDescription)*2);	
		attribute_descs[0].binding = 0;
		attribute_descs[0].location = 0;
		attribute_descs[0].format = VK_FORMAT_R32G32_SFLOAT;
		attribute_descs[0].offset = 0;

		attribute_descs[1].binding = 1;
		attribute_descs[1].location = 1;
		attribute_descs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute_descs[1].offset = 0;


		vkstate->pipeline_vertex_input_state_create_info->vertexBindingDescriptionCount = 2;
		vkstate->pipeline_vertex_input_state_create_info->vertexAttributeDescriptionCount = 2;
		vkstate->pipeline_vertex_input_state_create_info->pVertexBindingDescriptions = binding_descs;
		vkstate->pipeline_vertex_input_state_create_info->pVertexAttributeDescriptions = attribute_descs;
	}
	{ //viewport state create info
		vkstate->pipeline_viewport_state_create_info = calloc(sizeof(VkPipelineViewportStateCreateInfo),1);
		vkstate->pipeline_viewport_state_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		vkstate->pipeline_viewport_state_create_info->pNext = NULL;
		vkstate->pipeline_viewport_state_create_info->viewportCount = 1;
		vkstate->pipeline_viewport_state_create_info->pViewports = &((VkViewport){
			.x = 0, .y = 0,
			.width = vkstate->swapchain_info->imageExtent.width,
			.height = vkstate->swapchain_info->imageExtent.height, .minDepth = 0.0, .maxDepth = 1.0});
		vkstate->pipeline_viewport_state_create_info->scissorCount = 1;
		vkstate->pipeline_viewport_state_create_info->pScissors = &(VkRect2D) {
			.offset = (VkOffset2D){0,0},
			.extent = (VkExtent2D){.width = vkstate->swapchain_info->imageExtent.width, .height = vkstate->swapchain_info->imageExtent.height}
		};
	}
	{ //rasterization state create info
		vkstate->pipeline_rasterization_state_create_info = calloc(sizeof(VkPipelineRasterizationStateCreateInfo),1);
		vkstate->pipeline_rasterization_state_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		//whether to clamp to a min and max depth. Nah.
		vkstate->pipeline_rasterization_state_create_info->depthClampEnable = VK_FALSE;
		//whether primitives are discraded immediately before the rasterization stage.
		//not sure what that means, but it sounds like it discards information that, if I don't want to use
		//I could just ignore. I'll enable it if it turns out later that I need/want it
		vkstate->pipeline_rasterization_state_create_info->rasterizerDiscardEnable = VK_FALSE;

		vkstate->pipeline_rasterization_state_create_info->polygonMode = VK_POLYGON_MODE_FILL;
		//culling. Don't see anything? try disabling it, maybe your triangles are facing the wrong way.
		vkstate->pipeline_rasterization_state_create_info->cullMode = VK_CULL_MODE_BACK_BIT;
		vkstate->pipeline_rasterization_state_create_info->frontFace = VK_FRONT_FACE_CLOCKWISE;
		vkstate->pipeline_rasterization_state_create_info->lineWidth = 1.0f;
		//bias depth values? nah
		vkstate->pipeline_rasterization_state_create_info->depthBiasEnable = VK_FALSE;
		vkstate->pipeline_rasterization_state_create_info->depthBiasConstantFactor = 0.0f;
		vkstate->pipeline_rasterization_state_create_info->depthBiasClamp = 0.0f;
		vkstate->pipeline_rasterization_state_create_info->depthBiasSlopeFactor = 0.0f;
	}
	{ //multisample state create info
		vkstate->pipeline_multisample_state_create_info = calloc(sizeof(VkPipelineMultisampleStateCreateInfo),1);
		vkstate->pipeline_multisample_state_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		vkstate->pipeline_multisample_state_create_info->rasterizationSamples = 1;
		//sample shading. Don't need it right now, could be fun.
		vkstate->pipeline_multisample_state_create_info->sampleShadingEnable = VK_FALSE;
		vkstate->pipeline_multisample_state_create_info->minSampleShading = 0.5;
		//tbh dunno
		vkstate->pipeline_multisample_state_create_info->alphaToCoverageEnable = VK_FALSE;
		vkstate->pipeline_multisample_state_create_info->alphaToOneEnable = VK_FALSE;
	}
	{ //input assembly state create info
		vkstate->pipeline_input_assembly_state_create_info = calloc(sizeof(VkPipelineInputAssemblyStateCreateInfo),1);
		vkstate->pipeline_input_assembly_state_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		//primitives, baby
		vkstate->pipeline_input_assembly_state_create_info->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		vkstate->pipeline_input_assembly_state_create_info->primitiveRestartEnable = VK_FALSE;
	}
	{ //color blend state create info
		vkstate->pipeline_color_blend_state_create_info = calloc(sizeof(VkPipelineColorBlendStateCreateInfo),1);
		vkstate->pipeline_color_blend_state_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		vkstate->pipeline_color_blend_state_create_info->attachmentCount = 1;
		vkstate->pipeline_color_blend_state_create_info->logicOpEnable = VK_FALSE;
		vkstate->pipeline_color_blend_state_create_info->logicOp = VK_LOGIC_OP_COPY;

		{ //color attachment
			VkPipelineColorBlendAttachmentState * attachment = calloc(sizeof(VkPipelineColorBlendAttachmentState),1);
			attachment->colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			attachment->blendEnable = VK_FALSE;
			vkstate->pipeline_color_blend_state_create_info->pAttachments = attachment; 
		}
		vkstate->pipeline_color_blend_state_create_info->blendConstants[0] = 0.0f;
		vkstate->pipeline_color_blend_state_create_info->blendConstants[1] = 0.0f;
		vkstate->pipeline_color_blend_state_create_info->blendConstants[2] = 0.0f;
		vkstate->pipeline_color_blend_state_create_info->blendConstants[3] = 0.0f;
	}
	// { //dynamic state create info
	// 	vkstate->pipeline_dynamic_state_create_info = calloc(sizeof(VkPipelineDynamicStateCreateInfo),1);
	// 	vkstate->pipeline_dynamic_state_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	// 	vkstate->pipeline_dynamic_state_create_info->pNext = NULL;
	// 	vkstate->pipeline_dynamic_state_create_info->dynamicStateCount = 1;
	// 	vkstate->pipeline_dynamic_state_create_info->pDynamicStates = &(VkDynamicState){};
	// 	// vkstate->pipeline_dynamic_state_create_info->
	// }
	{ //layout create info
		vkstate->pipeline_layout_create_info = calloc(sizeof(VkPipelineLayoutCreateInfo),1);
		vkstate->pipeline_layout = malloc(sizeof(VkPipelineLayout));
		vkstate->pipeline_layout_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		vkstate->pipeline_layout_create_info->pNext = NULL;
		//not # layouts, # descriptor sets included in the layout
		//since I'm not using any descriptors right now, 0
		vkstate->pipeline_layout_create_info->setLayoutCount = 0;
		vkstate->pipeline_layout_create_info->pSetLayouts = NULL;
		vkstate->pipeline_layout_create_info->pushConstantRangeCount = 0;
		vkstate->pipeline_layout_create_info->pPushConstantRanges = NULL;
		vkCreatePipelineLayout(vkstate->log_dev, vkstate->pipeline_layout_create_info, NULL, vkstate->pipeline_layout);
	}
	*(vkstate->graphics_pipeline_create_info) = (VkGraphicsPipelineCreateInfo) {
		.pInputAssemblyState = vkstate->pipeline_input_assembly_state_create_info,
		.pVertexInputState = vkstate->pipeline_vertex_input_state_create_info,
		.pRasterizationState = vkstate->pipeline_rasterization_state_create_info,
		.pStages = (VkPipelineShaderStageCreateInfo[2]){*vkstate->vertex_pipeline_shader_stage_create_info, *vkstate->fragment_pipeline_shader_stage_create_info},
		.pColorBlendState = vkstate->pipeline_color_blend_state_create_info,
		.pMultisampleState = vkstate->pipeline_multisample_state_create_info,
		.pViewportState = vkstate->pipeline_viewport_state_create_info,
		.renderPass = *vkstate->render_pass,
		.layout = *vkstate->pipeline_layout,
		.pDynamicState = NULL,
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.basePipelineHandle = NULL,
		.basePipelineIndex = 0,
		.subpass = 0,
		.flags = 0,
		.stageCount = 2,
	};
	// vkstate->graphics_pipeline_create_info->pDynamicState = vkstate->pipeline_dynamic_state_create_info;

	vkCreateGraphicsPipelines(vkstate->log_dev, VK_NULL_HANDLE, 1, vkstate->graphics_pipeline_create_info, NULL, &vkstate->graphics_pipeline);
}
void generate_semaphores(vk_state *vkstate) {
	{ //semaphore create info
		vkstate->semaphore_create_info = calloc(sizeof(VkSemaphoreCreateInfo),2);
		vkstate->semaphore_create_info->sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	}
	vkstate->semaphores = malloc(sizeof(VkSemaphore)*2);
	vkCreateSemaphore(vkstate->log_dev, vkstate->semaphore_create_info, NULL, &(vkstate->semaphores[0]));
	vkCreateSemaphore(vkstate->log_dev, vkstate->semaphore_create_info, NULL, &(vkstate->semaphores[1]));
}
void generate_fences(vk_state *vkstate) {
	{//create info
		vkstate->fence_create_info = calloc(sizeof(VkFenceCreateInfo),1);
		vkstate->fence_create_info->sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		vkstate->fence_create_info->flags = VK_FENCE_CREATE_SIGNALED_BIT;
	}
	vkstate->fences = malloc(sizeof(VkFence));
	vkCreateFence(vkstate->log_dev, vkstate->fence_create_info, NULL, vkstate->fences);
}

//TODO:
//instead of manually destroying the dependencies here, give them their own cleanup functions.
//only cleanup and free swapchain related things, and then call cleanup functions for anything directly dependant on it.
//dependencies of dependencies, like the renderpass(Which, IIRC, is depenent on imageviews generated using swapchain images) will be handled by a direct dependency.
//this way, no function leaves invalid handles or leaks memory, and there are no huge monolithic functions that violate the single-responsibility principle.
void cleanup_swapchain(vk_state *vkstate) {
	// for (int i = 0; i < vkstate->)
	// cleanup_vertex_buffers(vkstate);
	vkDestroyFramebuffer(vkstate->log_dev, *vkstate->framebuffers, NULL);
	vkDestroyPipeline(vkstate->log_dev, vkstate->graphics_pipeline, NULL);
	vkDestroyPipelineLayout(vkstate->log_dev, *vkstate->pipeline_layout, NULL);
	vkDestroyRenderPass(vkstate->log_dev, *vkstate->render_pass, NULL);
	for (uint32_t i = 0; i < vkstate->swapchain_imagecount;i++) {
		vkDestroyImageView(vkstate->log_dev, vkstate->imageviews[i], NULL);
	}
	vkDestroySwapchainKHR(vkstate->log_dev, *vkstate->swapchain, NULL);
	free(vkstate->swapchain);
	free(vkstate->swapchain_info);
	free(vkstate->swapchain_images);
	free(vkstate->cmd_begin_info);
	free(vkstate->render_pass_begin_infos);
}

void vulkan_before_buffers_init(void) {
	/*
		TODO:
			make function that has their logical dependencies understood.
			With the logical dependencies, as well as custom-set generate_x functions (these will be 'default'), regenerate anything that needs to be regenerated,
			free anything that needs freed, etc, so that things like swapchain regeneration and replacement can be done with just one function call.
	*/
	vkstate.swapchain = VK_NULL_HANDLE;
	generate_instance(&vkstate);
	generate_surface(&vkstate);
	generate_physical_device(&vkstate);
	generate_logical_device(&vkstate); 
	generate_swapchain(&vkstate);
	generate_renderpass(&vkstate);
	generate_imageviews(&vkstate);
	generate_framebuffers(&vkstate);
	generate_command_pool(&vkstate);
	generate_command_buffers(&vkstate);
	generate_graphics_pipeline(&vkstate);
	generate_semaphores(&vkstate);
	generate_fences(&vkstate);
	write_command_buffers(&vkstate);
}
// void vulkan_after_buffers_init(void) {
// 	create_vertex_buffers(&vkstate);
// 	write_command_buffers(&vkstate);
// }
void recreate_swapchain(vk_state *vkstate) {
	vkDeviceWaitIdle(vkstate->log_dev);
	//when recreating the swapchain, you also need to recreate all of it's dependencies.
	//because you can't remove a swapchain without also invalidating all of it's dependencies, cleanup_swapchain is all that needs to be called.
	cleanup_swapchain(vkstate);
	generate_swapchain(vkstate);
	generate_imageviews(vkstate);
	generate_renderpass(vkstate);
	generate_graphics_pipeline(vkstate);
	generate_framebuffers(vkstate);
	// write_command_buffers(vkstate);
}
void vulkan_update(int dt) { 
	vkWaitForFences(vkstate.log_dev, 1, &vkstate.fences[0], VK_TRUE, UINT64_MAX);
	uint32_t image_index;
	VkResult result = vkAcquireNextImageKHR(vkstate.log_dev, *vkstate.swapchain, 100000000, vkstate.semaphores[0], VK_NULL_HANDLE, &image_index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreate_swapchain(&vkstate);
		return;
	}
	vkResetFences(vkstate.log_dev,1,&vkstate.fences[0]);
	uint32_t flag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	result = vkQueueSubmit(*vkstate.device_queue, 1, &(VkSubmitInfo) {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = NULL,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vkstate.semaphores[0],
		.pWaitDstStageMask = &flag,
		.commandBufferCount = 1,
		.pCommandBuffers = &(vkstate.cmd_buffers[image_index]),
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &vkstate.semaphores[1],
	}, vkstate.fences[0]);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreate_swapchain(&vkstate);
		return;
	}
	VkPresentInfoKHR presentInfo = {0};
	presentInfo = (VkPresentInfoKHR) {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vkstate.semaphores[1],
		.swapchainCount = 1,
		.pSwapchains = vkstate.swapchain,
		.pImageIndices = &image_index,
	};
	result = vkQueuePresentKHR(*vkstate.device_queue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreate_swapchain(&vkstate);
		return;
	}

	// exit(-1);
}