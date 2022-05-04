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
I grind away at these lines
like tired men in coal mines
except instead of using a bird for danger to know
I go to stack overflow.
*/


typedef struct {
	// VkDebugReportCallbackEXT *debug_callback;
	VkInstanceCreateInfo *instance_info;
	VkInstance instance;

	VkSurfaceKHR surface;

	// VkExtent2D extent;

	VkPhysicalDevice phys_dev;
	VkDeviceCreateInfo *device_create_info;
	VkDevice log_dev;
	uint32_t queue_family_index;
	VkQueue *device_queue;

	VkCommandPoolCreateInfo *command_pool_create_info;
	VkCommandPool command_pool;

	VkRenderPassCreateInfo *render_pass_info;
	VkRenderPass render_pass;

	VkSwapchainCreateInfoKHR *swapchain_info;
	VkSwapchainKHR swapchain;
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

	VkPipeline graphics_pipeline;
	VkSemaphoreCreateInfo *semaphore_create_info;
	VkSemaphore *semaphores;
	VkFenceCreateInfo *fence_create_info;
	VkFence *fences;
} vk_state;
#define VERTEX_SHADER_PATH "./vert.spv"
#define FRAGMENT_SHADER_PATH "./frag.spv"
// #define USE_VALIDATION_LAYERS
vk_state vkstate;
//future speaking: THIS is what I decide to make it's own function?!
//Verify that, given an extension list and a requested extension, that the requested extension is in the extension list.
int has(VkExtensionProperties *properties, uint32_t property_count, char* requested_extension) {
	for (uint32_t i = 0; i < property_count; i++) {
		if (strcmp(properties[i].extensionName, requested_extension) == 0) {
			return 1;
		}
	}
	return 0;
}

void vulkan_init(void) {
	// SDL_Vulkan_GetDrawableSize(window, &vkstate.extent.width, &vkstate.extent.height)
	{ // create instance
		{ //instance info (Add it to vkstate)
			VkInstanceCreateInfo *instance_info = calloc(1,sizeof(VkInstanceCreateInfo));
			{ //application info
				VkApplicationInfo *appinfo = calloc(1,sizeof(VkApplicationInfo));
				appinfo->sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
				appinfo->pApplicationName = "Hi world.";
				appinfo->applicationVersion = 040;
				appinfo->apiVersion = VK_MAKE_VERSION(1, 2, 0);
				instance_info->pApplicationInfo = appinfo;
			}
			instance_info->sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			{ // Getting and verifying the instance extensions
				//compile time variables to configure:
				char *req_exts[] = {"VK_KHR_surface"};
				int enable_all_extensions = 0;
				//The rest is calcualation.

				uint32_t enabled_extension_count;
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

				char **requested_extensions;
				{ //prepare requested_extensions and set enabled_extension_count
					if (enable_all_extensions) {
						requested_extensions = malloc(sizeof(char*)*property_count);
						enabled_extension_count = property_count;
						{ //fill the pointer with all extensions
							for (uint32_t i = 0; i < enabled_extension_count; i++) {
								requested_extensions[i] = malloc(strlen(props[i].extensionName));
								memcpy(props[i].extensionName,requested_extensions[i], strlen(props[i].extensionName));
							}
						}
					} else {
						//sizeof(req_exts) works becaues it's an array, not a char*
						requested_extensions = malloc(sizeof(req_exts));
						enabled_extension_count = sizeof(req_exts)/sizeof(char*);
						//future: Why didn't I just use memcpy for this? Why does this even exist?
						{ //fill the pointer with requested extensions
							for (uint32_t i = 0; i < enabled_extension_count; i++) {
								requested_extensions[i] = malloc(strlen(req_exts[i]) + 1);
								memcpy(requested_extensions[i],req_exts[i],strlen(req_exts[i]) + 1);
								// printf("Copied %s into str %s\n",req_exts[i])
							}
						}
						{ //detect missing extensions
							int crash = 0;
							for (unsigned i = 0; i < enabled_extension_count; i++) {
								if(!has(props,property_count,requested_extensions[i])) {
									crash = 1;
									LOG_ERROR("Error: Missing extension %s\n",requested_extensions[i]);
								}
							}
							if (crash) {
								exit(-1);
							}
						}
					}
					{ //add SDL extensions
						unsigned int pcount;
						char ** pnames;
						if (SDL_Vulkan_GetInstanceExtensions(window, &pcount,NULL) == SDL_FALSE) {
							LOG_ERROR("Error, could not get SDL Vulkan extensions\n");
							exit(-1);
						}
						pnames = malloc(sizeof(char*)*pcount);
						SDL_Vulkan_GetInstanceExtensions(window,&pcount,(const char **)pnames);
						requested_extensions = realloc(requested_extensions,sizeof(char*)*(enabled_extension_count + pcount));
						for (uint32_t i = enabled_extension_count; i < enabled_extension_count+pcount; i++) {
							requested_extensions[i] = malloc(strlen(pnames[i-enabled_extension_count]) + 1);
							memcpy(requested_extensions[i], pnames[i-enabled_extension_count], strlen(pnames[i-enabled_extension_count]) + 1);
						}
						enabled_extension_count += pcount;
					}
				}
				instance_info->ppEnabledExtensionNames = (const char **)requested_extensions;
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
	{ //create surface
		/*blah blah*/
		if (SDL_Vulkan_CreateSurface(window,vkstate.instance,&vkstate.surface) == SDL_FALSE) {
			LOG_ERROR("Error, can't create surface, SDL_VulkanCreateSurface failed.\n");
			exit(-1);
		}
	}
	{ //create a logical device 
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
				int found_device = 0;
				for (uint32_t i = 0; i < dev_count; i++) {
					VkPhysicalDeviceProperties props;
					vkGetPhysicalDeviceProperties(devices[i], &props);
					uint32_t queuefamilycount = 0;
					vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queuefamilycount, NULL);
					// VkBool32 surface_supported = VK_FALSE;
					// if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
					// printf("queue family count %d\n",queuefamilycount);
					VkBool32 surface_supported = 0;
					for (uint32_t j = 0; j < queuefamilycount; j++) {
						vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], j, vkstate.surface, &surface_supported);
					}

					// printf("trying dev %d\n",i);
					// printf("surface supported? %d\n", surface_supported);
					// printf("gpu? %d\n", props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU );

					if (surface_supported) {
						// VkSurfaceCapabilitiesKHR surface_capabilities;
						// vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[i], vkstate.surface, &surface_capabilities);
						found_device = (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
						if (found_device) {
							vkstate.phys_dev = devices[i];
							break;
						}
					}
					// }
				}
				if (!found_device) {
					LOG_ERROR("My shitty algorithm found no suitable devices lul\n");
					exit(-1);
				}
			}
		}
			{ // Device create info

				VkDeviceQueueCreateInfo *device_queue_create_info;
				{ //Device queue create info
					{ //Find suitable family
						uint32_t property_count;
						VkQueueFamilyProperties *props;
						vkGetPhysicalDeviceQueueFamilyProperties(vkstate.phys_dev, &property_count, NULL);
						props = malloc(sizeof(VkQueueFamilyProperties)*property_count);
						vkGetPhysicalDeviceQueueFamilyProperties(vkstate.phys_dev, &property_count, props);
						int found_one = 0;
						for (uint32_t i = 0; i < property_count; i++) {
							if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
								vkstate.queue_family_index = i;
								found_one = 1;
								break;
							}
						}
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
				char **device_extensions;
				uint32_t enabled_extension_count;
				{ //get & verify device extensions
					char *req_exts[] = {"VK_KHR_swapchain"};
					int enable_all_extensions = 0;
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
					if (enable_all_extensions) {
						enabled_extension_count = property_count;
						device_extensions = malloc(sizeof(char*)*property_count);
						for (uint32_t i = 0; i < property_count; i++) {
							device_extensions[i] = malloc(strlen(properties[property_count].extensionName) + 1);
							memcpy(device_extensions[i], properties[property_count].extensionName, strlen(properties[property_count].extensionName) + 1);
						}
					} else {
						device_extensions = req_exts;
						int crash = 0;
						enabled_extension_count = sizeof(req_exts)/sizeof(char*);
						for (long unsigned i = 0; i < sizeof(req_exts)/sizeof(char*); i++) {
							if (!has(properties,property_count,req_exts[i])) {
								LOG_ERROR("Error: Missing extension %s\n",req_exts[i]);
								crash = 1;
							}
						}
						if(crash) {
							exit(-1);
						}
					}
				}


				VkDeviceCreateInfo *device_create_info = calloc(sizeof(VkDeviceCreateInfo),1);
				device_create_info->sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
				device_create_info->queueCreateInfoCount = 1;
				device_create_info->pQueueCreateInfos = device_queue_create_info;
				device_create_info->ppEnabledExtensionNames = (const char**) device_extensions;
				device_create_info->enabledExtensionCount = enabled_extension_count;
				device_create_info->pEnabledFeatures = NULL;

				// device_create_info->ppEnabledLayerNames = (const char * const[]){"VK_LAYER_KHRONOS_validation"};
				// device_create_info->enabledLayerCount = 1;
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

			uint32_t formats_len = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(vkstate.phys_dev, vkstate.surface, &formats_len, NULL);
			VkSurfaceFormatKHR *formats = malloc(sizeof(VkSurfaceFormatKHR)*formats_len);	
			vkGetPhysicalDeviceSurfaceFormatsKHR(vkstate.phys_dev, vkstate.surface, &formats_len, formats);
			if (formats_len == 0) {
				LOG_ERROR("No image formats supported for this device.\n");
				exit(-1);
			}
			// for (uint32_t i = 0; i < formats_len; i++) {
			// 	printf("Supported image format %d\n", formats[i].format);
			// }
			vkstate.swapchain_info->imageFormat = formats[1].format;
			vkstate.swapchain_info->imageColorSpace = formats[1].colorSpace;
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
		}
		vkCreateSwapchainKHR(vkstate.log_dev, vkstate.swapchain_info,NULL, &vkstate.swapchain);
		{ //get swapchain images
			uint32_t imagecount = 2;
			vkstate.swapchain_images = malloc(sizeof(VkImage)*2);
			vkGetSwapchainImagesKHR(vkstate.log_dev, vkstate.swapchain, &imagecount, vkstate.swapchain_images);
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
				attachments->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				attachments->finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			}
			VkSubpassDescription * subpasses;
			{ //subpass description

				VkAttachmentReference * ref = malloc(sizeof(VkAttachmentReference));
				ref->layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				ref->attachment = 0;
				subpasses = calloc(sizeof(VkSubpassDescription),1);
				subpasses->pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
				subpasses->colorAttachmentCount = 1;
				subpasses->pColorAttachments = ref;
			}
			VkSubpassDependency * dependency;
			{ //subpass dependency
				dependency = calloc(sizeof(VkSubpassDependency),1);
				dependency->srcSubpass = VK_SUBPASS_EXTERNAL;
				dependency->dstSubpass = 0;
				dependency->srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependency->dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependency->srcAccessMask = 0;
				dependency->dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			}
			VkRenderPassCreateInfo *render_pass_info = calloc(sizeof(VkRenderPassCreateInfo),1);
			render_pass_info->sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			// render_pass_info->pcolor
			render_pass_info->attachmentCount = 1;
			render_pass_info->pAttachments = attachments;
			render_pass_info->subpassCount = 1;
			render_pass_info->pSubpasses = subpasses;
			render_pass_info->dependencyCount = 1;
			render_pass_info->pDependencies = dependency;
			vkstate.render_pass_info = render_pass_info;
			// render_pass_info->
		}
		VkRenderPass render_pass;
		vkCreateRenderPass(vkstate.log_dev,vkstate.render_pass_info,NULL,&render_pass);
		vkstate.render_pass = render_pass;
	}
	{ //create image views
		{ //image view infos}
			vkstate.imageview_infos = calloc(sizeof(VkImageViewCreateInfo)*2,1);
			{ //first
				{ //image view create info
					vkstate.imageview_infos[0].sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
					vkstate.imageview_infos[0].image = vkstate.swapchain_images[0];
					vkstate.imageview_infos[0].viewType = VK_IMAGE_VIEW_TYPE_2D;
					vkstate.imageview_infos[0].format = VK_FORMAT_B8G8R8A8_SRGB;
					vkstate.imageview_infos[0].components = (VkComponentMapping){
						.r=VK_COMPONENT_SWIZZLE_IDENTITY,
						.g=VK_COMPONENT_SWIZZLE_IDENTITY,
						.b=VK_COMPONENT_SWIZZLE_IDENTITY,
						.a=VK_COMPONENT_SWIZZLE_IDENTITY};
					vkstate.imageview_infos[0].subresourceRange = (VkImageSubresourceRange) {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.baseMipLevel = 0,
						.levelCount = 1,
						.baseArrayLayer = 0,
						.layerCount = 1,
					};
				}
				vkstate.imageviews = malloc(sizeof(VkImageView)*2);
			}
			{ //second
				{ //image view create info
					vkstate.imageview_infos[1].sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
					vkstate.imageview_infos[1].image = vkstate.swapchain_images[1];
					vkstate.imageview_infos[1].viewType = VK_IMAGE_VIEW_TYPE_2D;
					vkstate.imageview_infos[1].format = VK_FORMAT_B8G8R8A8_SRGB;
					vkstate.imageview_infos[1].components = (VkComponentMapping){
						.r=VK_COMPONENT_SWIZZLE_IDENTITY,
						.g=VK_COMPONENT_SWIZZLE_IDENTITY,
						.b=VK_COMPONENT_SWIZZLE_IDENTITY,
						.a=VK_COMPONENT_SWIZZLE_IDENTITY};
					vkstate.imageview_infos[1].subresourceRange = (VkImageSubresourceRange) {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.baseMipLevel = 0,
						.levelCount = 1,
						.baseArrayLayer = 0,
						.layerCount = 1,
					};
				}
			}
		}
		vkCreateImageView(vkstate.log_dev, &vkstate.imageview_infos[0], NULL, &(vkstate.imageviews[0]));
		vkCreateImageView(vkstate.log_dev, &vkstate.imageview_infos[1], NULL, &(vkstate.imageviews[1]));
	}
	{ //create frame buffers
		{ //framebuffer create info
			vkstate.framebuffer_infos = calloc(sizeof(VkFramebufferCreateInfo)*2,1);
			{ //first
				vkstate.framebuffer_infos[0].sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				vkstate.framebuffer_infos[0].renderPass = vkstate.render_pass;
				vkstate.framebuffer_infos[0].attachmentCount = 1;
				vkstate.framebuffer_infos[0].pAttachments = &(vkstate.imageviews[0]);
				vkstate.framebuffer_infos[0].width = vkstate.swapchain_info->imageExtent.width;
				vkstate.framebuffer_infos[0].height = vkstate.swapchain_info->imageExtent.height;
				vkstate.framebuffer_infos[0].layers = 1;
			}
			{ //second
				vkstate.framebuffer_infos[1].sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				vkstate.framebuffer_infos[1].renderPass = vkstate.render_pass;
				vkstate.framebuffer_infos[1].attachmentCount = 1;
				vkstate.framebuffer_infos[1].pAttachments = &(vkstate.imageviews[1]);
				vkstate.framebuffer_infos[1].width = vkstate.swapchain_info->imageExtent.width;
				vkstate.framebuffer_infos[1].height = vkstate.swapchain_info->imageExtent.height;
				vkstate.framebuffer_infos[1].layers = 1;
			}
			vkstate.framebuffers = malloc(sizeof(VkFramebuffer)*2);
		}
		vkCreateFramebuffer(vkstate.log_dev, &(vkstate.framebuffer_infos[0]), NULL, &(vkstate.framebuffers[0]));
		vkCreateFramebuffer(vkstate.log_dev, &(vkstate.framebuffer_infos[1]), NULL, &(vkstate.framebuffers[1]));
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
	{ //allocate command buffer
		{ //infos
			vkstate.cmd_allocate_info = calloc(sizeof(VkCommandBufferAllocateInfo),1);
			vkstate.cmd_allocate_info->sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			vkstate.cmd_allocate_info->commandPool = vkstate.command_pool;
			vkstate.cmd_allocate_info->level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			vkstate.cmd_allocate_info->commandBufferCount = 1;
		}
		vkstate.cmd_buffers = malloc(sizeof(VkCommandBuffer)*vkstate.cmd_allocate_info->commandBufferCount);
		vkAllocateCommandBuffers(vkstate.log_dev, vkstate.cmd_allocate_info, vkstate.cmd_buffers);
	}
	{ //create graphics pipeline
		//shader stage
		{

			//vertex shader
			{
				FILE * vertex_shader=  fopen(VERTEX_SHADER_PATH, "rb");
				fseek(vertex_shader, 0, SEEK_END);
				long int size = ftell(vertex_shader);
				fseek(vertex_shader, 0, SEEK_SET);
				uint32_t * vertex_code = malloc(sizeof(uint32_t)*(size/4));
				fread(vertex_code, sizeof(uint32_t), size/4, vertex_shader);
				fclose(vertex_shader);


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
				FILE * fragment_shader = fopen(FRAGMENT_SHADER_PATH, "rb");
				fseek(fragment_shader, 0, SEEK_END);
				long int size = ftell(fragment_shader);
				fseek(fragment_shader, 0, SEEK_SET);
				uint32_t * fragment_code = malloc(sizeof(uint32_t)*(size/4));
				fread(fragment_code, sizeof(uint32_t),  size/4, fragment_shader);
				fclose(fragment_shader);


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

			// vkstate.pipeline_shader_stage_create_info->
		}
		VkPipelineShaderStageCreateInfo *stages = malloc(sizeof(VkPipelineShaderStageCreateInfo)*2);
		stages[0] = *vkstate.vertex_pipeline_shader_stage_create_info;
		stages[1] = *vkstate.fragment_pipeline_shader_stage_create_info;

		//no pipeline cache = VK_NULL_HANDLE 
		vkstate.graphics_pipeline_create_info = calloc(sizeof(VkGraphicsPipelineCreateInfo),1);
		vkstate.graphics_pipeline_create_info->sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		vkstate.graphics_pipeline_create_info->stageCount = 2;
		{ //create vertex input state
			vkstate.pipeline_vertex_input_state_create_info = calloc(sizeof(VkPipelineVertexInputStateCreateInfo),1);
			vkstate.pipeline_vertex_input_state_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vkstate.pipeline_vertex_input_state_create_info->vertexBindingDescriptionCount = 0;
			vkstate.pipeline_vertex_input_state_create_info->vertexAttributeDescriptionCount = 0;
		}
		{ //create viewport state
			vkstate.pipeline_viewport_state_create_info = calloc(sizeof(VkPipelineViewportStateCreateInfo),1);
			vkstate.pipeline_viewport_state_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			vkstate.pipeline_viewport_state_create_info->pNext = NULL;
			// vkstate.pipeline_viewport_state_create_info->flags = 
		}
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



		// vkstate.graphics_pipeline_create_info->pViewportState = SDL_RenderGetViewport(SDL_Renderer *renderer, SDL_Rect *rect)
		//all this commented out for data transfer with shaders to be added later!

			// VkVertexInputBindingDescription *binding_descs = calloc(sizeof(VkVertexInputBindingDescription),2);
			// //pos
			// binding_descs[0].binding = 0;
			// binding_descs[0].stride = sizeof(float);
			// binding_descs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			// //color
			// binding_descs[0].binding = 1;
			// binding_descs[0].stride = sizeof(uint32_t);
			// binding_descs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	
			// VkVertexInputAttributeDescription *attr_descs = calloc(sizeof(VkVertexInputAttributeDescription),2);
			// attr_descs[0].location = 0;
			// attr_descs[0].binding = 0;
			// attr_descs[0].format = VK_FORMAT_R16_SFLOAT;
			// attr_descs[0].offset = 0;
	
			// attr_descs[1].location = 1;
			// attr_descs[1].binding = 1;
			// attr_descs[1].format = VK_FORMAT_R32_UINT;
			// attr_descs[1].offset = 0;


			// vkstate.pipeline_vertex_input_state_create_info->pVertexBindingDescriptions = binding_descs;
			// vkstate.pipeline_vertex_input_state_create_info->pVertexAttributeDescriptions = attr_descs;



		// vkstate.vertex_pipeline_vertex_input_state_create_info
		
		//vertex input state, since we ARE inputting vertexes
		/*
			EDIT:
			Woah, it's been a while! I had this strange idea that at this point in the project, I had no choice but to sende data to the shaders. Nope!
			A simple hello triangle can do without that little feature, which would be far easier added later than to handle being there now.
			I'll try to delete those bindings. Ha!


		//okay, we gotta decide on bindings and attributes. 
		//Now is probably the time in development to decide on, in this little demo, how I want to send data between the shaders and the application.
		//I think a vertex buffer with the position data would be simple enough, and then later one with the color data too.
		//Gonna do some research to make sure I know vertexbinding and virtexattributes correctly, but if I /DO/ understand them, vertexbinding is just for dealing with memory on the drivers side, and describing that memory,
		//and vertexattribute is the shader-facing part, and where you decide what "location" the binding refers to in the shader.
		
		// Provided by VK_VERSION_1_0
			typedef struct VkPipelineVertexInputStateCreateInfo {
			    VkStructureType                             sType;
			    const void*                                 pNext;
			    VkPipelineVertexInputStateCreateFlags       flags;
			    uint32_t                                    vertexBindingDescriptionCount;
			    const VkVertexInputBindingDescription*      pVertexBindingDescriptions;
			    uint32_t                                    vertexAttributeDescriptionCount;
			    const VkVertexInputAttributeDescription*    pVertexAttributeDescriptions;
			} VkPipelineVertexInputStateCreateInfo;



			// Provided by VK_VERSION_1_0
			typedef struct VkVertexInputBindingDescription {
			    uint32_t             binding;
			    uint32_t             stride;
			    VkVertexInputRate    inputRate;
			} VkVertexInputBindingDescription;
			and
			// Provided by VK_VERSION_1_0
			typedef struct VkVertexInputAttributeDescription {
			    uint32_t    location;
			    uint32_t    binding;
			    VkFormat    format;
			    uint32_t    offset;
			} VkVertexInputAttributeDescription;


			When you want to bind the actual data, that's vkCmdBindVertexBuffers(), with VkBuffers already created with VkCreateBuffers.

			// Provided by VK_VERSION_1_0
			void vkCmdBindVertexBuffers(
			    VkCommandBuffer                             commandBuffer,
			    uint32_t                                    firstBinding,
			    uint32_t                                    bindingCount,
			    const VkBuffer*                             pBuffers,
			    const VkDeviceSize*                         pOffsets);
			(Firstbinding is the binding id, and then it loops through buffers up to bindingcount, with the binding updated to start at offset supplied by poffsets[i] from the start of the pbuffer[i])
			Plan is to make a triangle buffer, initialize it in vkstate, create attribute and vertex input buinding for it, and when the command buffer is in a recording state, use the VkCmdBindVertexBuffers() to bind them.
		*/
		VkPipelineLayout layout;
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
			// vkstate.pipeline_multisample_state_create_info->
		}
			{ //input assembly state
			vkstate.pipeline_input_assembly_state_create_info = calloc(sizeof(VkPipelineInputAssemblyStateCreateInfo),1);
			vkstate.pipeline_input_assembly_state_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			//primitives, baby
			vkstate.pipeline_input_assembly_state_create_info->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			vkstate.pipeline_input_assembly_state_create_info->primitiveRestartEnable = VK_FALSE;
			// vkstate.pipeline_input_assembly_state_create_info->
			// vkstate.pipeline_input_assembly_state_create_info->
		}



		{ //Color blend state
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
			// vkstate.pipeline_color_blend_state_create_info->


		}


		{ //create layout
			vkstate.pipeline_layout_create_info = calloc(sizeof(VkPipelineLayoutCreateInfo),1);
			vkstate.pipeline_layout_create_info->sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			vkstate.pipeline_layout_create_info->pNext = NULL;
			//not # layouts, # descriptor sets included in the layout
			//since I'm not using any descriptors right now, 0
			vkstate.pipeline_layout_create_info->setLayoutCount = 0;
			vkstate.pipeline_layout_create_info->pSetLayouts = NULL;
			vkstate.pipeline_layout_create_info->pushConstantRangeCount = 0;
			vkstate.pipeline_layout_create_info->pPushConstantRanges = NULL;
			vkCreatePipelineLayout(vkstate.log_dev, vkstate.pipeline_layout_create_info, NULL, &layout);
		}


		vkstate.graphics_pipeline_create_info->pStages = stages;
		vkstate.graphics_pipeline_create_info->pVertexInputState = vkstate.pipeline_vertex_input_state_create_info;
		vkstate.graphics_pipeline_create_info->pViewportState = vkstate.pipeline_viewport_state_create_info;
		vkstate.graphics_pipeline_create_info->pRasterizationState = vkstate.pipeline_rasterization_state_create_info;
		vkstate.graphics_pipeline_create_info->pMultisampleState = vkstate.pipeline_multisample_state_create_info;
		vkstate.graphics_pipeline_create_info->pInputAssemblyState = vkstate.pipeline_input_assembly_state_create_info;
		vkstate.graphics_pipeline_create_info->pColorBlendState = vkstate.pipeline_color_blend_state_create_info;
		vkstate.graphics_pipeline_create_info->renderPass = vkstate.render_pass;
		vkstate.graphics_pipeline_create_info->layout = layout;
		
		// vkstate.graphics_pipeline_create_info->
		// vkstate.graphics_pipeline_create_info->
		// vkstate.graphics_pipeline_create_info->
		// vkstate.graphics_pipeline_create_info->
		// vkstate.graphics_pipeline_create_info->


				// printf("??\n");
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
		vkstate.fences = calloc(sizeof(VkFence),1);
		vkCreateFence(vkstate.log_dev, vkstate.fence_create_info, NULL, vkstate.fences);
	}

}
void vulkan_update(int dt) {
	vkWaitForFences(vkstate.log_dev, 1, &vkstate.fences[0], VK_TRUE, UINT64_MAX);
	vkResetFences(vkstate.log_dev,1,&vkstate.fences[0]);
	uint32_t imageIndex;
	vkAcquireNextImageKHR(vkstate.log_dev, vkstate.swapchain, UINT64_MAX, vkstate.semaphores[0], VK_NULL_HANDLE, &imageIndex);

	{ //write to command buffer
		{ //command begin info
			vkstate.cmd_begin_info = calloc(sizeof(VkCommandBufferBeginInfo),1);
			vkstate.cmd_begin_info->sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		}
		vkResetCommandBuffer(vkstate.cmd_buffers[0], 0);

		vkBeginCommandBuffer(vkstate.cmd_buffers[0], vkstate.cmd_begin_info);
		{ //render pass begin info

			vkstate.render_pass_begin_info = calloc(sizeof(VkRenderPassBeginInfo),1);
			vkstate.render_pass_begin_info->sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			vkstate.render_pass_begin_info->renderPass = vkstate.render_pass;
			vkstate.render_pass_begin_info->framebuffer = vkstate.framebuffers[imageIndex];
			vkstate.render_pass_begin_info->renderArea.extent = vkstate.swapchain_info->imageExtent;
			vkstate.render_pass_begin_info->renderArea.offset = (VkOffset2D){0,0};
			VkClearValue *clear_color = malloc(sizeof(VkClearValue));
			*clear_color = (VkClearValue){.color={{0.0f,0.0f,0.0f,1.0f}}};
			vkstate.render_pass_begin_info->clearValueCount = 1;
			vkstate.render_pass_begin_info->pClearValues = clear_color;
		}
		vkCmdBeginRenderPass(vkstate.cmd_buffers[0],vkstate.render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(vkstate.cmd_buffers[0], VK_PIPELINE_BIND_POINT_GRAPHICS, vkstate.graphics_pipeline);
		vkCmdDraw(vkstate.cmd_buffers[0], 3, 1, 0, 0);
		vkCmdEndRenderPass(vkstate.cmd_buffers[0]);
		vkEndCommandBuffer(vkstate.cmd_buffers[0]);
	}
	// VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
	uint32_t flag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		vkQueueSubmit(*vkstate.device_queue, 1, &(VkSubmitInfo){
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = NULL,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &vkstate.semaphores[0],
			.pWaitDstStageMask = &flag,
			.commandBufferCount = 1,
			.pCommandBuffers = vkstate.cmd_buffers,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &vkstate.semaphores[1],
		}, vkstate.fences[0]);
		VkPresentInfoKHR presentInfo = {0};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &vkstate.semaphores[1];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &vkstate.swapchain;
		presentInfo.pImageIndices = &imageIndex;
		vkQueuePresentKHR(*vkstate.device_queue, &presentInfo);


	// if (!vkstate.semaphore_start) {
	// 	vkstate.semaphore_start = 1;
	// } else if (vkstate.semaphore_start == 1) {
	// 	vkQueueSubmit(*vkstate.device_queue, 1, &(VkSubmitInfo){
	// 		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	// 		.pNext = NULL,
	// 		.waitSemaphoreCount = 1,
	// 		.pWaitSemaphores = &vkstate.semaphores[0],
	// 		.pWaitDstStageMask = &flag,
	// 		.commandBufferCount = 1,
	// 		.pCommandBuffers = vkstate.cmd_buffers,
	// 		.signalSemaphoreCount = 1,
	// 		.pSignalSemaphores = &vkstate.semaphores[1],
	// 	}, VK_NULL_HANDLE);
	// 	vkstate.semaphore_start = 2;
	// } else {
	// 	return;
	// 	vkQueueSubmit(*vkstate.device_queue, 1, &(VkSubmitInfo){
	// 		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	// 		.pNext = NULL,
	// 		.waitSemaphoreCount = 1,
	// 		.pWaitSemaphores = &vkstate.semaphores[1],
	// 		.pWaitDstStageMask = &flag,
	// 		.commandBufferCount = 1,
	// 		.pCommandBuffers = vkstate.cmd_buffers,
	// 		.signalSemaphoreCount = 1,
	// 		.pSignalSemaphores = &vkstate.semaphores[0],
	// 	}, VK_NULL_HANDLE);
	// 	vkstate.semaphore_start = 1;
	// }
}