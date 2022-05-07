#include "log/log.h"

#define VK_ENABLE_BEHTA_EXTENSIONS
#include <vulkan/vulkan.h>
// #include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include "sdl/sdl.h"
#include "log/log.h"
#include <stdlib.h>
#include <string.h>
// #define STORE(NON_STRUCT_PTR,TYPE,STRUCT) do { TYPE * ptr = malloc(sizeof(TYPE)); memcpy(ptr,NON_STRUCT_PTR,sizeof(TYPE)); STRUCT.NON_STRUCT_PTR = ptr; } while 0
#define CLAMP(x, lo, hi)    ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))
//this is entirely a struct to store all objects created and information that was required to create them.
//if a info struct has a pointer to another struct within it, only the outermost struct must be put here.

/*
TODO:
	1.
		Clean up the vkstate struct by taking anything which is referenced through a create_info out of the struct.
		Point is to minimize the size of vkstate without loosing information needed to create or recreate any part of it.
	2.
		Move all the major steps, right now categorized through brackets into their own functions.
		Have them take as variables vkstate and any information regarding whatever it is they are creating (create infos, primarily)
	3.
		Make "template" create info generator(s), and put them in a different file.
		This way, you will be able to make a swapchain that fills the vkstate with gen_swapchain(&vkstruct, default_swapchain_template(vkstate) ),
		but if I find another swapchain info I want to use instead, gen_swapchain can be called with some other swapchain_info as the second argument.

		It must be a generator because it has to reference things that have been dealt with before in the vkstate.
		For instance, many functions require a logical device. Instead of passing everything, I pass a createinfo generated with vkstate available
		to pull things like log_dev and imageviews whenever needed.
	4.
		Make cleanup functions, and set up clean recreation of the different objects (That is, recreate the object and everything of which their validitiy requires
			recreation because of the first object being recreated)
		I'm considering doing this with a dependency graph, since sometimes things which seem like they could be dependant happen not to be,
		but that may be an overcomplication, and I may just make a specific ordering for the gen and cleanup functions to run in. 
	5.
		Set up dynamic state, and descriptors
	6.
		Change makefile to compile shaders with glslc if they are found.
	7.
		Do something about the compile-time shaders paths
	8.
		Use a more descriptive name than 'i' for your for loops.
*/


typedef struct {
	VkInstanceCreateInfo *instance_info;
	VkInstance instance;

	VkSurfaceKHR surface;

	VkPhysicalDevice phys_dev;
	VkDeviceCreateInfo *device_create_info;
	VkDevice log_dev;
	uint32_t queue_family_index;
	VkQueue *device_queue;

	VkCommandPoolCreateInfo *command_pool_create_info;
	VkCommandPool command_pool;

	VkRenderPassCreateInfo *render_pass_info;
	VkRenderPass *render_pass;

	VkSwapchainCreateInfoKHR *swapchain_info;
	VkSwapchainKHR *swapchain;
	uint32_t swapchain_imagecount;
	VkImage *swapchain_images;

	VkImageViewCreateInfo *imageview_infos;
	VkImageView *imageviews;

	VkFramebufferCreateInfo *framebuffer_infos;
	VkFramebuffer *framebuffers;

	VkCommandBufferAllocateInfo *cmd_allocate_info;
	VkCommandBuffer *cmd_buffers;

	VkCommandBufferBeginInfo *cmd_begin_info;

	VkRenderPassBeginInfo *render_pass_begin_info;


	VkShaderModuleCreateInfo *vertex_shader_module_create_info;
	VkShaderModule vertex_shader_module;

	VkShaderModuleCreateInfo *fragment_shader_module_create_info;
	VkShaderModule fragment_shader_module;

	VkPipelineShaderStageCreateInfo *vertex_pipeline_shader_stage_create_info;
	VkPipelineShaderStageCreateInfo *fragment_pipeline_shader_stage_create_info;

	VkPipelineVertexInputStateCreateInfo *pipeline_vertex_input_state_create_info;

	VkPipelineViewportStateCreateInfo *pipeline_viewport_state_create_info;

	VkPipelineRasterizationStateCreateInfo *pipeline_rasterization_state_create_info;

	VkPipelineMultisampleStateCreateInfo *pipeline_multisample_state_create_info;
	
	VkPipelineInputAssemblyStateCreateInfo *pipeline_input_assembly_state_create_info;

	VkPipelineColorBlendStateCreateInfo *pipeline_color_blend_state_create_info;

	VkPipelineLayoutCreateInfo *pipeline_layout_create_info;

	VkGraphicsPipelineCreateInfo *graphics_pipeline_create_info;

	VkPipelineLayout *pipeline_layout;


	VkPipeline graphics_pipeline;
	VkSemaphoreCreateInfo *semaphore_create_info;
	VkSemaphore *semaphores;
	VkFenceCreateInfo *fence_create_info;
	VkFence *fences;
} vk_state;
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

void vulkan_init(void) {
	{ // create an instance
		{ //instance info (Add it to vkstate)
			VkInstanceCreateInfo *instance_info = calloc(1,sizeof(VkInstanceCreateInfo));
			{ //application info
				VkApplicationInfo *appinfo = calloc(1,sizeof(VkApplicationInfo));
				appinfo->sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
				appinfo->pApplicationName = "Hi world.";
				// 040?
				appinfo->applicationVersion = 040;
				appinfo->apiVersion = VK_MAKE_VERSION(1, 2, 0);
				instance_info->pApplicationInfo = appinfo;
			}
			instance_info->sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			{  // Getting and verifying the instance extensions
				
				char **req_exts = malloc(sizeof(char*)*1);
				req_exts[0] = "VK_KHR_surface";
				uint32_t enabled_extension_count = 1;

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
						for (uint32_t i = enabled_extension_count; i < enabled_extension_count+pcount; i++) {
							//add one for null character not counted as part of the string length
							req_exts[i] = malloc(strlen(pnames[i-enabled_extension_count]) + 1);
							memcpy(req_exts[i], pnames[i-enabled_extension_count], strlen(pnames[i-enabled_extension_count]) + 1);
						}
						enabled_extension_count += pcount;
						free(pnames);
					}
				}
				instance_info->ppEnabledExtensionNames = (const char **)req_exts;
				instance_info->enabledExtensionCount = enabled_extension_count;


			}
			{//validation layers
				instance_info->ppEnabledLayerNames = (const char * const[]){"VK_LAYER_KHRONOS_validation"};
				instance_info->enabledLayerCount = 1;
			}
			vkstate.instance_info = instance_info;

		}
		vkCreateInstance(vkstate.instance_info, NULL, &vkstate.instance);
	}
	{ //create a surface
		if (SDL_Vulkan_CreateSurface(window,vkstate.instance,&vkstate.surface) == SDL_FALSE) {
			LOG_ERROR("Error, can't create surface, SDL_VulkanCreateSurface failed.\n");
			exit(-1);
		}
	}
	{ //get physical device
		uint32_t dev_count;
		VkPhysicalDevice * devices;
		{ //get device list & count
			VkResult result = vkEnumeratePhysicalDevices(vkstate.instance, &dev_count, NULL);
			if (result != VK_SUCCESS) {
				LOG_ERROR("Error, can't get number of physical devices. Error num: %d\n",result);
				exit(-1);
			}
			devices = malloc(sizeof(VkPhysicalDevice)*dev_count);
			result = vkEnumeratePhysicalDevices(vkstate.instance, &dev_count, devices);
			if (result != VK_SUCCESS) {
				LOG_ERROR("Error, can't enumerate physical devices. Error num: %d\n",result);
				exit(-1);
			}
		}
		{ //select suitable device
			//TODO: When turning things into functions, a suitability function for devices?
			//this way, as dev understanding of what is and isn't required changes, they just need to modify that, rather than
			//deal with the code that gets the physical device alltogether. SRP
			int found_device = 0;
			for (uint32_t i = 0; i < dev_count; i++) {
				VkPhysicalDeviceProperties props;
				vkGetPhysicalDeviceProperties(devices[i], &props);
				uint32_t queuefamilycount = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queuefamilycount, NULL);
				VkBool32 surface_supported = 0;
				for (uint32_t j = 0; j < queuefamilycount; j++) {
					vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], j, vkstate.surface, &surface_supported);
				}
				if (surface_supported && props.deviceType & VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
					found_device = 1;
					vkstate.phys_dev = devices[i];
					break;
				}
			}
			if (!found_device) {
				LOG_ERROR("No suitable devices found\n");
				exit(-1);
			}
		}
	}
	{ //create a logical device 
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
						uint32_t property_count;
						VkQueueFamilyProperties *props;
						vkGetPhysicalDeviceQueueFamilyProperties(vkstate.phys_dev, &property_count, NULL);
						props = malloc(sizeof(VkQueueFamilyProperties)*property_count);
						vkGetPhysicalDeviceQueueFamilyProperties(vkstate.phys_dev, &property_count, props);
						int found_one = 0;
						for (uint32_t index = 0; index < property_count; index++) {
							if (props[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
								vkstate.queue_family_index = index;
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
					device_queue_create_info->queueFamilyIndex = vkstate.queue_family_index;
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
					VkResult result = vkEnumerateDeviceExtensionProperties(vkstate.phys_dev, NULL, &property_count, NULL);
					if (result != VK_SUCCESS) {
						LOG_ERROR("Cannot count physical device extension properties\n");
						exit(-1);
					}
					VkExtensionProperties * properties = malloc(sizeof(VkExtensionProperties)*property_count);
					result = vkEnumerateDeviceExtensionProperties(vkstate.phys_dev, NULL, &property_count, NULL);
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
				vkstate.device_create_info = device_create_info;
			}
			vkCreateDevice(vkstate.phys_dev,vkstate.device_create_info,NULL,&vkstate.log_dev);
			{// get queue
				vkstate.device_queue = malloc(sizeof(vkstate.device_queue));
				vkGetDeviceQueue(vkstate.log_dev, vkstate.queue_family_index, 0, vkstate.device_queue);
			}
	}
	{ //create a swap chain
		{ //swap chain info
			
			vkstate.swapchain_info = calloc(sizeof(VkSwapchainCreateInfoKHR),1);
			vkstate.swapchain_info->sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			vkstate.swapchain_info->surface = vkstate.surface;
			vkstate.swapchain_info->minImageCount = 2;
			VkSurfaceCapabilitiesKHR capabilities;
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkstate.phys_dev, vkstate.surface, &capabilities);
			int width,height;
			SDL_Vulkan_GetDrawableSize(window, &width, &height);
			width = CLAMP((uint32_t)width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			height = CLAMP((uint32_t)height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
			vkstate.swapchain_info->imageExtent = (VkExtent2D){.width=width,.height=height};
			vkstate.swapchain_info->imageArrayLayers = 1;
			vkstate.swapchain_info->imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			vkstate.swapchain_info->imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			vkstate.swapchain_info->preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
			vkstate.swapchain_info->compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			vkstate.swapchain_info->presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
			vkstate.swapchain_info->clipped = VK_TRUE;
			vkstate.swapchain_info->oldSwapchain = VK_NULL_HANDLE;

			VkSurfaceFormatKHR *formats;
			{//get formats
				uint32_t formats_len = 0;
				vkGetPhysicalDeviceSurfaceFormatsKHR(vkstate.phys_dev, vkstate.surface, &formats_len, NULL);
				formats = malloc(sizeof(VkSurfaceFormatKHR)*formats_len);	
				vkGetPhysicalDeviceSurfaceFormatsKHR(vkstate.phys_dev, vkstate.surface, &formats_len, formats);
				if (formats_len == 0) {
					LOG_ERROR("No image formats supported for this device.\n");
					exit(-1);
				}
			}
			vkstate.swapchain_info->imageFormat = formats[1].format;
			vkstate.swapchain_info->imageColorSpace = formats[1].colorSpace;
		}
		vkstate.swapchain = malloc(sizeof(VkSwapchainKHR));
		vkCreateSwapchainKHR(vkstate.log_dev, vkstate.swapchain_info,NULL, vkstate.swapchain);
		{ //get swapchain images
			vkGetSwapchainImagesKHR(vkstate.log_dev, *vkstate.swapchain, &vkstate.swapchain_imagecount, NULL);
			vkstate.swapchain_images = malloc(sizeof(VkImage)*vkstate.swapchain_imagecount);
			vkGetSwapchainImagesKHR(vkstate.log_dev, *vkstate.swapchain, &vkstate.swapchain_imagecount, vkstate.swapchain_images);
		}
	}
	{ //create a render pass
		{ //render pass info
			VkAttachmentDescription *attachments;
			{ //attachment description
				attachments = calloc(sizeof(VkAttachmentDescription),1);
				attachments->format = vkstate.swapchain_info->imageFormat;
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
			vkstate.render_pass_info = render_pass_info;
		}
		vkstate.render_pass = malloc(sizeof(VkRenderPass));
		vkCreateRenderPass(vkstate.log_dev,vkstate.render_pass_info,NULL,vkstate.render_pass);
	}
	{ //create image views
		vkstate.imageview_infos = calloc(sizeof(VkImageViewCreateInfo)*2,1);
		vkstate.imageviews = malloc(sizeof(VkImageView)*2);
		for (uint32_t i = 0; i < vkstate.swapchain_imagecount; i++) {
			vkstate.imageview_infos[i].sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			vkstate.imageview_infos[i].image = vkstate.swapchain_images[i];
			vkstate.imageview_infos[i].viewType = VK_IMAGE_VIEW_TYPE_2D;
			vkstate.imageview_infos[i].format = VK_FORMAT_B8G8R8A8_SRGB;
			vkstate.imageview_infos[i].components = (VkComponentMapping){
				.r=VK_COMPONENT_SWIZZLE_IDENTITY,
				.g=VK_COMPONENT_SWIZZLE_IDENTITY,
				.b=VK_COMPONENT_SWIZZLE_IDENTITY,
				.a=VK_COMPONENT_SWIZZLE_IDENTITY};
			vkstate.imageview_infos[i].subresourceRange = (VkImageSubresourceRange) {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			};
			vkCreateImageView(vkstate.log_dev, &vkstate.imageview_infos[i], NULL, &(vkstate.imageviews[i]));
		}
	}
	{ //create frame buffers
		vkstate.framebuffer_infos = calloc(sizeof(VkFramebufferCreateInfo)*2,1);
		vkstate.framebuffers = malloc(sizeof(VkFramebuffer)*2);
		for (uint32_t i = 0; i < vkstate.swapchain_imagecount; i++) {
			vkstate.framebuffer_infos[i].sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			vkstate.framebuffer_infos[i].renderPass = *vkstate.render_pass;
			vkstate.framebuffer_infos[i].attachmentCount = 1;
			vkstate.framebuffer_infos[i].pAttachments = &(vkstate.imageviews[i]);
			vkstate.framebuffer_infos[i].width = vkstate.swapchain_info->imageExtent.width;
			vkstate.framebuffer_infos[i].height = vkstate.swapchain_info->imageExtent.height;
			vkstate.framebuffer_infos[i].layers = 1;
		vkCreateFramebuffer(vkstate.log_dev, &(vkstate.framebuffer_infos[i]), NULL, &(vkstate.framebuffers[i]));
		}
	}
	{ //create a command pool
		{ //command pool create info
			vkstate.command_pool_create_info = calloc(sizeof(VkCommandPoolCreateInfo),1);
			vkstate.command_pool_create_info->sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			vkstate.command_pool_create_info->queueFamilyIndex = vkstate.queue_family_index;
			vkstate.command_pool_create_info->flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		}
		vkCreateCommandPool(vkstate.log_dev, vkstate.command_pool_create_info, NULL, &vkstate.command_pool);
	}
	//allocating one for each swapchain image, that way 
	//you can present them based on an image index without having to reset and re-record the command buffer every time
	{ //allocate command buffers
		{ //infos
			vkstate.cmd_allocate_info = calloc(sizeof(VkCommandBufferAllocateInfo),vkstate.swapchain_imagecount);
			vkstate.cmd_allocate_info->sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			vkstate.cmd_allocate_info->commandPool = vkstate.command_pool;
			vkstate.cmd_allocate_info->level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			vkstate.cmd_allocate_info->commandBufferCount = vkstate.swapchain_imagecount;
		}
		vkstate.cmd_buffers = malloc(sizeof(VkCommandBuffer)*vkstate.cmd_allocate_info->commandBufferCount);
		vkAllocateCommandBuffers(vkstate.log_dev, vkstate.cmd_allocate_info, vkstate.cmd_buffers);
	}
	{ //create a graphics pipeline
		vkstate.graphics_pipeline_create_info = calloc(sizeof(VkGraphicsPipelineCreateInfo),1);
		vkstate.graphics_pipeline_create_info->sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		{ //shader stage
			vkstate.graphics_pipeline_create_info->stageCount = 2;
			//TOD definitiely abstract these into one function
			//vertex shader
			{
				int size;
				uint32_t * vertex_code;
				read_file(VERTEX_SHADER_PATH, &size , &vertex_code);
				vkstate.vertex_shader_module_create_info = calloc(sizeof(VkShaderModuleCreateInfo),1);
				vkstate.vertex_shader_module_create_info->sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				vkstate.vertex_shader_module_create_info->codeSize = size;
				vkstate.vertex_shader_module_create_info->pCode = vertex_code;

				vkCreateShaderModule(vkstate.log_dev, vkstate.vertex_shader_module_create_info, NULL, &vkstate.vertex_shader_module);

				vkstate.vertex_pipeline_shader_stage_create_info = calloc(sizeof(VkPipelineShaderStageCreateInfo),1);
				vkstate.vertex_pipeline_shader_stage_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				vkstate.vertex_pipeline_shader_stage_create_info->stage = VK_SHADER_STAGE_VERTEX_BIT;
				vkstate.vertex_pipeline_shader_stage_create_info->module = vkstate.vertex_shader_module;
				vkstate.vertex_pipeline_shader_stage_create_info->pName = "main";
			}
			//fragment shader
			{
				int size;
				uint32_t * fragment_code;
				read_file(FRAGMENT_SHADER_PATH, &size , &fragment_code);

				vkstate.fragment_shader_module_create_info = calloc(sizeof(VkShaderModuleCreateInfo),1);
				vkstate.fragment_shader_module_create_info->sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				vkstate.fragment_shader_module_create_info->codeSize = size;
				vkstate.fragment_shader_module_create_info->pCode = fragment_code;

				vkCreateShaderModule(vkstate.log_dev, vkstate.fragment_shader_module_create_info, NULL, &vkstate.fragment_shader_module);

				vkstate.fragment_pipeline_shader_stage_create_info = calloc(sizeof(VkPipelineShaderStageCreateInfo),1);
				vkstate.fragment_pipeline_shader_stage_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				vkstate.fragment_pipeline_shader_stage_create_info->stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				vkstate.fragment_pipeline_shader_stage_create_info->module = vkstate.fragment_shader_module;
				vkstate.fragment_pipeline_shader_stage_create_info->pName = "main";
			}
		}
		{ //vertex input state create info
			vkstate.pipeline_vertex_input_state_create_info = calloc(sizeof(VkPipelineVertexInputStateCreateInfo),1);
			vkstate.pipeline_vertex_input_state_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vkstate.pipeline_vertex_input_state_create_info->vertexBindingDescriptionCount = 0;
			vkstate.pipeline_vertex_input_state_create_info->vertexAttributeDescriptionCount = 0;
		}
		{ //viewport state create info
			vkstate.pipeline_viewport_state_create_info = calloc(sizeof(VkPipelineViewportStateCreateInfo),1);
			vkstate.pipeline_viewport_state_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			vkstate.pipeline_viewport_state_create_info->pNext = NULL;
			vkstate.pipeline_viewport_state_create_info->viewportCount = 1;
			vkstate.pipeline_viewport_state_create_info->pViewports = &((VkViewport){
				.x = 0, .y = 0,
				.width = vkstate.swapchain_info->imageExtent.width,
				.height = vkstate.swapchain_info->imageExtent.height, .minDepth = 0.0, .maxDepth = 1.0});
			vkstate.pipeline_viewport_state_create_info->scissorCount = 1;
			vkstate.pipeline_viewport_state_create_info->pScissors = &(VkRect2D) {
				.offset = (VkOffset2D){0,0},
				.extent = (VkExtent2D){.width = vkstate.swapchain_info->imageExtent.width, .height = vkstate.swapchain_info->imageExtent.height}
			};
		}
		{ //rasterization state create info
			vkstate.pipeline_rasterization_state_create_info = calloc(sizeof(VkPipelineRasterizationStateCreateInfo),1);
			vkstate.pipeline_rasterization_state_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			//whether to clamp to a min and max depth. Nah.
			vkstate.pipeline_rasterization_state_create_info->depthClampEnable = VK_FALSE;
			//whether primitives are discraded immediately before the rasterization stage.
			//not sure what that means, but it sounds like it discards information that, if I don't want to use
			//I could just ignore. I'll enable it if it turns out later that I need/want it
			vkstate.pipeline_rasterization_state_create_info->rasterizerDiscardEnable = VK_FALSE;

			vkstate.pipeline_rasterization_state_create_info->polygonMode = VK_POLYGON_MODE_FILL;
			//culling. Don't see anything? try disabling it, maybe your triangles are facing the wrong way.
			vkstate.pipeline_rasterization_state_create_info->cullMode = VK_CULL_MODE_BACK_BIT;
			vkstate.pipeline_rasterization_state_create_info->frontFace = VK_FRONT_FACE_CLOCKWISE;
			vkstate.pipeline_rasterization_state_create_info->lineWidth = 1.0f;
			//bias depth values? nah
			vkstate.pipeline_rasterization_state_create_info->depthBiasEnable = VK_FALSE;
			vkstate.pipeline_rasterization_state_create_info->depthBiasConstantFactor = 0.0f;
			vkstate.pipeline_rasterization_state_create_info->depthBiasClamp = 0.0f;
			vkstate.pipeline_rasterization_state_create_info->depthBiasSlopeFactor = 0.0f;
		}
		{ //multisample state create info
			vkstate.pipeline_multisample_state_create_info = calloc(sizeof(VkPipelineMultisampleStateCreateInfo),1);
			vkstate.pipeline_multisample_state_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			vkstate.pipeline_multisample_state_create_info->rasterizationSamples = 1;
			//sample shading. Don't need it right now, could be fun.
			vkstate.pipeline_multisample_state_create_info->sampleShadingEnable = VK_FALSE;
			vkstate.pipeline_multisample_state_create_info->minSampleShading = 0.5;
			//tbh dunno
			vkstate.pipeline_multisample_state_create_info->alphaToCoverageEnable = VK_FALSE;
			vkstate.pipeline_multisample_state_create_info->alphaToOneEnable = VK_FALSE;
		}
		{ //input assembly state create info
			vkstate.pipeline_input_assembly_state_create_info = calloc(sizeof(VkPipelineInputAssemblyStateCreateInfo),1);
			vkstate.pipeline_input_assembly_state_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			//primitives, baby
			vkstate.pipeline_input_assembly_state_create_info->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			vkstate.pipeline_input_assembly_state_create_info->primitiveRestartEnable = VK_FALSE;
		}
		{ //Color blend state create info
			vkstate.pipeline_color_blend_state_create_info = calloc(sizeof(VkPipelineColorBlendStateCreateInfo),1);
			vkstate.pipeline_color_blend_state_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			vkstate.pipeline_color_blend_state_create_info->attachmentCount = 1;
			vkstate.pipeline_color_blend_state_create_info->logicOpEnable = VK_FALSE;
			vkstate.pipeline_color_blend_state_create_info->logicOp = VK_LOGIC_OP_COPY;

			{ //color attachment
				VkPipelineColorBlendAttachmentState * attachment = calloc(sizeof(VkPipelineColorBlendAttachmentState),1);
				attachment->colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
				attachment->blendEnable = VK_FALSE;
				vkstate.pipeline_color_blend_state_create_info->pAttachments = attachment; 
			}
			vkstate.pipeline_color_blend_state_create_info->blendConstants[0] = 0.0f;
			vkstate.pipeline_color_blend_state_create_info->blendConstants[1] = 0.0f;
			vkstate.pipeline_color_blend_state_create_info->blendConstants[2] = 0.0f;
			vkstate.pipeline_color_blend_state_create_info->blendConstants[3] = 0.0f;
		}
		{ //layout create info
			vkstate.pipeline_layout_create_info = calloc(sizeof(VkPipelineLayoutCreateInfo),1);
			vkstate.pipeline_layout = malloc(sizeof(VkPipelineLayout));
			vkstate.pipeline_layout_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			vkstate.pipeline_layout_create_info->pNext = NULL;
			//not # layouts, # descriptor sets included in the layout
			//since I'm not using any descriptors right now, 0
			vkstate.pipeline_layout_create_info->setLayoutCount = 0;
			vkstate.pipeline_layout_create_info->pSetLayouts = NULL;
			vkstate.pipeline_layout_create_info->pushConstantRangeCount = 0;
			vkstate.pipeline_layout_create_info->pPushConstantRanges = NULL;
			vkCreatePipelineLayout(vkstate.log_dev, vkstate.pipeline_layout_create_info, NULL, vkstate.pipeline_layout);
		}
		vkstate.graphics_pipeline_create_info->pInputAssemblyState = vkstate.pipeline_input_assembly_state_create_info;
		vkstate.graphics_pipeline_create_info->pVertexInputState = vkstate.pipeline_vertex_input_state_create_info;
		vkstate.graphics_pipeline_create_info->pRasterizationState = vkstate.pipeline_rasterization_state_create_info;
		vkstate.graphics_pipeline_create_info->pStages = (VkPipelineShaderStageCreateInfo[2]){*vkstate.vertex_pipeline_shader_stage_create_info, *vkstate.fragment_pipeline_shader_stage_create_info};
		vkstate.graphics_pipeline_create_info->pColorBlendState = vkstate.pipeline_color_blend_state_create_info;
		vkstate.graphics_pipeline_create_info->pMultisampleState = vkstate.pipeline_multisample_state_create_info;
		vkstate.graphics_pipeline_create_info->pViewportState = vkstate.pipeline_viewport_state_create_info;
		vkstate.graphics_pipeline_create_info->renderPass = *vkstate.render_pass;
		vkstate.graphics_pipeline_create_info->layout = *vkstate.pipeline_layout;

		vkCreateGraphicsPipelines(vkstate.log_dev, VK_NULL_HANDLE, 1, vkstate.graphics_pipeline_create_info, NULL, &vkstate.graphics_pipeline);
	}
	{ //create semaphores
		{ //semaphore create info
			vkstate.semaphore_create_info = calloc(sizeof(VkSemaphoreCreateInfo),1);
			vkstate.semaphore_create_info->sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		}
		vkstate.semaphores = malloc(sizeof(VkSemaphore)*2);
		vkCreateSemaphore(vkstate.log_dev, vkstate.semaphore_create_info, NULL, &(vkstate.semaphores[0]));
		vkCreateSemaphore(vkstate.log_dev, vkstate.semaphore_create_info, NULL, &(vkstate.semaphores[1]));
	}
	{ //create fences
		{//create info
			vkstate.fence_create_info = calloc(sizeof(VkFenceCreateInfo),1);
			vkstate.fence_create_info->sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			vkstate.fence_create_info->flags = VK_FENCE_CREATE_SIGNALED_BIT;
		}
		vkstate.fences = malloc(sizeof(VkFence));
		vkCreateFence(vkstate.log_dev, vkstate.fence_create_info, NULL, vkstate.fences);
	}
	{ //write to command buffer
		//writing 2, one for each swapchain image.
		for (uint32_t buffer_index = 0; buffer_index < vkstate.cmd_allocate_info->commandBufferCount; buffer_index++) {
			{ //command begin info
				vkstate.cmd_begin_info = calloc(sizeof(VkCommandBufferBeginInfo),1);
				vkstate.cmd_begin_info->sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			}
			// vkResetCommandBuffer(vkstate.cmd_buffers[0], 0);

			vkBeginCommandBuffer(vkstate.cmd_buffers[buffer_index], vkstate.cmd_begin_info);
			{ //render pass begin info

				vkstate.render_pass_begin_info = calloc(sizeof(VkRenderPassBeginInfo),1);
				vkstate.render_pass_begin_info->sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				vkstate.render_pass_begin_info->renderPass = *vkstate.render_pass;
				vkstate.render_pass_begin_info->framebuffer = vkstate.framebuffers[buffer_index];
				vkstate.render_pass_begin_info->renderArea.extent = vkstate.swapchain_info->imageExtent;
				vkstate.render_pass_begin_info->renderArea.offset = (VkOffset2D){0,0};
				VkClearValue *clear_color = malloc(sizeof(VkClearValue));
				*clear_color = (VkClearValue){.color={{0.0f,0.0f,0.0f,1.0f}}};
				vkstate.render_pass_begin_info->clearValueCount = 1;
				vkstate.render_pass_begin_info->pClearValues = clear_color;
			}
			vkCmdBeginRenderPass(vkstate.cmd_buffers[buffer_index],vkstate.render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(vkstate.cmd_buffers[buffer_index], VK_PIPELINE_BIND_POINT_GRAPHICS, vkstate.graphics_pipeline);
			vkCmdDraw(vkstate.cmd_buffers[buffer_index], 3, 1, 0, 0);
			vkCmdEndRenderPass(vkstate.cmd_buffers[buffer_index]);
			vkEndCommandBuffer(vkstate.cmd_buffers[buffer_index]);
		}
	}

}
void vulkan_update(int dt) {
	vkWaitForFences(vkstate.log_dev, 1, &vkstate.fences[0], VK_TRUE, UINT64_MAX);
	vkResetFences(vkstate.log_dev,1,&vkstate.fences[0]);
	uint32_t image_index;
	vkAcquireNextImageKHR(vkstate.log_dev, *vkstate.swapchain, UINT64_MAX, vkstate.semaphores[0], VK_NULL_HANDLE, &image_index);
	// VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
	uint32_t flag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	vkQueueSubmit(*vkstate.device_queue, 1, &(VkSubmitInfo) {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = NULL,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vkstate.semaphores[0],
		.pWaitDstStageMask = &flag,
		.commandBufferCount = 1,
		.pCommandBuffers = &vkstate.cmd_buffers[image_index],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &vkstate.semaphores[1],
	}, vkstate.fences[0]);
	VkPresentInfoKHR presentInfo = {0};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &vkstate.semaphores[1];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = vkstate.swapchain;
	presentInfo.pImageIndices = &image_index;
	vkQueuePresentKHR(*vkstate.device_queue, &presentInfo);
}