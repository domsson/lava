#include <vulkan/vulkan.h>
#include <sys/stat.h>

typedef struct lv_config
{
	int validation;
	// func pointer to debug_callback_function for validation message handling
} lv_config_s;

typedef struct lv_name_set
{
	const char* const* names; // same as const char** ?
	uint32_t           count;
} lv_name_set_s;

typedef struct lv_queue
{
	VkQueue  queue;
	uint32_t index;
	float priority;
} lv_queue_s;

typedef struct lv_image_set
{
	VkImage  *images;
	uint32_t  count;
} lv_image_set_s;

typedef struct lv_image_set lv_image_set_s;

typedef enum lv_shader_type
{
	LV_NONE,	// type not specified or unknown
	LV_VERT,	// vertex
	LV_TESC,	// tessellation control
	LV_TESE,	// tessellation evaluation
	LV_GEOM,	// geometry
	LV_FRAG,	// fragment
	LV_COMP		// compute
} lv_shader_type_e;

typedef struct lv_shader
{
	uint32_t         *data;		// SPIR-V bytecode
	size_t            size;		// size of bytecode
	lv_shader_type_e  type;		// shader type (vertex, fragment, ...)
	VkShaderModule   *module;	// vulkan shader module TODO make this a pointer so we can check against NULL?
} lv_shader_s;

typedef struct lv_state
{
	lv_config_s      *config;
	VkInstance        vk_instance;
	VkPhysicalDevice  physical_device;
	VkDevice          logical_device;
	lv_queue_s       *graphics_queue;
	lv_queue_s       *present_queue;
	VkSurfaceKHR      surface;
} lv_state_s;

//
//
//

lv_state_s *lv_init(lv_config_s *cfg)
{
	lv_state_s *lv = malloc(sizeof(lv_state_s));
	// TODO copy this instead, so the caller can free their thingy if they want
	lv->config = cfg;

	return lv;
}

// const char* const* -> "a pointer to a constant pointer to a char constant"
int lv_instance_create(VkInstance *instance, lv_name_set_s *extensions, lv_name_set_s *layers)
{
	VkInstanceCreateInfo info = { 0 };
	info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	// Layers
	info.enabledLayerCount       = layers ? layers->count : 0;
	info.ppEnabledLayerNames     = layers ? layers->names : NULL;
	// Extensions
	info.enabledExtensionCount   = extensions ? extensions->count : 0;
	info.ppEnabledExtensionNames = extensions ? extensions->names : NULL;

	return (vkCreateInstance(&info, NULL, instance) == VK_SUCCESS);
}

int lv_print_extensions()
{
	uint32_t ext_count = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &ext_count, NULL);

	if (ext_count == 0)
	{
		return 0;
	}

	VkExtensionProperties *ext_props = malloc(sizeof(VkExtensionProperties) * ext_count);
	vkEnumerateInstanceExtensionProperties(NULL, &ext_count, ext_props);

	for (int i = 0; i < ext_count; ++i)
	{
		fprintf(stdout, "%*d: %s\n", 2, i+1, ext_props[i].extensionName);
	}
	
	free(ext_props);
	return ext_count;
}

int lv_instance_has_extension(const char *name)
{
	uint32_t ext_count = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &ext_count, NULL);

	if (ext_count == 0)
	{
		return 0;
	}

	VkExtensionProperties *ext_props = malloc(sizeof(VkExtensionProperties) * ext_count);
	vkEnumerateInstanceExtensionProperties(NULL, &ext_count, ext_props);

	int found = 0;
	for (int i = 0; i < ext_count; ++i)
	{
		if (strcmp(name, ext_props[i].extensionName) == 0)
		{
			found = 1;
			break;
		}
	}

	free(ext_props);
	return found;
}

int lv_print_layers()
{
	uint32_t layer_count = 0;
	vkEnumerateInstanceLayerProperties(&layer_count, NULL);

	if (layer_count == 0)
	{
		return 0;
	}

	VkLayerProperties* layer_props = malloc(sizeof(VkLayerProperties) * layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, layer_props);

	for (int i = 0; i < layer_count; ++i)
	{
		fprintf(stdout, "%*d: %s\n", 2, i+1, layer_props[i].layerName);
	}

	free(layer_props);
	return layer_count;
}

int lv_instance_has_layer(const char *name)
{
	uint32_t layer_count = 0;
	vkEnumerateInstanceLayerProperties(&layer_count, NULL);

	if (layer_count == 0)
	{
		return 0;
	}

	VkLayerProperties* layer_props = malloc(sizeof(VkLayerProperties) * layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, layer_props);

	int found = 0;
	for (int i = 0; i < layer_count; ++i)
	{
		if (strcmp(name, layer_props[i].layerName) == 0)
		{
			found = 1;
			break;
		}
	}

	free(layer_props);
	return found;
}

int lv_device_has_extension(VkPhysicalDevice device, const char *name)
{
	uint32_t ext_count = 0;
	vkEnumerateDeviceExtensionProperties(device, NULL, &ext_count, NULL);

	VkExtensionProperties *ext_props = malloc(sizeof(VkExtensionProperties) * ext_count);
	vkEnumerateDeviceExtensionProperties(device, NULL, &ext_count, ext_props);
	
	int found = 0;
	for (int i = 0; i < ext_count; ++i)
	{
		if (strcmp(name, ext_props[i].extensionName) == 0)
		{
			found = 1;
			break;
		}
	}

	free(ext_props);
	return found;
}

int lv_device_has_graphics_queue(VkPhysicalDevice device, int *idx)
{
	uint32_t queue_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_count, NULL);

	VkQueueFamilyProperties *queue_props = malloc(sizeof(VkQueueFamilyProperties) * queue_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_count, queue_props);

	int found = 0;
	for (int i = 0; i < queue_count; ++i)
	{
		if (queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			if (idx != NULL)
			{
				*idx = i;
			}

			found = 1;
			break;
		}
	}

	free(queue_props);
	return found;
}

int lv_device_has_present_queue(VkPhysicalDevice device, VkSurfaceKHR surface, int *idx)
{
	uint32_t queue_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_count, NULL);

	VkQueueFamilyProperties *queue_props = malloc(sizeof(VkQueueFamilyProperties) * queue_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_count, queue_props);

	int found = 0;
	VkBool32 support = 0;
	for (int i = 0; i < queue_count; ++i)
	{
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &support);
		if (support)
		{
			if (idx != NULL)
			{
				*idx = i;
			}

			found = 1;
			break;
		}
	}

	free(queue_props);
	return found;
}

VkSurfaceCapabilitiesKHR lv_device_surface_get_capabilities(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);

	return capabilities;
}

int lv_device_surface_format_count(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	uint32_t format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, NULL);
	return format_count;
}

int lv_device_surface_has_format(VkPhysicalDevice device, VkSurfaceKHR surface, VkFormat format, VkColorSpaceKHR cspace, int *idx)
{
	uint32_t format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, NULL);

	if (format_count == 0)
	{
		return 0;
	}

	VkSurfaceFormatKHR *formats = malloc(sizeof(VkSurfaceFormatKHR) * format_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, formats);

	int found = 0;
	for (int i = 0; i < format_count; ++i)
	{
		if (formats[i].format == format && formats[i].colorSpace == cspace)
		{
			if (idx != NULL)
			{
				*idx = i;
			}

			found = 1;
			break;
		}
	}
	
	free(formats);
	return found;
}

VkSurfaceFormatKHR lv_device_surface_get_format_by_index(VkPhysicalDevice device, VkSurfaceKHR surface, int index)
{
	VkSurfaceFormatKHR format = { 0 };
	
	uint32_t format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, NULL);

	if (index >= format_count)
	{
		return format;
	}

	VkSurfaceFormatKHR *formats = malloc(sizeof(VkSurfaceFormatKHR) * format_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, formats);

	format.format     = formats[index].format;
	format.colorSpace = formats[index].colorSpace;

	free(formats);
	return format;
}

int lv_device_surface_present_mode_count(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, NULL);
	return present_mode_count;
}

VkPresentModeKHR lv_device_surface_get_present_mode_by_index(VkPhysicalDevice device, VkSurfaceKHR surface, int index)
{
	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, NULL);

	if (index >= present_mode_count)
	{
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkPresentModeKHR *present_modes = malloc(sizeof(VkPresentModeKHR) * present_mode_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, present_modes);

	return present_modes[index];
}

int lv_device_surface_has_present_mode(VkPhysicalDevice device, VkSurfaceKHR surface, VkPresentModeKHR mode, int *idx)
{
	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, NULL);

	if (present_mode_count == 0)
	{
		return 0;
	}

	VkPresentModeKHR *present_modes = malloc(sizeof(VkPresentModeKHR) * present_mode_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, present_modes);

	int found = 0;
	for (int i = 0; i < present_mode_count; ++i)
	{
		if (present_modes[i] == mode)
		{
			if (idx != NULL)
			{
				*idx = i;
			}

			found = 1;
			break;
		}
	}

	free(present_modes);
	return found;
}

int lv_create_swapchain(VkPhysicalDevice pdevice, VkSurfaceKHR surface, VkDevice ldevice,
		lv_queue_s *gqueue, lv_queue_s *pqueue, VkSwapchainKHR *swapchain)
{
	
	VkSurfaceCapabilitiesKHR caps = lv_device_surface_get_capabilities(pdevice, surface);
	VkSurfaceFormatKHR format = lv_device_surface_get_format_by_index(pdevice, surface, 0);

	VkSwapchainCreateInfoKHR info = { 0 };
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = surface;

	info.minImageCount = caps.minImageCount + 1;
	info.imageFormat = format.format;
	info.imageColorSpace = format.colorSpace;
	info.imageExtent = caps.currentExtent; // TODO
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queue_indices[] = { gqueue->index, pqueue->index };
	
	if (gqueue->index == pqueue->index)
	{
		info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	else
       	{
		info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		info.queueFamilyIndexCount = 2;
		info.pQueueFamilyIndices = queue_indices;
	}
	
	info.preTransform = caps.currentTransform;
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.presentMode = VK_PRESENT_MODE_FIFO_KHR; // TODO
	info.clipped = VK_TRUE;

	return (vkCreateSwapchainKHR(ldevice, &info, NULL, swapchain) == VK_SUCCESS);
}

int lv_get_swapchain_images(VkDevice ldevice, VkSwapchainKHR swapchain, lv_image_set_s *images)
{
	vkGetSwapchainImagesKHR(ldevice, swapchain, &images->count, NULL);

	images->images = malloc(sizeof(VkImage) * images->count);
	return (vkGetSwapchainImagesKHR(ldevice, swapchain, &images->count, images->images) == VK_SUCCESS);
}

int lv_create_swapchain_imageviews(VkPhysicalDevice pdevice, VkDevice ldevice, VkSurfaceKHR surface, lv_image_set_s *images, VkImageView *image_views)
{
	image_views = malloc(sizeof(VkImageView) * images->count);

	VkSurfaceFormatKHR format = lv_device_surface_get_format_by_index(pdevice, surface, 0);

	int created = 0;
	for (int i = 0; i < images->count; ++i)
	{
		VkImageViewCreateInfo info = { 0 };
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image = images->images[i];
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.format = format.format;
		info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = 1;
		
		if (vkCreateImageView(ldevice, &info, NULL, &image_views[i]) == VK_SUCCESS)
		{
			++created;
		}
	}
	return created == images->count;

	// TODO the image views have to be destroyed somewhere, somehow, at the end
	// vkDestroyImageView()
}

/*
 * name needs to have a size of at least
 * VK_MAX_PHYSICAL_DEVICE_NAME_SIZE
 */
void lv_device_name(VkPhysicalDevice device, char *name)
{
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(device, &props);
	
	strncpy(name, props.deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE);
}

int lv_print_devices(VkInstance instance)
{
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance, &device_count, NULL);

	if (device_count == 0)
	{
		return 0;
	}

	VkPhysicalDevice *devices = malloc(sizeof(VkPhysicalDevice) * device_count);
	vkEnumeratePhysicalDevices(instance, &device_count, devices);

	char *device_name = malloc(sizeof(char) * VK_MAX_PHYSICAL_DEVICE_NAME_SIZE);

	for (int i = 0; i < device_count; ++i)
	{
		lv_device_name(devices[i], device_name);
		fprintf(stdout, "%*d: %s\n", 2, i+1, device_name);
	}

	free(device_name);
	free(devices);
	return device_count;
}

/*
 * Returns 3 for a discrete GPU, 2 for an integrated GPU, 1 for all other
 * recognized GPU types, 0 for unknown (other) devices.
 */
int lv_device_score(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(device, &props);

	if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_OTHER)
	{
		return 0;
	}
	if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	{
		return 3;
	}
	if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
	{
		return 2;
	}
	// Any other type
	return 1;
}

int lv_logical_device_create(VkDevice *ldevice, VkPhysicalDevice *pdevice, 
		lv_queue_s *gqueue, lv_queue_s *pqueue, lv_name_set_s *extensions)
{
	VkDeviceQueueCreateInfo queue_info = { 0 };
	VkPhysicalDeviceFeatures device_features = { 0 };
	VkDeviceCreateInfo device_info = { 0 };
	
	queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_info.queueFamilyIndex = gqueue->index;
	queue_info.queueCount = 1;
	queue_info.pQueuePriorities = &gqueue->priority;

	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pQueueCreateInfos = &queue_info;
	device_info.queueCreateInfoCount = 1;
	device_info.pEnabledFeatures = &device_features;
	device_info.enabledExtensionCount = extensions->count;
	device_info.ppEnabledExtensionNames = extensions->names;

	if (vkCreateDevice(*pdevice, &device_info, NULL, ldevice) != VK_SUCCESS)
	{
		return 0;
	}

	vkGetDeviceQueue(*ldevice, gqueue->index, 0, &gqueue->queue);
	vkGetDeviceQueue(*ldevice, pqueue->index, 0, &pqueue->queue);

	return gqueue->queue != VK_NULL_HANDLE && pqueue->queue != VK_NULL_HANDLE;
}

/*
 * Loads a file containing SPIR-V shader bytecode and stores the data,
 * as well as its size in bytes, in the provided lv_shader_s struct.
 * TODO we could check the file suffix and set the shader->type
 *      accordingly (.vert, .tesc, .tese, .frag, .geom, .comp)
 */
int lv_load_shader_spv(const char* path, lv_shader_s *shader)
{
	// Try to open file for reading
	FILE *file = fopen(path, "rb");
	if (file == NULL)
	{
		return 0;
	}

	// Try to query file information
	struct stat st;
	if (stat(path, &st) == -1)
	{
		return 0;
	}

	// Try to allocate memory for shader data
	shader->data = malloc(sizeof(char) * st.st_size); // TODO needs a free somewhere
	if (shader->data == NULL)
	{
		return 0;
	}

	// Check if number of bytes read is different from shader file size
	if (fread(shader->data, 1, st.st_size, file) != st.st_size)
	{
		return 0;
	}

	shader->size = st.st_size;
	return 1; 
}

/*
 * Creates a shader module from the SPIR-V shader byte code give in the
 * provided lv_shader_s struct. The shader module will be stored in the
 * struct as well.
 * TODO figure out if we can safely free the shader->data once it's 
 *      been turned into a VkShaderModule handle?
 */
int lv_shader_module_create(VkDevice ldevice, lv_shader_s *shader)
{
	VkShaderModuleCreateInfo info = { 0 };
	info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	info.codeSize = shader->size;
	info.pCode    = shader->data;

	shader->module = malloc(sizeof(VkShaderModule)); // TODO this needs a free. also, we could avoid this by using a non-pointer member instead
	VkResult lol = vkCreateShaderModule(ldevice, &info, NULL, shader->module);
	if (lol == VK_ERROR_OUT_OF_HOST_MEMORY)
	{
		fprintf(stderr, "vkCreateShaderModule(): VK_ERROR_OUT_OF_HOST_MEMORY\n");
	}

	return lol == VK_SUCCESS;
}

int lv_shader_from_file_spv(VkDevice ldevice, const char *path, lv_shader_s *shader)
{
	if (lv_load_shader_spv(path, shader) == 0)
	{
		return 0;
	}
	if (lv_shader_module_create(ldevice, shader) == 0)
	{
		return 0;
	}
	return 1;
}

int lv_shader_stage_create(VkPhysicalDevice pdevice, VkDevice ldevice, VkSurfaceKHR surface, VkShaderModule *vert_shader_module, VkShaderModule *frag_shader_module)
{
	VkPipelineShaderStageCreateInfo vert_info = { 0 };
	vert_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_info.module = *vert_shader_module;
	vert_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_info = { 0 };
	frag_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_info.module = *frag_shader_module;
	frag_info.pName = "main";

	// TODO ??? we're not doing anything with that shizzle yet lul


	VkPipelineVertexInputStateCreateInfo vertexInputInfo = { 0 };
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;


	VkPipelineInputAssemblyStateCreateInfo inputAssembly = { 0 };
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;


	VkSurfaceCapabilitiesKHR caps = lv_device_surface_get_capabilities(pdevice, surface);

	VkViewport viewport = { 0 };
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) caps.currentExtent.width;
	viewport.height = (float) caps.currentExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	
	VkRect2D scissor = { 0 };
	scissor.offset.x = 0;
        scissor.offset.y = 0;
	scissor.extent = caps.currentExtent;

	VkPipelineViewportStateCreateInfo viewportState = { 0 };
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = { 0 };
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = { 0 };
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = { 0 };
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending = { 0 };
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	VkDynamicState dynamicStates[] = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState = { 0 };
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	VkPipelineLayout pipelineLayout;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = { 0 };
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	if (vkCreatePipelineLayout(ldevice, &pipelineLayoutInfo, NULL, &pipelineLayout) != VK_SUCCESS) {
		return 0;
	}

	return 1;
}
