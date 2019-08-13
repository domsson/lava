#include <stdio.h>		// stdout, stderr, ...
#include <stdlib.h>		// malloc(), ...
#include <string.h>		// strcmp(), ...

#include <vulkan/vulkan.h>
#include "liblava.c"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define WINDOW_TITLE "LAVA LAVA"

struct lava
{
	GLFWwindow		*window;
	VkInstance       	vulkan_instance;
	VkPhysicalDevice 	physical_device;	// aka GPU
	VkDevice         	logical_device;		// we talk to the GPU via this
	VkQueue			graphics_queue;		// graphics queue
	uint32_t		graphics_queue_index;	// graphcis queue index
	VkQueue			present_queue;
	uint32_t		present_queue_index;
	VkSurfaceKHR		surface;		// surface we draw on
	VkSwapchainKHR          swapchain;
	lava_image_set_t        swapchain_images;
	VkImageView             *imageviews;
};

int create_glfw_window(struct lava *lv)
{
	// Attempt to initialize GLFW
	if (glfwInit() == GLFW_FALSE)
	{
		return 0;
	}

	//                                .-- no OpenGL, Vulkan
	//                                |
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	lv->window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
	return lv->window != NULL;
}

int create_vk_instance(struct lava *lv)
{
	lava_name_set_t extensions = { 0 };
	extensions.names = glfwGetRequiredInstanceExtensions(&extensions.count);

	return lava_instance_create(&lv->vulkan_instance, &extensions, NULL);
}

int create_surface(struct lava *lv)
{
    if (glfwCreateWindowSurface(lv->vulkan_instance, lv->window, NULL, &(lv->surface)) != VK_SUCCESS)
    {
	return 0;
    }
    return 1;
}

int create_swapchain(struct lava *lv)
{
	lava_queue_t gqueue = { .queue = lv->graphics_queue, .index = lv->graphics_queue_index };
	lava_queue_t pqueue = { .queue = lv->present_queue,  .index = lv->present_queue_index };

	if (lava_create_swapchain(lv->physical_device, lv->surface, lv->logical_device, &gqueue, &pqueue, &lv->swapchain) == 0)
	{
		return 0;
	}

	if (lava_get_swapchain_images(lv->logical_device, lv->swapchain, &lv->swapchain_images) == 0)
	{
		return 0;
	}

	return 1;
}

int create_swapchain_imageviews(struct lava *lv)
{
	return lava_create_swapchain_imageviews(lv->physical_device, lv->logical_device, lv->surface, &lv->swapchain_images, lv->imageviews);
}

int swapchain_adequate(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	return lava_device_surface_format_count(device, surface) &&
		lava_device_surface_present_mode_count(device, surface);
}

int select_vk_gpu(struct lava *lv)
{
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(lv->vulkan_instance, &device_count, NULL);

	if (device_count == 0)
	{
		return 0;
	}

	VkPhysicalDevice *devices = malloc(sizeof(VkPhysicalDevice) * device_count);
	vkEnumeratePhysicalDevices(lv->vulkan_instance, &device_count, devices);
	int device_suitability = 0;
	int graphics_queue_index = 0;
	int present_queue_index = 0;

	for (int i = 0; i < device_count; ++i)
	{
		int suitability        = lava_device_score(devices[i]);
		int has_graphics_queue = lava_device_has_graphics_queue(devices[i], &graphics_queue_index);
		int has_present_queue  = lava_device_has_present_queue(devices[i], lv->surface, &present_queue_index);
		int supports_swapchain = lava_device_has_extension(devices[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		int chain_adequate     = supports_swapchain && swapchain_adequate(devices[i], lv->surface);

		if (suitability > device_suitability && has_graphics_queue && has_present_queue && chain_adequate)
		{
			device_suitability = suitability;
			lv->physical_device = devices[i];
			lv->graphics_queue_index = graphics_queue_index;
			lv->present_queue_index = present_queue_index;
			break;
		}
	}

	free(devices);
	return lv->physical_device != VK_NULL_HANDLE;
}

int create_logical_device(struct lava *lv)
{
	lava_name_set_t extensions = { 0 };

	const char* names[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	extensions.names = names;
	extensions.count = 1;

	lava_queue_t graphics_queue = { .priority = 1.0f };
	lava_queue_t present_queue  = { 0 };

	return lava_logical_device_create(&lv->logical_device, &lv->physical_device,
			&graphics_queue, &present_queue, &extensions); 
}

void main_loop(struct lava *lv)
{
	while (!glfwWindowShouldClose(lv->window))
	{
		glfwPollEvents();
	}
}

int main()
{
	// INIT

	struct lava lv = { 0 };

	if (create_glfw_window(&lv) == 0)
	{
		fprintf(stderr, "Could not create GLFW window\n");
		return EXIT_FAILURE;
	}

	if (create_vk_instance(&lv) == 0)
	{
		fprintf(stderr, "Could not create Vulkan instance\n");
		return EXIT_FAILURE;
	}

	if (lava_instance_has_layer("VK_LAYER_LUNARG_standard_validation") == 0)
	{
		fprintf(stderr, "Validation layer is not available\n");
	}

	if (create_surface(&lv) == 0)
	{
		fprintf(stderr, "Could not create drawing surface\n");
		return EXIT_FAILURE;
	}

	if (select_vk_gpu(&lv) == 0)
	{
		fprintf(stderr, "Could not find a GPU with Vulkan support\n");
		return EXIT_FAILURE;
	}

	if (lava_device_surface_has_format(lv.physical_device, lv.surface, VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, NULL) == 0)
	{
		fprintf(stderr, "Device does not have desired surface format available\n");
	}

	if (lava_device_surface_has_present_mode(lv.physical_device, lv.surface, VK_PRESENT_MODE_MAILBOX_KHR, NULL) == 0)
	{
		fprintf(stderr, "Device does not have desired present mode available\n");
	}

	if (lava_device_surface_has_present_mode(lv.physical_device, lv.surface, VK_PRESENT_MODE_FIFO_KHR, NULL) == 0)
	{
		fprintf(stderr, "Device does not have any present mode available\n");
	}

	if (create_logical_device(&lv) == 0)
	{
		fprintf(stderr, "Could not create logical device\n");
		return EXIT_FAILURE;
	}	

	if (create_swapchain(&lv) == 0)
	{
		fprintf(stderr, "Could not create swapchain\n");
		return EXIT_FAILURE;
	}

	if (create_swapchain_imageviews(&lv) == 0)
	{
		fprintf(stderr, "Could not create swapchain imageviews\n");
		return EXIT_FAILURE;
	}

	fprintf(stdout, "Devices available:\n");
	lava_print_devices(lv.vulkan_instance);

	fprintf(stdout, "Extensions available:\n");
	lava_print_extensions();

	fprintf(stdout, "Layers available:\n");
	lava_print_layers();

	int dsfc = lava_device_surface_format_count(lv.physical_device, lv.surface);
	fprintf(stdout, "Number of surface formats available: %d\n", dsfc);

	int dspmc = lava_device_surface_present_mode_count(lv.physical_device, lv.surface);
	fprintf(stdout, "Number of surface present modes available: %d\n", dspmc);

	// MAIN LOOP

	main_loop(&lv);

	// FREE SHIT

	vkDestroySurfaceKHR(lv.vulkan_instance, lv.surface, NULL);
	vkDestroySwapchainKHR(lv.logical_device, lv.swapchain, NULL);
	vkDestroyDevice(lv.logical_device, NULL);
	vkDestroyInstance(lv.vulkan_instance, NULL);

	glfwDestroyWindow(lv.window);
	glfwTerminate();

	// BAI BAI

	return EXIT_SUCCESS;
}
