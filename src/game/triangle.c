#include "sdl/modules/vulkan/vkstate.h"
#include <cglm/cglm.h>
#include <stdlib.h>
#include <string.h>
#include "log/log.h"
vec2 positions[3] = (vec2[3]) {
	{0,-1}, {1,1}, {-1,1},
};
vec3 colors[3] = (vec3[3]) {
	{1,0,0},{0,1,0},{0,0,1},
};

/*
this is a nearly failed attempt to isolate the triangle-drawing portion of the code(not including shaders)
issue is: buffers. oh buffers. they need to be bound to the command buffer upon the command buffers creation, but the command buffer needs to be recreated every time
the swapchain is invalidated. Which is //all the time//. So the solution I came up with is using num_buffers, it just iterates through them both in the initialization of this
and in the vulkan initialization, if they're not NULL (meaning if nobody else has any buffers to add). That's the crux; we need vulkan to be initialized in order to do things, 
but vulkan needs us to have already done things in order to finish setting things up, and to recreate things.
*/



int get_memory_type_index(VkBuffer buffer) {
	VkMemoryRequirements mem_req;
	VkPhysicalDeviceMemoryProperties mem_prop;
	vkGetBufferMemoryRequirements(vkstate.log_dev, buffer, &mem_req);
	vkGetPhysicalDeviceMemoryProperties(vkstate.phys_dev, &mem_prop);
	/*
		TODO: all memory here is host-visible. It would cause problems to make any large pieces of memory host-visible. This is where that's decided.
		It would be a problem to allocate all memory this way, and I should implement device-visible memory with command queue calls where neccessary to transfer
		to the gpu.
	*/
	for (uint32_t i = 0; i < mem_prop.memoryTypeCount; i++) {
		if ((mem_req.memoryTypeBits & (1 << i)) && (mem_prop.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
			return i;
			break;
		} 
	}
	return -1;
}
static int *buffer_indexes;

void write_command_buffers(vk_state *vkstate) {
	vkstate->render_pass_begin_infos = calloc(sizeof(VkRenderPassBeginInfo),vkstate->cmd_allocate_info->commandBufferCount);
	for (uint32_t buffer_index = 0; buffer_index < vkstate->cmd_allocate_info->commandBufferCount; buffer_index++) {
		{ //command begin info
			vkstate->cmd_begin_info = calloc(sizeof(VkCommandBufferBeginInfo),1);
			vkstate->cmd_begin_info->sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		}
		vkBeginCommandBuffer(vkstate->cmd_buffers[buffer_index], vkstate->cmd_begin_info);
		{ //render pass begin info

			vkstate->render_pass_begin_infos[buffer_index].sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			vkstate->render_pass_begin_infos[buffer_index].renderPass = *vkstate->render_pass;
			vkstate->render_pass_begin_infos[buffer_index].framebuffer = vkstate->framebuffers[buffer_index];
			vkstate->render_pass_begin_infos[buffer_index].renderArea.extent = vkstate->swapchain_info->imageExtent;
			vkstate->render_pass_begin_infos[buffer_index].renderArea.offset = (VkOffset2D){0,0};
			vkstate->render_pass_begin_infos[buffer_index].clearValueCount = 1;
			vkstate->render_pass_begin_infos[buffer_index].pClearValues = &(VkClearValue){.color={{0.6f,0.6f,0.6f,1.0f}}};
		}
		vkCmdBeginRenderPass(vkstate->cmd_buffers[buffer_index],&vkstate->render_pass_begin_infos[buffer_index], VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(vkstate->cmd_buffers[buffer_index], VK_PIPELINE_BIND_POINT_GRAPHICS, vkstate->graphics_pipeline);

		VkDeviceSize offsets[] = {0,0};
		vkCmdBindVertexBuffers(vkstate->cmd_buffers[buffer_index], 0, vkstate->num_buffers, vkstate->buffers, offsets);
		//WARNING currently, positions is an array, not a pointer. This means sizeof(positions) gives the size of the /whole thing/, not just the size of a pointer.
		//replace this if/when dynamic memroy allocatino is being used!
		// vkCmdDraw(vkstate->cmd_buffers[buffer_index], sizeof(positions)/sizeof(positions[0]), 1, 0, 0);
		vkCmdDraw(vkstate->cmd_buffers[buffer_index], 3, 1, 0, 0);
		vkCmdEndRenderPass(vkstate->cmd_buffers[buffer_index]);
		vkEndCommandBuffer(vkstate->cmd_buffers[buffer_index]);
	}

}
void create_buffers() {
	buffer_indexes = malloc(sizeof(int)*2);
	buffer_indexes[0] = vkstate.num_buffers;
	buffer_indexes[1] = vkstate.num_buffers + 1;
	/* create the buffers */
	VkBufferCreateInfo *bufferinfo = calloc(sizeof(VkBufferCreateInfo),2);
	vkstate.buffers = calloc(sizeof(VkBuffer),2);
	bufferinfo[0] = (VkBufferCreateInfo) {	.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, 
											.size = sizeof(positions),
											.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
											.sharingMode = VK_SHARING_MODE_EXCLUSIVE	};
	bufferinfo[1] = bufferinfo[0];
	bufferinfo[1].size = sizeof(colors);

	VkResult result = vkCreateBuffer(vkstate.log_dev, &bufferinfo[0], NULL, &vkstate.buffers[0]);
	CHECK_ERROR(result, "Failed to create position vertex buffer. Erro num %d\n", result);
	result = vkCreateBuffer(vkstate.log_dev, &bufferinfo[1], NULL, &vkstate.buffers[1]);
	CHECK_ERROR(result, "Failed to create color vertex buffer. Erro num %d\n",result);
	vkstate.num_buffers += 2;
	/* Get some host-visible memory to easily assign to the buffers */

	vkstate.buffer_mem = realloc(vkstate.buffer_mem, sizeof(VkDeviceMemory *)*vkstate.num_buffers);
	vkstate.buffer_data	= realloc(vkstate.buffer_data, sizeof(void *)*vkstate.num_buffers);

	/* Deal with first buffer's memory */
	VkMemoryRequirements mem_req;
	vkGetBufferMemoryRequirements(vkstate.log_dev, vkstate.buffers[0], &mem_req);
	VkMemoryAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = get_memory_type_index(vkstate.buffers[0]);

	vkstate.buffer_mem[vkstate.num_buffers - 2] = malloc(sizeof(VkDeviceMemory));
	result = vkAllocateMemory(vkstate.log_dev, &alloc_info, NULL,&vkstate.buffer_mem[vkstate.num_buffers - 2]);
	CHECK_ERROR(result, "error allocating memory. error num %d\n",result);
	if (result != VK_SUCCESS) exit(-1);

	/* Deal with second buffer's memory */
	vkGetBufferMemoryRequirements(vkstate.log_dev, vkstate.buffers[1], &mem_req);
	memset(&alloc_info, 0, sizeof(VkMemoryAllocateInfo));
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = get_memory_type_index(vkstate.buffers[1]);

	vkstate.buffer_mem[vkstate.num_buffers - 1] = malloc(sizeof(VkDeviceMemory));
	result = vkAllocateMemory(vkstate.log_dev, &alloc_info, NULL,&vkstate.buffer_mem[vkstate.num_buffers - 1]);
	CHECK_ERROR(result, "error allocating memory. error num %d\n", result);
	if (result != VK_SUCCESS) exit(-1); 

	/* Bind it, map it, wrap it in a bow */
	vkBindBufferMemory(vkstate.log_dev, vkstate.buffers[0], vkstate.buffer_mem[vkstate.num_buffers - 2], 0);
	vkBindBufferMemory(vkstate.log_dev, vkstate.buffers[1], vkstate.buffer_mem[vkstate.num_buffers - 1], 0);
	vkMapMemory(vkstate.log_dev, vkstate.buffer_mem[vkstate.num_buffers - 2],  0, bufferinfo[0].size, 0, &vkstate.buffer_data[vkstate.num_buffers - 2]);
	vkMapMemory(vkstate.log_dev, vkstate.buffer_mem[vkstate.num_buffers - 1],  0, bufferinfo[1].size, 0, &vkstate.buffer_data[vkstate.num_buffers - 1]);
	memcpy(vkstate.buffer_data[vkstate.num_buffers - 2], positions, bufferinfo[0].size);
	memcpy(vkstate.buffer_data[vkstate.num_buffers - 1], colors, bufferinfo[1].size);
}
void triangle_init() {
	create_buffers();
	scribe_command_buffers(&vkstate);

}
static int counter = 0;
void triangle_update(int dt) {
	counter += dt;
	if (counter > 1000) {
		counter -= (counter/1000)*1000;
		float tmp[3] = {0};
		memcpy(tmp, colors[0], sizeof(float)*3);
		memcpy(&colors[0],&colors[1],sizeof(float)*3);
		memcpy(&colors[1],&colors[2],sizeof(float)*3);
		memcpy(&colors[2],&tmp,sizeof(float)*3);
 		// Similarly, positions can be changed this way
		// memcpy(&positions[0],(vec2[3]){{positions[0][0]*-1.01+0.01,positions[0][1]},{positions[1][0],positions[1][1]},{positions[2][0],positions[2][1]}},sizeof(vec2)*3);
		// memcpy(vkstate.buffer1_data, positions, sizeof(positions));
		memcpy(vkstate.buffer_data[buffer_indexes[1]], colors, sizeof(colors));
	}
}