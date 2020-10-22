#include <vulkan/vulkan.h>
#include <sys/stat.h>          // stat(), struct stat

//
// ENUMS
//

enum lv_shader_type
{
	LV_SHADER_NONE, // type not specified or unknown
	LV_SHADER_VERT, // vertex
	LV_SHADER_TESC, // tessellation control
	LV_SHADER_TESE, // tessellation evaluation
	LV_SHADER_GEOM, // geometry
	LV_SHADER_FRAG, // fragment
	LV_SHADER_COMP  // compute
};

typedef enum lv_shader_type lv_shader_type_e;

//
// STRUCTS
// 

struct lv_name_set
{
	const char* const* names; // same as const char** ?
	uint32_t           count;
};

typedef struct lv_name_set lv_name_set_s;

struct lv_queue
{
	VkQueue  queue;
	uint32_t index;
	float priority;
};

typedef struct lv_queue lv_queue_s;

struct lv_image_set
{
	VkImage     *images;
	VkImageView *views;
	uint32_t     count;
};

typedef struct lv_image_set lv_image_set_s;


struct lv_shader
{
	uint32_t         *data;		// SPIR-V bytecode
	size_t            size;		// size of bytecode
	lv_shader_type_e  type;		// shader type (vertex, fragment, ...)
	VkShaderModule    module;	// vulkan shader module 
	VkPipelineShaderStageCreateInfo info;
};

typedef struct lv_shader lv_shader_s;

struct lv_buffer_set
{
	union
	{
		VkFramebuffer   *fbs;
		VkCommandBuffer *cbs;
	};
	uint32_t  count;
};

typedef struct lv_buffer_set lv_buffer_set_s;

struct lv_state
{
	void             *window;
	VkInstance        instance;
	VkPhysicalDevice  gpu;
	VkDevice          device;
	lv_queue_s        gqueue;
	lv_queue_s        pqueue;
	VkSurfaceKHR      surface;
	lv_shader_s       vert_shader;
	lv_shader_s       frag_shader;
	VkSwapchainKHR    swapchain;
	lv_image_set_s    swapchain_images;
	VkRenderPass      render_pass;
	VkPipelineLayout  pipeline_layout;
	VkPipeline        pipeline;
	lv_buffer_set_s   framebuffers;
	VkCommandPool     commandpool;
	lv_buffer_set_s   commandbuffers;
	VkSemaphore       image_available;
	VkSemaphore       render_finished;
};

typedef struct lv_state lv_state_s;

//
// FUNCTIONS
//

int lv_instance_create(lv_state_s *lv, lv_name_set_s *extensions, lv_name_set_s *layers)
{
	// some information about our application. This data is technically 
	// optional, but it may provide some useful information to the driver 
	// in order to optimize our specific application
	VkApplicationInfo app = { 0 };
	app.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app.pApplicationName   = "Hello Lava";
	app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app.pEngineName        = "No Engine";
	app.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
	app.apiVersion         = VK_API_VERSION_1_0;

	// tells the Vulkan driver which global extensions and validation 
	// layers we want to use
	VkInstanceCreateInfo info = { 0 };
	info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	info.pApplicationInfo        = &app;
	info.enabledLayerCount       = layers ? layers->count : 0;
	info.ppEnabledLayerNames     = layers ? layers->names : NULL;
	info.enabledExtensionCount   = extensions ? extensions->count : 0;
	info.ppEnabledExtensionNames = extensions ? extensions->names : NULL;

	return (vkCreateInstance(&info, NULL, &lv->instance) == VK_SUCCESS);
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

int lv_create_swapchain(VkPhysicalDevice gpu, VkSurfaceKHR surface, VkDevice device,
		lv_queue_s *gqueue, lv_queue_s *pqueue, VkSwapchainKHR *swapchain)
{
	
	VkSurfaceCapabilitiesKHR caps = lv_device_surface_get_capabilities(gpu, surface);
	VkSurfaceFormatKHR format = lv_device_surface_get_format_by_index(gpu, surface, 0);

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

	return (vkCreateSwapchainKHR(device, &info, NULL, swapchain) == VK_SUCCESS);
}

int lv_get_swapchain_images(VkDevice device, VkSwapchainKHR swapchain, lv_image_set_s *images)
{
	vkGetSwapchainImagesKHR(device, swapchain, &images->count, NULL);

	images->images = malloc(sizeof(VkImage) * images->count);
	return (vkGetSwapchainImagesKHR(device, swapchain, &images->count, images->images) == VK_SUCCESS);
}

static VkResult
lv_create_imageview(VkDevice device, VkImage image, VkSurfaceFormatKHR format, VkImageView *imageview)
{
	VkImageViewCreateInfo info = { 0 };
	info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.image    = image;
	info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	info.format   = format.format;
	info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	info.subresourceRange.baseMipLevel   = 0;
	info.subresourceRange.levelCount     = 1;
	info.subresourceRange.baseArrayLayer = 0;
	info.subresourceRange.layerCount     = 1;
		
	return vkCreateImageView(device, &info, NULL, imageview);
}

int lv_create_swapchain_imageviews(lv_state_s *lv)
{
	lv->swapchain_images.views = malloc(sizeof(VkImageView) * lv->swapchain_images.count);

	VkSurfaceFormatKHR format = lv_device_surface_get_format_by_index(lv->gpu, lv->surface, 0);

	int created = 0;
	for (int i = 0; i < lv->swapchain_images.count; ++i)
	{
		VkImage      image     =  lv->swapchain_images.images[i];
		VkImageView *imageview = &lv->swapchain_images.views[i];

		if (lv_create_imageview(lv->device, image, format, imageview) == VK_SUCCESS)
		{
			++created;
		}
	}
	
	return created == lv->swapchain_images.count;
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

int lv_swapchain_adequate(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	return lv_device_surface_format_count(device, surface) &&
		lv_device_surface_present_mode_count(device, surface);
}

int lv_device_autoselect(lv_state_s *lv)
{
	// check how many devices are avilable
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(lv->instance, &device_count, NULL);

	// fail early if no device available at all
	if (device_count == 0)
	{
		return 0;
	}
	
	// make room for information on all devices
	VkPhysicalDevice *devices = malloc(sizeof(VkPhysicalDevice) * device_count);
	vkEnumeratePhysicalDevices(lv->instance, &device_count, devices);

	int device_score =  0;
	int gqueue_index = -1;
	int pqueue_index = -1;

	// iterate all available devices to find the best one
	for (int i = 0; i < device_count; ++i)
	{
		int score      = lv_device_score(devices[i]);
		int has_gqueue = lv_device_has_graphics_queue(devices[i], &gqueue_index);
		int has_pqueue = lv_device_has_present_queue(devices[i], lv->surface, &pqueue_index);
		int swapchain  = lv_device_has_extension(devices[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		int chain_ok   = swapchain && lv_swapchain_adequate(devices[i], lv->surface);

		if (score > device_score && has_gqueue && has_pqueue && chain_ok)
		{
			device_score = score;
			lv->gpu = devices[i];
			lv->gqueue.index = gqueue_index;
			lv->pqueue.index = pqueue_index;
			break;
		}
	}

	free(devices);
	return lv->gpu != VK_NULL_HANDLE;
}

int lv_logical_device_create(lv_state_s *lv, lv_name_set_s *extensions)
{
	lv->gqueue.priority = 1.0f;

	VkDeviceQueueCreateInfo queue_info = { 0 };
	VkPhysicalDeviceFeatures device_features = { 0 };
	VkDeviceCreateInfo device_info = { 0 };
	
	queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_info.queueFamilyIndex = lv->gqueue.index;
	queue_info.queueCount = 1;
	queue_info.pQueuePriorities = &lv->gqueue.priority;

	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pQueueCreateInfos = &queue_info;
	device_info.queueCreateInfoCount = 1;
	device_info.pEnabledFeatures = &device_features;
	device_info.enabledExtensionCount = extensions->count;
	device_info.ppEnabledExtensionNames = extensions->names;

	if (vkCreateDevice(lv->gpu, &device_info, NULL, &lv->device) != VK_SUCCESS)
	{
		return 0;
	}

	vkGetDeviceQueue(lv->device, lv->gqueue.index, 0, &lv->gqueue.queue);
	vkGetDeviceQueue(lv->device, lv->pqueue.index, 0, &lv->pqueue.queue);

	return lv->gqueue.queue != VK_NULL_HANDLE && lv->pqueue.queue != VK_NULL_HANDLE;
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
int lv_shader_module_create(VkDevice device, lv_shader_s *shader)
{
	VkShaderModuleCreateInfo info = { 0 };
	info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	info.codeSize = shader->size;
	info.pCode    = shader->data;

	return vkCreateShaderModule(device, &info, NULL, &shader->module) == VK_SUCCESS;
}

int lv_shader_stage_create(VkPhysicalDevice gpu, VkDevice device, VkSurfaceKHR surface, lv_shader_s *vert_shader, lv_shader_s *frag_shader)
{
	VkPipelineShaderStageCreateInfo vert_info = { 0 };
	vert_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_info.module = vert_shader->module;
	vert_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_info = { 0 };
	frag_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_info.module = frag_shader->module;
	frag_info.pName = "main";

	vert_shader->info = vert_info;
	frag_shader->info = frag_info;

	return 1;
}

int lv_shader_from_file_spv(VkDevice device, const char *path, lv_shader_s *shader, lv_shader_type_e type)
{
	if (lv_load_shader_spv(path, shader) == 0)
	{
		return 0;
	}

	shader->type = type;

	if (lv_shader_module_create(device, shader) == 0)
	{
		return 0;
	}

	return 1;
}

// https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes
int lv_renderpass_create(lv_state_s *lv)
{
	VkSurfaceFormatKHR format = lv_device_surface_get_format_by_index(lv->gpu, lv->surface, 0);

	VkAttachmentDescription colorAttachment = { 0 };
	colorAttachment.format         = format.format;
	colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = { 0 };
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = { 0 };
	subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments    = &colorAttachmentRef;

	VkSubpassDependency dependency = { 0 };
	dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass    = 0;
	dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo = { 0 };
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments    = &colorAttachment;
	renderPassInfo.subpassCount    = 1;
	renderPassInfo.pSubpasses      = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies   = &dependency;

	return vkCreateRenderPass(lv->device, &renderPassInfo, NULL, &lv->render_pass) == VK_SUCCESS;
}

// https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
int lv_pipeline_create(lv_state_s *lv)
{
	// TODO ???

	// describes the format of the vertex data that will be passed to 
	// the vertex shader
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = { 0 };
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount   = 0;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;

	// describes two things: what kind of geometry will be drawn from
	// the vertices and if primitive restart should be enabled
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = { 0 };
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// 
	VkSurfaceCapabilitiesKHR caps = lv_device_surface_get_capabilities(lv->gpu, lv->surface);

	// A viewport basically describes the region of the framebuffer that 
	// the output will be rendered to. This will almost always be (0, 0) 
	// to (width, height)
	VkViewport viewport = { 0 };
	viewport.x        = 0.0f;
	viewport.y        = 0.0f;
	viewport.width    = (float) caps.currentExtent.width;
	viewport.height   = (float) caps.currentExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	
	// scissor rectangles define in which regions pixels will actually 
	// be stored. Any pixels outside the scissor rectangles will be 
	// discarded by the rasterizer.
	VkRect2D scissor = { 0 };
	scissor.offset.x = 0;
        scissor.offset.y = 0;
	scissor.extent   = caps.currentExtent;

	// viewport and scissor rectangle need to be combined into 
	// a viewport state 
	VkPipelineViewportStateCreateInfo viewportState = { 0 };
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports    = &viewport;
	viewportState.scissorCount  = 1;
	viewportState.pScissors     = &scissor;

	// The rasterizer takes the geometry that is shaped by the vertices 
	// from the vertex shader and turns it into fragments to be colored 
	// by the fragment shader
	VkPipelineRasterizationStateCreateInfo rasterizer = { 0 };
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable        = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth               = 1.0f;
	rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable         = VK_FALSE;

	// multisampling, which is one of the ways to perform anti-aliasing. 
	// It works by combining the fragment shader results of multiple 
	// polygons that rasterize to the same pixel.
	VkPipelineMultisampleStateCreateInfo multisampling = { 0 };
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable  = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// After a fragment shader has returned a color, it needs to be 
	// combined with the color that is already in the framebuffer. 
	// This transformation is known as color blending

	// VkPipelineColorBlendAttachmentState contains the configuration
	//  per attached framebuffer 	
	VkPipelineColorBlendAttachmentState colorBlendAttachment = { 0 };
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable    = VK_FALSE;

	// VkPipelineColorBlendStateCreateInfo contains the global color 
	// blending settings
	VkPipelineColorBlendStateCreateInfo colorBlending = { 0 };
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable   = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments    = &colorBlendAttachment;

	VkDynamicState dynamicStates[] =
	{
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_LINE_WIDTH
	};

	// A limited amount of the state that we've specified in the previous 
	// structs can actually be changed without recreating the pipeline. 
	// Examples are the size of the viewport, line width and blend constants.
	// If you want to do that, then you'll have to fill in 
	// a VkPipelineDynamicStateCreateInfo structure
	VkPipelineDynamicStateCreateInfo dynamicState = { 0 };
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates    = dynamicStates;

	// You can use uniform values in shaders, which can be changed at 
	// drawing time to alter the behavior of your shaders without having 
	// to recreate them. These uniform values need to be specified during 
	// pipeline creation by creating a VkPipelineLayout object.
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = { 0 };
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	if (vkCreatePipelineLayout(lv->device, &pipelineLayoutInfo, NULL, &lv->pipeline_layout) != VK_SUCCESS)
	{
		return 0;
	}

	const VkPipelineShaderStageCreateInfo shaderStages[] =
	{
		lv->vert_shader.info,
		lv->frag_shader.info
	};
	
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount          = 2;
	pipelineInfo.pStages             = shaderStages;
	pipelineInfo.pVertexInputState   = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState      = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState   = &multisampling;
	pipelineInfo.pColorBlendState    = &colorBlending;
	pipelineInfo.layout              = lv->pipeline_layout;
	pipelineInfo.renderPass          = lv->render_pass;
	pipelineInfo.subpass             = 0;

	if (vkCreateGraphicsPipelines(lv->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &lv->pipeline) != VK_SUCCESS)
	{
		return 0;
	}
	
	return 1;
}

static VkResult
lv_create_framebuffer(VkDevice device, VkRenderPass renderpass, VkImageView view, VkExtent2D extent, VkFramebuffer *buffer)
{
	VkImageView attachments[] = { view };

	VkFramebufferCreateInfo info = { 0 };
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.renderPass      = renderpass;
	info.attachmentCount = 1;
	info.pAttachments    = attachments;
	info.width           = extent.width;
	info.height          = extent.height;
	info.layers          = 1;

	return vkCreateFramebuffer(device, &info, NULL, buffer);
}

int lv_create_framebuffers(lv_state_s *lv)
{
	lv->framebuffers.count = lv->swapchain_images.count;
	lv->framebuffers.fbs   = malloc(sizeof(VkFramebuffer) * lv->framebuffers.count);
	if (lv->framebuffers.fbs == NULL)
	{
		return 0;
	}

	VkSurfaceCapabilitiesKHR caps = lv_device_surface_get_capabilities(lv->gpu, lv->surface);

	VkExtent2D     extent = caps.currentExtent;
	VkImageView    view = 0;
	VkFramebuffer *fb   = NULL;

	for (size_t i = 0; i < lv->framebuffers.count; ++i)
	{
		view =  lv->swapchain_images.views[i];
		fb   = &lv->framebuffers.fbs[i];

		if (lv_create_framebuffer(lv->device, lv->render_pass, view, extent, fb) != VK_SUCCESS)
		{
			return 0;
		}
	}

	return 1;
}

// https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Command_buffers
int lv_create_commandpool(lv_state_s *lv)
{
	VkCommandPoolCreateInfo poolInfo = { 0 };
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = lv->gqueue.index;

	if (vkCreateCommandPool(lv->device, &poolInfo, NULL, &lv->commandpool) != VK_SUCCESS)
	{
		return 0;
	}

	return 1;
}

// https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Command_buffers
int lv_create_commandbuffers(lv_state_s *lv)
{
	lv->commandbuffers.count = lv->swapchain_images.count;
	lv->commandbuffers.cbs   = malloc(sizeof(VkCommandBuffer) * lv->commandbuffers.count);
	if (lv->commandbuffers.cbs == NULL)
	{
		return 0;
	}

	VkCommandBufferAllocateInfo cba_info = { 0 };
	cba_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cba_info.commandPool        = lv->commandpool;
	cba_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cba_info.commandBufferCount = (uint32_t) lv->commandbuffers.count;

	if (vkAllocateCommandBuffers(lv->device, &cba_info, lv->commandbuffers.cbs) != VK_SUCCESS)
	{
		return 0;
	}

	VkCommandBufferBeginInfo cbb_info = { 0 };
	cbb_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkSurfaceCapabilitiesKHR caps = lv_device_surface_get_capabilities(lv->gpu, lv->surface);
	VkClearValue clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
	VkOffset2D offset = { 0, 0 };

	VkRenderPassBeginInfo rp_info = { 0 };
	rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rp_info.renderPass        = lv->render_pass;
	rp_info.renderArea.offset = offset;
	rp_info.renderArea.extent = caps.currentExtent;
	rp_info.clearValueCount   = 1;
	rp_info.pClearValues      = &clear_color;

	for (uint32_t i = 0; i < lv->commandbuffers.count; ++i)
	{
		if (vkBeginCommandBuffer(lv->commandbuffers.cbs[i], &cbb_info) != VK_SUCCESS)
		{
			return 0;
		}

		rp_info.framebuffer = lv->framebuffers.fbs[i];
	
		vkCmdBeginRenderPass(lv->commandbuffers.cbs[i], &rp_info, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(lv->commandbuffers.cbs[i], VK_PIPELINE_BIND_POINT_GRAPHICS, lv->pipeline);
		//                                   .----------- vertexCount
		//                                   |  .-------- instanceCount
		//                                   |  |  .----- firstVertex
		//                                   |  |  |  .-- firstInstance
		//                                   |  |  |  |
		vkCmdDraw(lv->commandbuffers.cbs[i], 3, 1, 0, 0);
		vkCmdEndRenderPass(lv->commandbuffers.cbs[i]);
		if (vkEndCommandBuffer(lv->commandbuffers.cbs[i]) != VK_SUCCESS)
		{
			return 0;
		}
	}
	
	// TODO
	
	return 1;
}

// https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Rendering_and_presentation 
int lv_create_semaphores(lv_state_s *lv)
{
	VkSemaphoreCreateInfo info = { 0 };
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(lv->device, &info, NULL, &lv->image_available) != VK_SUCCESS)
	{
		return 0;
	}
	
	if (vkCreateSemaphore(lv->device, &info, NULL, &lv->render_finished) != VK_SUCCESS)
       	{
		return 0;
	}

	return 1;
}

int lv_draw_frame(lv_state_s *lv)
{
	uint32_t image_index;
	vkAcquireNextImageKHR(lv->device, lv->swapchain, UINT64_MAX, lv->image_available, VK_NULL_HANDLE, &image_index);

	VkSemaphore sem_wait[]   = { lv->image_available };
	VkSemaphore sem_signal[] = { lv->render_finished };
	
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submit_info = { 0 };
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount   = 1;
	submit_info.pWaitSemaphores      = sem_wait;
	submit_info.pWaitDstStageMask    = wait_stages;
	submit_info.commandBufferCount   = 1; // `lv->commandbuffers.count` = segfault (why?)
	submit_info.pCommandBuffers      = &lv->commandbuffers.cbs[image_index];
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores    = sem_signal;

	if (vkQueueSubmit(lv->gqueue.queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		return 0;
	}

	VkPresentInfoKHR presentInfo = { 0 };
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores    = sem_signal;

	VkSwapchainKHR swapChains[] = { lv->swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains    = swapChains;
	presentInfo.pImageIndices  = &image_index;

	vkQueuePresentKHR(lv->pqueue.queue, &presentInfo);

	// TODO continue
	// https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation

	vkQueueWaitIdle(lv->pqueue.queue);
	return 1;
}

int lv_free(lv_state_s *lv)
{
	vkDestroySwapchainKHR(lv->device, lv->swapchain, NULL);
	vkDestroySurfaceKHR(lv->instance, lv->surface, NULL);
	for (int i = 0; i < lv->framebuffers.count; ++i)
	{
		vkDestroyFramebuffer(lv->device, lv->framebuffers.fbs[i], NULL);
	}
	vkDestroyCommandPool(lv->device, lv->commandpool, NULL);
	for (int i = 0; i < lv->swapchain_images.count; ++i)
	{
		vkDestroyImageView(lv->device, lv->swapchain_images.views[i], NULL);
	}
	vkDestroySemaphore(lv->device, lv->image_available, NULL);
	vkDestroySemaphore(lv->device, lv->render_finished, NULL);
	vkDestroyPipelineLayout(lv->device, lv->pipeline_layout, NULL);
	vkDestroyRenderPass(lv->device, lv->render_pass, NULL);
	vkDestroyPipeline(lv->device, lv->pipeline, NULL);
	vkDestroyShaderModule(lv->device, lv->frag_shader.module, NULL);
	vkDestroyShaderModule(lv->device, lv->vert_shader.module, NULL);
	vkDestroyDevice(lv->device, NULL);
	vkDestroyInstance(lv->instance, NULL);

	return 1;
}
