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

struct lv
{
	GLFWwindow		*window;
	VkInstance       	 vulkan_instance;
	VkPhysicalDevice 	 physical_device;	// aka GPU
	VkDevice         	 logical_device;	// we talk to the GPU via this
	VkQueue			 graphics_queue;	// graphics queue
	uint32_t		 graphics_queue_index;	// graphcis queue index
	VkQueue			 present_queue;
	uint32_t		 present_queue_index;
	VkSurfaceKHR		 surface;		// surface we draw on
	VkSwapchainKHR           swapchain;
	lv_image_set_s           swapchain_images;
	VkImageView             *imageviews;
	lv_shader_s	 	 vertex_shader;
	lv_shader_s 		 fragment_shader;
};

int load_shaders(struct lv *lv)
{
	if (lv_shader_from_file_spv(lv->logical_device, "./shaders/default.vert.spv", &lv->vertex_shader, LV_SHADER_VERT) == 0)
	{
		return 0;
	}

	if (lv_shader_from_file_spv(lv->logical_device, "./shaders/default.frag.spv", &lv->fragment_shader, LV_SHADER_FRAG) == 0)
	{
		return 0;
	}

	return 1;
	// TODO gotta free these motherflippers somewhere at some point lol
}

int create_glfw_window(struct lv *lv)
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

int create_vk_instance(struct lv *lv)
{
	lv_name_set_s extensions = { 0 };
	extensions.names = glfwGetRequiredInstanceExtensions(&extensions.count);
	
	lv_name_set_s layers = { 0 };
	const char* names[] = { "VK_LAYER_LUNARG_standard_validation" };
	layers.names = names;
	layers.count = 1;

	return lv_instance_create(&lv->vulkan_instance, &extensions, &layers);
}

int create_surface(struct lv *lv)
{
    if (glfwCreateWindowSurface(lv->vulkan_instance, lv->window, NULL, &(lv->surface)) != VK_SUCCESS)
    {
	return 0;
    }
    return 1;
}

int create_swapchain(struct lv *lv)
{
	lv_queue_s gqueue = { .queue = lv->graphics_queue, .index = lv->graphics_queue_index };
	lv_queue_s pqueue = { .queue = lv->present_queue,  .index = lv->present_queue_index };

	if (lv_create_swapchain(lv->physical_device, lv->surface, lv->logical_device, &gqueue, &pqueue, &lv->swapchain) == 0)
	{
		return 0;
	}

	if (lv_get_swapchain_images(lv->logical_device, lv->swapchain, &lv->swapchain_images) == 0)
	{
		return 0;
	}

	return 1;
}

int create_swapchain_imageviews(struct lv *lv)
{
	return lv_create_swapchain_imageviews(lv->physical_device, lv->logical_device, lv->surface, &lv->swapchain_images, lv->imageviews);
}

int swapchain_adequate(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	return lv_device_surface_format_count(device, surface) &&
		lv_device_surface_present_mode_count(device, surface);
}

int select_vk_gpu(struct lv *lv)
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
		int suitability        = lv_device_score(devices[i]);
		int has_graphics_queue = lv_device_has_graphics_queue(devices[i], &graphics_queue_index);
		int has_present_queue  = lv_device_has_present_queue(devices[i], lv->surface, &present_queue_index);
		int supports_swapchain = lv_device_has_extension(devices[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME);
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

int create_logical_device(struct lv *lv)
{
	lv_name_set_s extensions = { 0 };

	const char* names[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	extensions.names = names;
	extensions.count = 1;

	lv_queue_s graphics_queue = { .priority = 1.0f };
	lv_queue_s present_queue  = { 0 };

	return lv_logical_device_create(&lv->logical_device, &lv->physical_device,
			&graphics_queue, &present_queue, &extensions); 
}

void main_loop(struct lv *lv)
{
	while (!glfwWindowShouldClose(lv->window))
	{
		glfwPollEvents();
	}
}

int main()
{
	// INIT

	struct lv lv = { 0 };

	if (create_glfw_window(&lv) == 0)
	{
		fprintf(stderr, "Could not create GLFW window\n");
		return EXIT_FAILURE;
	}

	if (lv_instance_has_layer("VK_LAYER_LUNARG_standard_validation") == 0)
	{
		fprintf(stderr, "Validation layer is not available\n");
	}

	if (create_vk_instance(&lv) == 0)
	{
		fprintf(stderr, "Could not create Vulkan instance\n");
		return EXIT_FAILURE;
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

	if (lv_device_surface_has_format(lv.physical_device, lv.surface, VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, NULL) == 0)
	{
		fprintf(stderr, "Device does not have desired surface format available\n");
	}

	if (lv_device_surface_has_present_mode(lv.physical_device, lv.surface, VK_PRESENT_MODE_MAILBOX_KHR, NULL) == 0)
	{
		fprintf(stderr, "Device does not have desired present mode available\n");
	}

	if (lv_device_surface_has_present_mode(lv.physical_device, lv.surface, VK_PRESENT_MODE_FIFO_KHR, NULL) == 0)
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

	if (load_shaders(&lv) == 0)
	{
		fprintf(stderr, "Failed loading shaders\n");
		return EXIT_FAILURE;
	}

	fprintf(stdout, "Devices available:\n");
	lv_print_devices(lv.vulkan_instance);

	fprintf(stdout, "Extensions available:\n");
	lv_print_extensions();

	fprintf(stdout, "Layers available:\n");
	lv_print_layers();

	int dsfc = lv_device_surface_format_count(lv.physical_device, lv.surface);
	fprintf(stdout, "Number of surface formats available: %d\n", dsfc);

	int dspmc = lv_device_surface_present_mode_count(lv.physical_device, lv.surface);
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
