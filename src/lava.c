#include <stdio.h>		// stdout, stderr, ...
#include <stdlib.h>		// malloc(), ...
#include <string.h>		// strcmp(), ...

#include <vulkan/vulkan.h>
#include "liblava.c"

// #define GLFW_INCLUDE_VULKAN
// #include <GLFW/glfw3.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define WINDOW_TITLE "LAVA LAVA"

#define VALIDATION_LAYER "VK_LAYER_KHRONOS_validation"

// TODO rename "lava" to "maar", someone else came out with a liblava (that actually works)
//      at pretty much the same time (about a month later) as I started working on this :-)
//      alternatively to "maar", "raba" (japanese katakana-kotoba for 'lava' is also okay)

int setup_pipeline(lv_state_s *lv)
{
	if (lv_renderpass_create(lv) == 0)
	{
		return 0;
	}

	if (lv_pipeline_create(lv) == 0)
	{
		return 0;
	}

	return 1;
}

int load_shaders(lv_state_s *lv)
{
	if (lv_shader_from_file_spv(lv->ldevice, "./shaders/default.vert.spv", lv->vert_shader, LV_SHADER_VERT) == 0)
	{
		return 0;
	}

	if (lv_shader_from_file_spv(lv->ldevice, "./shaders/default.frag.spv", lv->frag_shader, LV_SHADER_FRAG) == 0)
	{
		return 0;
	}

	if (lv_shader_stage_create(lv->pdevice, lv->ldevice, lv->surface, lv->vert_shader, lv->frag_shader) == 0)
	{
		return 0;
	}

	return 1;
	// TODO gotta free these motherflippers somewhere at some point lol
}

int create_glfw_window(lv_state_s *lv)
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

int create_vk_instance(lv_state_s *lv)
{
	lv_name_set_s extensions = { 0 };
	extensions.names = glfwGetRequiredInstanceExtensions(&extensions.count);
	
	lv_name_set_s layers = { 0 };
	const char* names[] = { VALIDATION_LAYER };
	layers.names = names;
	layers.count = 1;

	return lv_instance_create(&lv->instance, &extensions, &layers);
}

int create_surface(lv_state_s *lv)
{
    if (glfwCreateWindowSurface(lv->instance, lv->window, NULL, &(lv->surface)) != VK_SUCCESS)
    {
	return 0;
    }
    return 1;
}

int create_swapchain(lv_state_s *lv)
{
	if (lv_create_swapchain(lv->pdevice, lv->surface, lv->ldevice, lv->gqueue, lv->pqueue, &lv->swapchain) == 0)
	{
		return 0;
	}

	if (lv_get_swapchain_images(lv->ldevice, lv->swapchain, &lv->swapchain_images) == 0)
	{
		return 0;
	}

	return 1;
}

int create_swapchain_imageviews(lv_state_s *lv)
{
	return lv_create_swapchain_imageviews(lv);
}

int select_vk_gpu(lv_state_s *lv)
{
	return lv_device_autoselect(lv);
}

int create_logical_device(lv_state_s *lv)
{
	lv_name_set_s extensions = { 0 };

	const char* names[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	extensions.names = names;
	extensions.count = 1;

	return lv_logical_device_create(lv, &extensions);
}

int create_framebuffers(lv_state_s *lv)
{
	return lv_create_framebuffers(lv);
}

int create_commandpool(lv_state_s *lv)
{
	return lv_create_commandpool(lv);
}

int create_commandbuffers(lv_state_s *lv)
{
	return lv_create_commandbuffers(lv);
}

int create_semaphores(lv_state_s *lv)
{
	return lv_create_semaphores(lv);
}

void draw_frame(lv_state_s *lv)
{
	// TODO
	uint32_t imageIndex;
	vkAcquireNextImageKHR(lv->ldevice, lv->swapchain, UINT64_MAX, lv->image_available, VK_NULL_HANDLE, &imageIndex);

	VkSemaphore waitSemaphores[]   = { lv->image_available };
	VkSemaphore signalSemaphores[] = { lv->render_finished };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo = { 0 };
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount   = 1;
	submitInfo.pWaitSemaphores      = waitSemaphores;
	submitInfo.pWaitDstStageMask    = waitStages;
	submitInfo.commandBufferCount   = 1;
	submitInfo.pCommandBuffers      = &lv->commandbuffers[imageIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores    = signalSemaphores;
	
	if (vkQueueSubmit(lv->gqueue->queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		fprintf(stderr, "vkQueueSubmit() failed\n");
		return;
	}

	VkPresentInfoKHR presentInfo = { 0 };
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	
	VkSwapchainKHR swapChains[] = { lv->swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains    = swapChains;
	presentInfo.pImageIndices  = &imageIndex;

	vkQueuePresentKHR(lv->pqueue->queue, &presentInfo);
}

void main_loop(lv_state_s *lv)
{
	while (!glfwWindowShouldClose(lv->window))
	{
		glfwPollEvents();
		draw_frame(lv);
	}
}

int main()
{
	// INIT
	
	lv_state_s *lv = lv_init(NULL);

	if (create_glfw_window(lv) == 0)
	{
		fprintf(stderr, "Could not create GLFW window\n");
		return EXIT_FAILURE;
	}

	if (lv_instance_has_layer(VALIDATION_LAYER) == 0)
	{
		fprintf(stderr, "Validation layer is not available\n");
	}

	if (create_vk_instance(lv) == 0)
	{
		fprintf(stderr, "Could not create Vulkan instance\n");
		return EXIT_FAILURE;
	}

	if (create_surface(lv) == 0)
	{
		fprintf(stderr, "Could not create drawing surface\n");
		return EXIT_FAILURE;
	}

	if (select_vk_gpu(lv) == 0)
	{
		fprintf(stderr, "Could not find a GPU with Vulkan support\n");
		return EXIT_FAILURE;
	}

	if (lv_device_surface_has_format(lv->pdevice, lv->surface, VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, NULL) == 0)
	{
		fprintf(stderr, "Device does not have desired surface format available\n");
	}

	if (lv_device_surface_has_present_mode(lv->pdevice, lv->surface, VK_PRESENT_MODE_MAILBOX_KHR, NULL) == 0)
	{
		fprintf(stderr, "Device does not have desired present mode available\n");
	}

	if (lv_device_surface_has_present_mode(lv->pdevice, lv->surface, VK_PRESENT_MODE_FIFO_KHR, NULL) == 0)
	{
		fprintf(stderr, "Device does not have any present mode available\n");
	}

	if (create_logical_device(lv) == 0)
	{
		fprintf(stderr, "Could not create logical device\n");
		return EXIT_FAILURE;
	}	

	if (create_swapchain(lv) == 0)
	{
		fprintf(stderr, "Could not create swapchain\n");
		return EXIT_FAILURE;
	}

	if (create_swapchain_imageviews(lv) == 0)
	{
		fprintf(stderr, "Could not create swapchain imageviews\n");
		return EXIT_FAILURE;
	}

	if (load_shaders(lv) == 0)
	{
		fprintf(stderr, "Failed loading shaders\n");
		return EXIT_FAILURE;
	}
	
	if (setup_pipeline(lv) == 0)
	{
		fprintf(stderr, "Failed pipelining the render sausage accumulator pass\n");
		return EXIT_FAILURE;
	}

	// https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Framebuffers
	if (create_framebuffers(lv) == 0)
	{
		fprintf(stderr, "Failed creating framebuffers\n");
		return EXIT_FAILURE;
	}

	// https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Command_buffers
	if (create_commandpool(lv) == 0)
	{
		fprintf(stderr, "Failed creating command pool\n");
		return EXIT_FAILURE;
	}
	
	// https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Command_buffers
	if (create_commandbuffers(lv) == 0)
	{
		fprintf(stderr, "Failed creating command buffers\n");
		return EXIT_FAILURE;
	}

	// TODO
	// https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Rendering_and_presentation

	if (create_semaphores(lv) == 0)
	{
		fprintf(stderr, "Failed creating semaphores\n");
		return EXIT_FAILURE;
	}

	fprintf(stdout, "Devices available:\n");
	lv_print_devices(lv->instance);

	fprintf(stdout, "Extensions available:\n");
	lv_print_extensions();

	fprintf(stdout, "Layers available:\n");
	lv_print_layers();

	int dsfc = lv_device_surface_format_count(lv->pdevice, lv->surface);
	fprintf(stdout, "Number of surface formats available: %d\n", dsfc);

	int dspmc = lv_device_surface_present_mode_count(lv->pdevice, lv->surface);
	fprintf(stdout, "Number of surface present modes available: %d\n", dspmc);

	// LOOP

	main_loop(lv);

	// FREE 

	vkDestroySurfaceKHR(lv->instance, lv->surface, NULL);
	vkDestroySwapchainKHR(lv->ldevice, lv->swapchain, NULL);
	vkDestroyDevice(lv->ldevice, NULL);
	vkDestroyInstance(lv->instance, NULL);

	glfwDestroyWindow(lv->window);
	glfwTerminate();

	return EXIT_SUCCESS;
}
