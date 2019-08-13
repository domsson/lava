#include <vulkan/vulkan.h>

struct lava_config
{
	int validation;
	// func pointer to debug_callback_function for validation message handling
};

typedef struct lava_config lava_config_t;

struct lava_name_set
{
	const char* const* names; // same as const char** ?
	uint32_t           count;
};

typedef struct lava_name_set lava_name_set_t;

struct lava_queue
{
	VkQueue  queue;
	uint32_t index;
	float priority;
};

struct lava_image_set
{
	VkImage  *images;
	uint32_t  count;
};

typedef struct lava_image_set lava_image_set_t;

typedef struct lava_queue lava_queue_t;

struct lava_state
{
	lava_config_t    *config;
	VkInstance        vk_instance;
	VkPhysicalDevice  physical_device;
	VkDevice          logical_device;
	lava_queue_t     *graphics_queue;
	lava_queue_t     *present_queue;
	VkSurfaceKHR      surface;
};

typedef struct lava_state lava_state_t;

//
//
//

lava_state_t *lava_init(lava_config_t *cfg)
{
	lava_state_t *lv = malloc(sizeof(lava_state_t));
	// TODO copy this instead, so the caller can free their thingy if they want
	lv->config = cfg;

	return lv;
}

// const char* const* -> "a pointer to a constant pointer to a char constant"
int lava_instance_create(VkInstance *instance, lava_name_set_t *extensions, lava_name_set_t *layers)
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

int lava_print_extensions()
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

int lava_instance_has_extension(const char *name)
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

int lava_print_layers()
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

int lava_instance_has_layer(const char *name)
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

int lava_device_has_extension(VkPhysicalDevice device, const char *name)
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

int lava_device_has_graphics_queue(VkPhysicalDevice device, int *idx)
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

int lava_device_has_present_queue(VkPhysicalDevice device, VkSurfaceKHR surface, int *idx)
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

VkSurfaceCapabilitiesKHR lava_device_surface_get_capabilities(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);

	return capabilities;
}

int lava_device_surface_format_count(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	uint32_t format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, NULL);
	return format_count;
}

int lava_device_surface_has_format(VkPhysicalDevice device, VkSurfaceKHR surface, VkFormat format, VkColorSpaceKHR cspace, int *idx)
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

VkSurfaceFormatKHR lava_device_surface_get_format_by_index(VkPhysicalDevice device, VkSurfaceKHR surface, int index)
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

int lava_device_surface_present_mode_count(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, NULL);
	return present_mode_count;
}

VkPresentModeKHR lava_device_surface_get_present_mode_by_index(VkPhysicalDevice device, VkSurfaceKHR surface, int index)
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

int lava_device_surface_has_present_mode(VkPhysicalDevice device, VkSurfaceKHR surface, VkPresentModeKHR mode, int *idx)
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

int lava_create_swapchain(VkPhysicalDevice pdevice, VkSurfaceKHR surface, VkDevice ldevice,
		lava_queue_t *gqueue, lava_queue_t *pqueue, VkSwapchainKHR *swapchain)
{
	
	VkSurfaceCapabilitiesKHR caps = lava_device_surface_get_capabilities(pdevice, surface);
	VkSurfaceFormatKHR format = lava_device_surface_get_format_by_index(pdevice, surface, 0);

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

int lava_get_swapchain_images(VkDevice ldevice, VkSwapchainKHR swapchain, lava_image_set_t *images)
{
	vkGetSwapchainImagesKHR(ldevice, swapchain, &images->count, NULL);

	images->images = malloc(sizeof(VkImage) * images->count);
	return (vkGetSwapchainImagesKHR(ldevice, swapchain, &images->count, images->images) == VK_SUCCESS);
}

int lava_create_swapchain_imageviews(VkPhysicalDevice pdevice, VkDevice ldevice, VkSurfaceKHR surface, lava_image_set_t *images, VkImageView *image_views)
{
	image_views = malloc(sizeof(VkImageView) * images->count);

	VkSurfaceFormatKHR format = lava_device_surface_get_format_by_index(pdevice, surface, 0);

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
void lava_device_name(VkPhysicalDevice device, char *name)
{
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(device, &props);
	
	strncpy(name, props.deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE);
}

int lava_print_devices(VkInstance instance)
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
		lava_device_name(devices[i], device_name);
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
int lava_device_score(VkPhysicalDevice device)
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

int lava_logical_device_create(VkDevice *ldevice, VkPhysicalDevice *pdevice, 
		lava_queue_t *gqueue, lava_queue_t *pqueue, lava_name_set_t *extensions)
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

