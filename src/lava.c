#include <stdio.h>		// stdout, stderr, ...
#include <stdlib.h>		// malloc(), ...
#include <string.h>		// strcmp(), ...
#include <unistd.h>             // sleep()

#include <vulkan/vulkan.h>
#include "liblava.c"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600
#define WINDOW_TITLE  "LAVA LAVA"

#define VALIDATION_LAYER "VK_LAYER_KHRONOS_validation"

#define SHADER_VERT "./shaders/default.vert.spv"
#define SHADER_FRAG "./shaders/default.frag.spv"

// TODO rename "lava", because someone else came out with a liblava (that actually works)
//      at pretty much the same time (about a month later) as I started working on this :-)

int init_pipeline(lv_state_s *lv)
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
	if (lv_shader_from_file_spv(lv->device, SHADER_VERT, &lv->vert_shader, LV_SHADER_VERT) == 0)
	{
		return 0;
	}

	if (lv_shader_from_file_spv(lv->device, SHADER_FRAG, &lv->frag_shader, LV_SHADER_FRAG) == 0)
	{
		return 0;
	}

	if (lv_shader_stage_create(lv->gpu, lv->device, lv->surface, &lv->vert_shader, &lv->frag_shader) == 0)
	{
		return 0;
	}

	return 1;
}

int init_window(lv_state_s *lv)
{
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

int init_validation(lv_state_s *lv)
{
	if (lv_instance_has_layer(VALIDATION_LAYER) == 0)
	{
		return 0;
	}

	return 1;
}

int init_instance(lv_state_s *lv)
{
	lv_name_set_s extensions = { 0 };
	extensions.names = glfwGetRequiredInstanceExtensions(&extensions.count);

	fprintf(stderr, "GLFW required extensions:\n");
	for (int i = 0; i < extensions.count; ++i)
	{
		fprintf(stderr, " - %s\n", extensions.names[i]);
	}
	
	lv_name_set_s layers = { 0 };
	const char* names[] = { VALIDATION_LAYER };
	layers.names = names;
	layers.count = 1;

	return lv_instance_create(lv, &extensions, &layers);
}

int init_surface(lv_state_s *lv)
{
    if (glfwCreateWindowSurface(lv->instance, lv->window, NULL, &(lv->surface)) != VK_SUCCESS)
    {
	return 0;
    }
    return 1;
}

int init_swapchain(lv_state_s *lv)
{
	if (lv_create_swapchain(lv->gpu, lv->surface, lv->device, &lv->gqueue, &lv->pqueue, &lv->swapchain) == 0)
	{
		return 0;
	}

	if (lv_get_swapchain_images(lv->device, lv->swapchain, &lv->swapchain_images) == 0)
	{
		return 0;
	}

	if (lv_create_swapchain_imageviews(lv) == 0)
	{
		return 0;
	}

	return 1;
}

int init_gpu(lv_state_s *lv)
{
	return lv_device_autoselect(lv);
}

int init_physical_device(lv_state_s *lv)
{
	if (lv_device_surface_has_format(lv->gpu, lv->surface, VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, NULL) == 0)
	{
		return 0;
	}

	if (lv_device_surface_has_present_mode(lv->gpu, lv->surface, VK_PRESENT_MODE_FIFO_KHR, NULL) == 0)
	{
		return 0;
	}
	return 1;
}

int init_logical_device(lv_state_s *lv)
{
	lv_name_set_s extensions = { 0 };

	const char* names[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	extensions.names = names;
	extensions.count = 1;

	return lv_logical_device_create(lv, &extensions);
}

int init_framebuffers(lv_state_s *lv)
{
	return lv_create_framebuffers(lv);
}

int init_commandpool(lv_state_s *lv)
{
	return lv_create_commandpool(lv);
}

int init_commandbuffers(lv_state_s *lv)
{
	return lv_create_commandbuffers(lv);
}

int init_semaphores(lv_state_s *lv)
{
	return lv_create_semaphores(lv);
}

void loop(lv_state_s *lv)
{
	while (!glfwWindowShouldClose(lv->window))
	{
		glfwPollEvents();
		lv_draw_frame(lv);	
		sleep(1);
	}
}

void kill(lv_state_s *lv)
{
	lv_free(lv);

	glfwDestroyWindow(lv->window);
	glfwTerminate();
}

int main()
{
	// INIT
	
	lv_state_s lv = { 0 };

	// https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Base_code
	if (init_window(&lv) == 0)
	{
		fprintf(stderr, "Could not create GLFW window\n");
		return EXIT_FAILURE;
	}

	// https://vulkan-tutorial.com/en/Drawing_a_triangle/Setup/Validation_layers
	if (init_validation(&lv) == 0)
	{
		fprintf(stderr, "Could not initialize validation layer\n");
		return EXIT_FAILURE;
	}
	
	// https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Instance	
	if (init_instance(&lv) == 0)
	{
		fprintf(stderr, "Could not create Vulkan instance\n");
		return EXIT_FAILURE;
	}

	// https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Window_surface
	if (init_surface(&lv) == 0)
	{
		fprintf(stderr, "Could not create drawing surface\n");
		return EXIT_FAILURE;
	}

	// https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Physical_devices_and_queue_families
	if (init_gpu(&lv) == 0)
	{
		fprintf(stderr, "Could not find a GPU with Vulkan support\n");
		return EXIT_FAILURE;
	}

	// https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Physical_devices_and_queue_families
	if (init_physical_device(&lv) == 0)
	{
		fprintf(stderr, "Could not initialize physical device\n");
		return EXIT_FAILURE;
	}
	
	// https://vulkan-tutorial.com/en/Drawing_a_triangle/Setup/Logical_device_and_queues
	if (init_logical_device(&lv) == 0)
	{
		fprintf(stderr, "Could not create logical device\n");
		return EXIT_FAILURE;
	}	

	// https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain
	// https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Image_views
	if (init_swapchain(&lv) == 0)
	{
		fprintf(stderr, "Could not create swapchain\n");
		return EXIT_FAILURE;
	}

	// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules
	if (load_shaders(&lv) == 0)
	{
		fprintf(stderr, "Failed loading shaders\n");
		return EXIT_FAILURE;
	}
	
	// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes
	// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
	if (init_pipeline(&lv) == 0)
	{
		fprintf(stderr, "Failed pipelining the render sausage accumulator pass\n");
		return EXIT_FAILURE;
	}

	// https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Framebuffers
	if (init_framebuffers(&lv) == 0)
	{
		fprintf(stderr, "Failed creating framebuffers\n");
		return EXIT_FAILURE;
	}

	// https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Command_buffers
	if (init_commandpool(&lv) == 0)
	{
		fprintf(stderr, "Failed creating command pool\n");
		return EXIT_FAILURE;
	}
	
	// https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Command_buffers
	if (init_commandbuffers(&lv) == 0)
	{
		fprintf(stderr, "Failed creating command buffers\n");
		return EXIT_FAILURE;
	}

	// https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Rendering_and_presentation
	if (init_semaphores(&lv) == 0)
	{
		fprintf(stderr, "Failed creating semaphores\n");
		return EXIT_FAILURE;
	}
	
	// TODO continue the tutorial

	fprintf(stdout, "Devices available:\n");
	lv_print_devices(lv.instance);

	fprintf(stdout, "Extensions available:\n");
	lv_print_extensions();

	fprintf(stdout, "Layers available:\n");
	lv_print_layers();

	int dsfc = lv_device_surface_format_count(lv.gpu, lv.surface);
	fprintf(stdout, "Number of surface formats available: %d\n", dsfc);

	int dspmc = lv_device_surface_present_mode_count(lv.gpu, lv.surface);
	fprintf(stdout, "Number of surface present modes available: %d\n", dspmc);

	// LOOP

	loop(&lv);

	// FREE 

	kill(&lv);

	// CIAO
	
	return EXIT_SUCCESS;
}
