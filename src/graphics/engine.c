#include "graphics/engine.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "graphics/io.h"
#include "graphics/resource.h"
#include "graphics/vertex.h"

// TODO: cant go over the swapchain_length
#define MAX_FRAMES_IN_FLIGHT 2

// TODO move
static struct Vertex vertex_pos[] = {
    {
        .pos = { .x = 0.0, .y = 1.0, .z = 1.0 },
    },
    {
        .pos = { .x = 1.0, .y = 0.0, .z = 1.0 },
    },
    {
        .pos = { .x = -1.0, .y = 0.0, .z = 1.0 },
    },
};

// Private Declarations
VkInstance gfx_init_instance(void);
VkSurfaceKHR gfx_init_surface(VkInstance instance, GLFWwindow *window);
struct GfxPhysicalDevice gfx_init_physical_device(VkInstance instance, VkSurfaceKHR surface);
VkDevice gfx_init_device(VkInstance instance, struct GfxPhysicalDevice const *const physical_device);
VkSurfaceFormatKHR gfx_get_surface_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
VkExtent2D gfx_get_extent(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface,
    GLFWwindow *window);
VkSwapchainKHR gfx_init_swapchain(
    VkDevice device,
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface,
    VkSurfaceFormatKHR surface_format,
    VkExtent2D extent);
VkImage* gfx_init_swapchain_images(VkDevice device, VkSwapchainKHR swapchain, uint32_t *length);
VkImageView* gfx_init_swapchain_image_views(
    VkDevice device,
    uint32_t length,
    VkImage const *const swapchain_images,
    struct VkSurfaceFormatKHR const *const surface_format);
VkDescriptorPool gfx_init_descriptor_pool(VkDevice device, uint32_t swapchain_length);
VkDescriptorSetLayout gfx_init_descriptor_layout(VkDevice device);
VkPipelineLayout gfx_init_pipeline_layout(VkDevice device, VkDescriptorSetLayout descriptor_layout);
VkRenderPass gfx_init_render_pass(VkDevice device, VkFormat format);
struct GfxResource gfx_init_vertices_resource(VkDevice device, VkPhysicalDevice physical_device, uint32_t length);
struct GfxResource* gfx_init_uniform_resources(VkDevice device, VkPhysicalDevice physical_device, uint32_t length);
VkDescriptorSet* gfx_init_descriptor_sets(
    VkDevice device,
    uint32_t length,
    VkDescriptorSetLayout descriptor_layout,
    VkDescriptorPool descriptor_pool,
    struct GfxResource *uniform_resources);
VkPipeline gfx_init_pipeline(
    VkDevice device,
    struct VkExtent2D extent,
    VkPipelineLayout pipeline_layout,
    VkRenderPass render_pass);
VkShaderModule gfx_init_shader_module(VkDevice device, uint32_t size, uint32_t const *code);
VkFramebuffer* gfx_init_framebuffers(
    VkDevice device,
    uint32_t image_view_length,
    VkImageView *image_views,
    VkRenderPass render_pass,
    VkExtent2D extent);
VkCommandBuffer* gfx_init_command_buffers(
    VkDevice device,
    uint32_t length,
    VkCommandPool command_pool);
void gfx_record_command_buffers(
    uint32_t length,
    VkCommandBuffer *command_buffers,
    VkRenderPass render_pass,
    VkFramebuffer *framebuffers,
    VkPipeline pipeline,
    VkPipelineLayout pipeline_layout,
    VkBuffer vertex_buffer,
    VkDescriptorSet *descriptor_sets,
    VkExtent2D extent);
void gfx_update_uniform_buffers(VkDevice device, VkDeviceMemory memory, struct UBO *ubo);
void gfx_init_with_extent();
void gfx_deinit_with_extent();
void gfx_reinit_swapchain(GLFWwindow *window);

// Private Structs
struct GfxPhysicalDevice {
    VkPhysicalDevice gpu;
    uint32_t graphics_family_index; 
    VkQueueFamilyProperties graphics_family_properties;
};

struct GfxEngine {
    VkInstance instance;
    VkSurfaceKHR surface;
    struct GfxPhysicalDevice physical_device;
    VkDevice device;
    VkQueue graphics_queue;
    VkSurfaceFormatKHR surface_format;
    struct VkExtent2D extent;
    VkSwapchainKHR swapchain;
    uint32_t swapchain_length;
    VkImage *swapchain_images;
    VkImageView *swapchain_image_views;
    VkCommandPool graphics_command_pool;
    VkDescriptorPool descriptor_pool;
    VkSemaphore *is_image_available_semaphore;
    VkSemaphore *is_present_ready_semaphore;
    VkFence *is_main_render_done;

    VkDescriptorSetLayout descriptor_layout;
    VkPipelineLayout pipeline_layout;
    VkRenderPass render_pass;
    struct GfxResource vertices_resource;
    struct GfxResource *uniform_resources;
    VkDescriptorSet *descriptor_sets;
    VkPipeline pipeline;
    VkFramebuffer *framebuffers;
    VkCommandBuffer *command_buffers;
};

// Global Variables
static struct GfxEngine engine = {};
static VkResult result = VK_SUCCESS;
static uint32_t current_frame = 0;

// Public functions
int gfx_init(GLFWwindow *window) {
    engine.instance = gfx_init_instance();
    engine.surface = gfx_init_surface(engine.instance, window);
    engine.physical_device = gfx_init_physical_device(engine.instance, engine.surface);
    engine.device = gfx_init_device(engine.instance, &engine.physical_device);
    vkGetDeviceQueue(engine.device, engine.physical_device.graphics_family_index, 0, &engine.graphics_queue);
    engine.surface_format = gfx_get_surface_format(engine.physical_device.gpu, engine.surface);
    engine.extent = gfx_get_extent(engine.physical_device.gpu, engine.surface, window);
    engine.swapchain = gfx_init_swapchain(
        engine.device,
        engine.physical_device.gpu,
        engine.surface,
        engine.surface_format,
        engine.extent
    );
    engine.swapchain_images = gfx_init_swapchain_images(engine.device, engine.swapchain, &engine.swapchain_length);
    engine.swapchain_image_views = gfx_init_swapchain_image_views(
        engine.device,
        engine.swapchain_length,
        engine.swapchain_images,
        &engine.surface_format
    );

    VkCommandPoolCreateInfo graphics_command_pool_info =  {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = engine.physical_device.graphics_family_index,
    };
    result = vkCreateCommandPool(engine.device, &graphics_command_pool_info, 0, &engine.graphics_command_pool);
    assert(result == VK_SUCCESS);

    engine.descriptor_pool = gfx_init_descriptor_pool(engine.device, engine.swapchain_length);

    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    engine.is_image_available_semaphore = malloc(MAX_FRAMES_IN_FLIGHT * sizeof *engine.is_image_available_semaphore);
    engine.is_present_ready_semaphore = malloc(MAX_FRAMES_IN_FLIGHT * sizeof *engine.is_present_ready_semaphore);
    engine.is_main_render_done = malloc(MAX_FRAMES_IN_FLIGHT * sizeof *engine.is_main_render_done);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        result = vkCreateSemaphore(engine.device, &semaphore_info, 0, &engine.is_image_available_semaphore[i]);
        assert(result == VK_SUCCESS);
        result = vkCreateSemaphore(engine.device, &semaphore_info, 0, &engine.is_present_ready_semaphore[i]);
        assert(result == VK_SUCCESS);
        result = vkCreateFence(engine.device, &fence_info, 0, &engine.is_main_render_done[i]);
        assert(result == VK_SUCCESS);
    }

    engine.descriptor_layout = gfx_init_descriptor_layout(engine.device);
    engine.pipeline_layout = gfx_init_pipeline_layout(engine.device, engine.descriptor_layout);
    engine.render_pass = gfx_init_render_pass(engine.device, engine.surface_format.format);
    engine.vertices_resource = gfx_init_vertices_resource(engine.device, engine.physical_device.gpu, sizeof vertex_pos);

    // TODO: move to game/main
    void *data;
    vkMapMemory(engine.device, engine.vertices_resource.memory, 0, sizeof vertex_pos, 0, &data);
    memcpy(data, &vertex_pos, sizeof vertex_pos);
    vkUnmapMemory(engine.device, engine.vertices_resource.memory);

    engine.uniform_resources = gfx_init_uniform_resources(engine.device, engine.physical_device.gpu, engine.swapchain_length);
    engine.descriptor_sets = gfx_init_descriptor_sets(
        engine.device,
        engine.swapchain_length,
        engine.descriptor_layout,
        engine.descriptor_pool,
        engine.uniform_resources
    );

    gfx_init_with_extent();

    return 1;
}

void gfx_deinit() {

    vkDeviceWaitIdle(engine.device);

    gfx_deinit_with_extent();

    gfx_destroy_resource(engine.device, &engine.vertices_resource, 0);
    vkDestroyRenderPass(engine.device, engine.render_pass, 0);
    vkDestroyPipelineLayout(engine.device, engine.pipeline_layout, 0);
    vkDestroyDescriptorSetLayout(engine.device, engine.descriptor_layout, 0);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(engine.device, engine.is_image_available_semaphore[i], 0);
        vkDestroySemaphore(engine.device, engine.is_present_ready_semaphore[i], 0);
        vkDestroyFence(engine.device, engine.is_main_render_done[i], 0);
    }
    for (size_t i = 0; i < engine.swapchain_length; i++) {
        gfx_destroy_resource(engine.device, &engine.uniform_resources[i], 0);
    }
    vkDestroyDescriptorPool(engine.device, engine.descriptor_pool, 0);
    vkDestroyCommandPool(engine.device, engine.graphics_command_pool, 0);
    vkDestroyDevice(engine.device, 0);
    vkDestroySurfaceKHR(engine.instance, engine.surface, 0);
    vkDestroyInstance(engine.instance, 0);
}

void gfx_draw_frame(GLFWwindow *window, struct UBO *ubo) {
    result = vkWaitForFences(engine.device, 1, &engine.is_main_render_done[current_frame], VK_TRUE, UINT64_MAX);
    assert(result == VK_SUCCESS);

    result = vkResetFences(engine.device, 1, &engine.is_main_render_done[current_frame]);
    assert(result == VK_SUCCESS);

    uint32_t image_index;
    result = vkAcquireNextImageKHR(
        engine.device,
        engine.swapchain,
        UINT64_MAX,
        engine.is_image_available_semaphore[current_frame],
        0,
        &image_index
    );
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        gfx_deinit_with_extent();
        gfx_reinit_swapchain(window);
    }

    gfx_update_uniform_buffers(engine.device, engine.uniform_resources[current_frame].memory, ubo);

    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &engine.is_image_available_semaphore[current_frame],
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &engine.command_buffers[image_index],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &engine.is_present_ready_semaphore[current_frame],
    };

    result = vkQueueSubmit(engine.graphics_queue, 1, &submit_info, engine.is_main_render_done[current_frame]);
    assert(result == VK_SUCCESS);

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &engine.is_present_ready_semaphore[current_frame],
        .swapchainCount = 1,
        .pSwapchains = &engine.swapchain,
        .pImageIndices = &image_index,
    };

    result = vkQueuePresentKHR(engine.graphics_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        gfx_deinit_with_extent();
        gfx_reinit_swapchain(window);
    }

    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

// Private functions

void gfx_init_with_extent() {
    engine.pipeline = gfx_init_pipeline( 
        engine.device,
        engine.extent,
        engine.pipeline_layout,
        engine.render_pass
    );
    engine.framebuffers = gfx_init_framebuffers(
        engine.device,
        engine.swapchain_length,
        engine.swapchain_image_views,
        engine.render_pass,
        engine.extent
    );
    engine.command_buffers = gfx_init_command_buffers(
        engine.device,
        engine.swapchain_length,
        engine.graphics_command_pool
    );
    gfx_record_command_buffers(
        engine.swapchain_length,
        engine.command_buffers,
        engine.render_pass,
        engine.framebuffers,
        engine.pipeline,
        engine.pipeline_layout,
        engine.vertices_resource.buffer,
        engine.descriptor_sets,
        engine.extent
    );
}

void gfx_reinit_swapchain(GLFWwindow *window) {
    engine.extent = gfx_get_extent(engine.physical_device.gpu, engine.surface, window);
    engine.swapchain = gfx_init_swapchain(
        engine.device,
        engine.physical_device.gpu,
        engine.surface,
        engine.surface_format,
        engine.extent
    );
    engine.swapchain_images = gfx_init_swapchain_images(engine.device, engine.swapchain, &engine.swapchain_length);
    engine.swapchain_image_views = gfx_init_swapchain_image_views(
        engine.device,
        engine.swapchain_length,
        engine.swapchain_images,
        &engine.surface_format
    );

    gfx_init_with_extent();
}

void gfx_deinit_with_extent() {
    vkDeviceWaitIdle(engine.device);

    for (size_t i = 0; i < engine.swapchain_length; i++) {
        vkDestroyFramebuffer(engine.device, engine.framebuffers[i], 0);
    }
    free(engine.framebuffers);
    vkDestroyPipeline(engine.device, engine.pipeline, 0);
    for (size_t i = 0; i < engine.swapchain_length; i++) {
        vkDestroyImageView(engine.device, engine.swapchain_image_views[i], 0);
    }
    free(engine.swapchain_image_views);
    vkDestroySwapchainKHR(engine.device, engine.swapchain, 0);
}

VkInstance gfx_init_instance(void) {
    VkInstance instance = 0;

    char const *layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    uint32_t glfw_extension_count = 0; 
    char const **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    char const *other_extensions[] = {
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME
    };

    unsigned int extensions_count = glfw_extension_count + sizeof other_extensions / sizeof other_extensions[0];
    char **extensions = malloc(extensions_count * sizeof extensions);
    if (!extensions) {
        // TODO: handle malloc
        goto fail_extensions_alloc;
    }

    // TODO: handle errors?
    memcpy(extensions, glfw_extensions, glfw_extension_count * sizeof *glfw_extensions);
    memcpy(&extensions[glfw_extension_count], other_extensions, sizeof other_extensions);

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Flicker",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Flicker",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    };

    VkInstanceCreateInfo const create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = sizeof layers / sizeof layers[0],
        .ppEnabledLayerNames = layers,
        .enabledExtensionCount = extensions_count,
        // TODO: what is the point of const casting?
        .ppEnabledExtensionNames = (char const *const *)extensions,
    };

    result = vkCreateInstance(&create_info, 0, &instance);
    assert(result == VK_SUCCESS);

    free(extensions);
  fail_extensions_alloc:

    return instance;
}

VkSurfaceKHR gfx_init_surface(VkInstance instance, GLFWwindow *window) {
    VkSurfaceKHR surface = 0;
    result = glfwCreateWindowSurface(instance, window, 0, &surface);
    assert(result == VK_SUCCESS);

    return surface;
}

struct GfxPhysicalDevice gfx_init_physical_device(VkInstance instance, VkSurfaceKHR surface) {
    struct GfxPhysicalDevice physical_device = {};

    uint32_t physical_device_count = 0;
    result = vkEnumeratePhysicalDevices(instance, &physical_device_count, 0);
    assert(result == VK_SUCCESS);
    VkPhysicalDevice *physical_devices = malloc(physical_device_count * sizeof *physical_devices);
    if (!physical_devices) {
        // TODO: handle malloc
        goto fail_physical_devices_alloc;
    }
    result = vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices);
    assert(result == VK_SUCCESS);

    // TODO: abstract the sequential array allocation to different struct
    // TODO: is the sequential array allocation faster?
    //       - many mallocs vs pointer math to get offset
    // TODO: storing extra property_count
    uint32_t *property_counts = malloc((physical_device_count + 1) * sizeof *property_counts);
    if (!property_counts) {
        // TODO handle malloc
        goto fail_property_counts_alloc;
    }

    uint32_t total_property_count = 0;
    property_counts[0] = 0;
    for (size_t i = 0; i < physical_device_count; i++) {
        uint32_t property_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &property_count, 0);
        total_property_count += property_count;
        property_counts[i + 1] = total_property_count;
    }

    VkQueueFamilyProperties *queue_family_properties = malloc(total_property_count * sizeof *queue_family_properties);
    if (!queue_family_properties) {
        // TODO: handle queue_family_properties
        goto fail_queue_family_properties_alloc;
    }

    for (size_t i = 0; i < physical_device_count; i++) {
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &property_counts[i + 1], &queue_family_properties[property_counts[i]]);

        for (size_t j = 0; j < property_counts[i + 1]; j++) {
            if (queue_family_properties[property_counts[i] + j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                VkBool32 is_surface_supported = VK_FALSE;
                result = vkGetPhysicalDeviceSurfaceSupportKHR(physical_devices[i], j, surface, &is_surface_supported);
                assert(result == VK_SUCCESS);

                if (is_surface_supported == VK_TRUE) {
                    memcpy(&physical_device.gpu, &physical_devices[i], sizeof physical_device.gpu);
                    physical_device.graphics_family_index = j;
                    memcpy(
                        &physical_device.graphics_family_properties,
                        &queue_family_properties[property_counts[i] + j],
                        sizeof *queue_family_properties
                    );
                    goto break_physical_device_found;
                }
            }
        }
    }
  break_physical_device_found:
    // how to check if physical device was found

    free(queue_family_properties);
  fail_queue_family_properties_alloc:
    free(property_counts);
  fail_property_counts_alloc:
    free(physical_devices);
  fail_physical_devices_alloc:

    return physical_device;
}

VkDevice gfx_init_device(VkInstance instance, struct GfxPhysicalDevice const *const physical_device) {
    VkDevice device = 0;

    char const *extension_names[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkPhysicalDeviceFeatures features =  {
        .logicOp = VK_TRUE,
    };

    float *queue_priorities = calloc(physical_device->graphics_family_properties.queueCount, sizeof *queue_priorities);
    if (!queue_priorities) {
        goto fail_queue_prioities_alloc;
    }
    VkDeviceQueueCreateInfo device_queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = physical_device->graphics_family_index,
        .queueCount = physical_device->graphics_family_properties.queueCount,
        .pQueuePriorities = queue_priorities,
    };

    VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &device_queue_info,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = extension_names,
        .pEnabledFeatures = &features,
    };

    result = vkCreateDevice(physical_device->gpu, &device_info, 0, &device);
    assert(result == VK_SUCCESS);

    free(queue_priorities);
  fail_queue_prioities_alloc:

    return device;
}

VkSurfaceFormatKHR gfx_get_surface_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    VkSurfaceFormatKHR default_surface_format = {};

    uint32_t surface_formats_count = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_formats_count, 0);
    assert(result == VK_SUCCESS);
    VkSurfaceFormatKHR *surface_formats = malloc(surface_formats_count * sizeof *surface_formats);
    if (!surface_formats) {
        goto fail_surface_formats_alloc;
    }
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_formats_count, surface_formats);
    assert(result == VK_SUCCESS);

    default_surface_format = surface_formats[0];

    VkSurfaceFormatKHR preferred_format = {
        .format = VK_FORMAT_B8G8R8_UNORM,
        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    };

    if (surface_formats_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
        return preferred_format;
    }

    for (size_t i = 0; i < surface_formats_count; i++) {
        if (surface_formats[i].format == preferred_format.format && surface_formats[i].colorSpace == preferred_format.colorSpace) {
            return preferred_format;
        }
    }

    // TODO: how to free when returning surface_format[0] as default
    free(surface_formats);
  fail_surface_formats_alloc:

    return default_surface_format;
}

VkExtent2D gfx_get_extent(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface,
    GLFWwindow *window
) {
    VkSurfaceCapabilitiesKHR surface_capabilities;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);
    assert(result == VK_SUCCESS);
    if (surface_capabilities.currentExtent.width != UINT32_MAX) {
        return surface_capabilities.currentExtent;
    } else {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);

        if (width < surface_capabilities.minImageExtent.width) {
            width = surface_capabilities.minImageExtent.width;
        } else if (width > surface_capabilities.maxImageExtent.width) {
            width = surface_capabilities.maxImageExtent.width;
        }

        if (height < surface_capabilities.minImageExtent.height) {
            height = surface_capabilities.minImageExtent.height;
        } else if (height > surface_capabilities.maxImageExtent.height) {
            height = surface_capabilities.maxImageExtent.height;
        }

        VkExtent2D extent = {
            .width = width,
            .height = height,
        };

        return extent;
    }
}

VkSwapchainKHR gfx_init_swapchain(
    VkDevice device,
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface,
    struct VkSurfaceFormatKHR surface_format,
    struct VkExtent2D extent
) {
    VkSwapchainKHR swapchain = 0;

    uint32_t present_modes_count = 0;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_modes_count, 0);
    assert(result == VK_SUCCESS);
    VkPresentModeKHR *present_modes = malloc(present_modes_count * sizeof *present_modes);
    if (!present_modes) {
        goto fail_present_modes_alloc;
    }
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_modes_count, present_modes);
    assert(result == VK_SUCCESS);

    VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (size_t i = 0; i < present_modes_count; i++) {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR || present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            swapchain_present_mode = present_modes[i];
            break;
        }
    }

    VkSurfaceCapabilitiesKHR surface_capabilities;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);
    assert(result == VK_SUCCESS);

    // TODO: supported composite alpha
    VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = surface_capabilities.minImageCount,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = surface_capabilities.currentTransform,
        .compositeAlpha = composite_alpha,
        .presentMode = swapchain_present_mode,
        .clipped = VK_TRUE,
    };

    result = vkCreateSwapchainKHR(device, &create_info, 0, &swapchain);
    assert(result == VK_SUCCESS);

    free(present_modes);
  fail_present_modes_alloc:

    return swapchain;
}

VkImage* gfx_init_swapchain_images(VkDevice device, VkSwapchainKHR swapchain, uint32_t *length)  {
    result = vkGetSwapchainImagesKHR(device, swapchain, length, 0);
    assert(result == VK_SUCCESS);
    VkImage *swapchain_images = malloc(*length * sizeof *swapchain_images);
    result = vkGetSwapchainImagesKHR(device, swapchain, length, swapchain_images);
    assert(result == VK_SUCCESS);

    return swapchain_images;
}

VkImageView* gfx_init_swapchain_image_views(
    VkDevice device,
    uint32_t length,
    VkImage const *const swapchain_images,
    struct VkSurfaceFormatKHR const *const surface_format
) {
    VkImageViewCreateInfo create_info =  {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = 0,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = surface_format->format,
        .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel =  0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
    };

    VkImageView *swapchain_image_views = malloc(length * sizeof *swapchain_image_views);
    if (!swapchain_image_views) {
        goto fail_image_views_alloc;
    }
    for (size_t i = 0; i < length; i++) {
        create_info.image = swapchain_images[i];
        VkImageView swapchain_image_view;
        result = vkCreateImageView(device, &create_info, 0, &swapchain_image_view);
        assert(result == VK_SUCCESS);

        swapchain_image_views[i] = swapchain_image_view;
    }

  fail_image_views_alloc:

    return swapchain_image_views;
}

VkDescriptorPool gfx_init_descriptor_pool(VkDevice device, uint32_t swapchain_length) {
    VkDescriptorPoolSize descriptor_pool_sizes[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = swapchain_length,
        }
    };

    VkDescriptorPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = swapchain_length,
        .poolSizeCount = sizeof descriptor_pool_sizes / sizeof descriptor_pool_sizes[0],
        .pPoolSizes = &descriptor_pool_sizes[0],
    };

    VkDescriptorPool pool;
    result = vkCreateDescriptorPool(device, &create_info, 0, &pool);
    assert(result == VK_SUCCESS);

    return pool;
}

VkDescriptorSetLayout gfx_init_descriptor_layout(VkDevice device) {
    VkDescriptorSetLayoutBinding ubo_layout_binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };

    VkDescriptorSetLayoutCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &ubo_layout_binding,
    };

    VkDescriptorSetLayout layout;
    result = vkCreateDescriptorSetLayout(device, &create_info, 0, &layout);
    assert(result == VK_SUCCESS);

    return layout;
}

VkPipelineLayout gfx_init_pipeline_layout(VkDevice device, VkDescriptorSetLayout descriptor_layout) {
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptor_layout,
        .pushConstantRangeCount = 0,
    };

    VkPipelineLayout layout;
    result = vkCreatePipelineLayout(device, &pipeline_layout_create_info, 0, &layout);
    assert(result == VK_SUCCESS);

    return layout;
}

VkRenderPass gfx_init_render_pass(VkDevice device, VkFormat format) {
    VkAttachmentDescription color_attachment_descriptions[] = {
        {
            .format = format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        }
    };

    VkAttachmentReference color_attachment_refs[] = {
        {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        }
    };

    VkSubpassDescription subpasses[] = {
         {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = sizeof color_attachment_refs / sizeof color_attachment_refs[0],
            .pColorAttachments = color_attachment_refs,
        },
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = 0,
    };

    VkRenderPassCreateInfo create_info =  {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = sizeof color_attachment_descriptions / sizeof color_attachment_descriptions[0],
        .pAttachments = color_attachment_descriptions,
        .subpassCount = sizeof subpasses / sizeof *subpasses,
        .pSubpasses = subpasses,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    VkRenderPass render_pass;
    result = vkCreateRenderPass(device, &create_info, 0, &render_pass);
    assert(result == VK_SUCCESS);

    return render_pass;
}

struct GfxResource gfx_init_vertices_resource(VkDevice device, VkPhysicalDevice physical_device, uint32_t size) {
    struct GfxResource resource = {};

    struct VkBufferCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    result = vkCreateBuffer(device, &create_info, 0, &resource.buffer);
    assert(result == VK_SUCCESS);

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device, resource.buffer, &memory_requirements);
    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = gfx_get_memory_type(
            physical_device,
            memory_requirements.memoryTypeBits, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        ),
    };

    result = vkAllocateMemory(device, &alloc_info, 0, &resource.memory);
    assert(result == VK_SUCCESS);

    vkBindBufferMemory(device, resource.buffer, resource.memory, 0);

    return resource;
}

struct GfxResource* gfx_init_uniform_resources(VkDevice device, VkPhysicalDevice physical_device, uint32_t length) {
    struct GfxResource *resources = malloc(length * sizeof *resources);
    for (size_t i = 0; i < length; i++) {
        VkBufferCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof (struct UBO),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        result = vkCreateBuffer(device, &create_info, 0, &resources[i].buffer);
        assert(result == VK_SUCCESS);

        VkMemoryRequirements memory_requirements;
        vkGetBufferMemoryRequirements(device, resources[i].buffer, &memory_requirements);
        VkMemoryAllocateInfo buffer_alloc_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memory_requirements.size,
            .memoryTypeIndex = gfx_get_memory_type(
                physical_device,
                memory_requirements.memoryTypeBits, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            ),
        };

        result = vkAllocateMemory(device, &buffer_alloc_info, 0, &resources[i].memory);
        assert(result == VK_SUCCESS);

        vkBindBufferMemory(device, resources[i].buffer, resources[i].memory, 0);
    }

    return resources;
}

VkDescriptorSet* gfx_init_descriptor_sets(
    VkDevice device,
    uint32_t length,
    VkDescriptorSetLayout descriptor_layout,
    VkDescriptorPool descriptor_pool,
    struct GfxResource *uniform_resources
) {
    VkDescriptorSet *descriptor_sets = malloc(length * sizeof *descriptor_sets);
    VkDescriptorSetLayout *layouts = malloc(length * sizeof *layouts);
    if (!layouts) {
        goto fail_layouts_alloc;
    }
    for (size_t i = 0; i < length; i++) {
        layouts[i] = descriptor_layout;
    }

    VkDescriptorSetAllocateInfo descriptor_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptor_pool,
        .descriptorSetCount = length,
        .pSetLayouts = layouts,
    };

    result = vkAllocateDescriptorSets(device, &descriptor_alloc_info, descriptor_sets);
    assert(result == VK_SUCCESS);

    for (size_t i = 0; i < length; i++) {
        VkDescriptorBufferInfo buffer_info = {
            .buffer = uniform_resources[i].buffer,
            .offset = 0,
            .range = VK_WHOLE_SIZE,
        };

        VkWriteDescriptorSet descriptor_write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_sets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &buffer_info,
        };

        vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, 0);
    }

    free(layouts);
  fail_layouts_alloc:

    return descriptor_sets;
}

VkPipeline gfx_init_pipeline(
    VkDevice device,
    struct VkExtent2D extent,
    VkPipelineLayout pipeline_layout,
    VkRenderPass render_pass
) {
    // TODO change cwd() to install path
    uint32_t vert_shader_code_size = 0; 
    uint32_t *vert_shader_code = 0;
    gfx_io_read_spirv("build-debug/shader.vert.spv", &vert_shader_code_size, &vert_shader_code);
    uint32_t frag_shader_code_size = 0; 
    uint32_t *frag_shader_code = 0;
    gfx_io_read_spirv("build-debug/shader.frag.spv", &frag_shader_code_size, &frag_shader_code);
    VkShaderModule vert_shader_module = gfx_init_shader_module(device, vert_shader_code_size, vert_shader_code);
    VkShaderModule frag_shader_module = gfx_init_shader_module(device, frag_shader_code_size, frag_shader_code);

    VkPipelineShaderStageCreateInfo vert_shader_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vert_shader_module,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo frag_shader_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = frag_shader_module,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };

    VkVertexInputBindingDescription binding_description = {
        .binding = 0,
        .stride = sizeof(struct Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    VkVertexInputAttributeDescription attribute_descriptions[] = {
         {
            .binding = 0,
            .location = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(struct Vertex, pos),
        },
    };

    VkPipelineVertexInputStateCreateInfo vertex_input = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &binding_description,
        .vertexAttributeDescriptionCount = 1,
        .pVertexAttributeDescriptions = &attribute_descriptions[0],
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkViewport viewports[] = {
        {
            .x = 0.0,
            .y = 0.0,
            .width = extent.width,
            .height = extent.height,
            .minDepth = 0.0,
            .maxDepth = 1.0,
        },
    };
    
    VkRect2D scissors[] = {
        {
            .offset.x = 0.0,
            .offset.y = 0.0,
            .extent = extent,
        },
    };

    VkPipelineViewportStateCreateInfo viewport = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = sizeof viewports / sizeof viewports[0],
        .pViewports = &viewports[0],
        .scissorCount = sizeof scissors / sizeof scissors[0],
        .pScissors = &scissors[0],
    };

    VkPipelineRasterizationStateCreateInfo rasterization = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0,
        .depthBiasClamp = 0.0,
        .depthBiasSlopeFactor = 0.0,
        .lineWidth = 1.0,
    };

    VkPipelineMultisampleStateCreateInfo multisample = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0.0,
        .pSampleMask = 0,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };
    
    // TODO: depth
    
    VkPipelineColorBlendAttachmentState color_blend_attachments[] = {
         {
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            .blendEnable = VK_FALSE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
        },
    };

    VkPipelineColorBlendStateCreateInfo color_blend = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = sizeof color_blend_attachments / sizeof color_blend_attachments[0],
        .pAttachments = &color_blend_attachments[0],
        .blendConstants = { 0.0, 0.0, 0.0, 0.0 },
    };

    // TODO: dynamic state

    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = sizeof shader_stages / sizeof shader_stages[0],
        .pStages = &shader_stages[0],
        .pVertexInputState = &vertex_input,
        .pInputAssemblyState = &input_assembly,
        .pTessellationState = 0,
        .pViewportState = &viewport,
        .pRasterizationState = &rasterization,
        .pMultisampleState = &multisample,
        .pDepthStencilState = 0,
        .pColorBlendState = &color_blend,
        .pDynamicState = 0,
        .layout = pipeline_layout,
        .renderPass = render_pass,
        .subpass = 0,
        .basePipelineHandle = 0,
        .basePipelineIndex = -1,
    };


    VkPipeline pipeline;    
    result = vkCreateGraphicsPipelines(device, 0, 1, &graphics_pipeline_create_info, 0, &pipeline);
    assert(result == VK_SUCCESS);

    free(vert_shader_code);
    free(frag_shader_code);
    vkDestroyShaderModule(device, vert_shader_module, 0);
    vkDestroyShaderModule(device, frag_shader_module, 0);

    return pipeline;
}

VkShaderModule gfx_init_shader_module(VkDevice device, uint32_t size, uint32_t const *code) {
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = code,
    };
    
    VkShaderModule shader_module;
    result = vkCreateShaderModule(device, &create_info, 0, &shader_module);
    assert(result == VK_SUCCESS);

    return shader_module;
}

VkFramebuffer* gfx_init_framebuffers(
    VkDevice device,
    uint32_t image_view_length,
    VkImageView *image_views,
    VkRenderPass render_pass,
    VkExtent2D extent
) {
    VkFramebuffer *framebuffers = malloc(sizeof *framebuffers * image_view_length);

    VkFramebufferCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = render_pass,
        .width = extent.width,
        .height = extent.height,
        .layers = 1,
    };
    
    for (size_t i = 0; i < image_view_length; i++) {
        VkImageView attachments[1] = {
            image_views[i]
        };

        create_info.attachmentCount = sizeof attachments / sizeof attachments[0];
        create_info.pAttachments = attachments;

        result = vkCreateFramebuffer(device, &create_info, 0, &framebuffers[i]);
        assert(result == VK_SUCCESS);
    }

    return framebuffers;
}

VkCommandBuffer* gfx_init_command_buffers(
    VkDevice device,
    uint32_t length,
    VkCommandPool command_pool
) {
    VkCommandBuffer *command_buffers = malloc(sizeof *command_buffers * length);
    VkCommandBufferAllocateInfo command_buffer_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = length,
    };
    result = vkAllocateCommandBuffers(device, &command_buffer_info, command_buffers);
    assert(result == VK_SUCCESS);

    return command_buffers;
}

void gfx_record_command_buffers(
    uint32_t length,
    VkCommandBuffer *command_buffers,
    VkRenderPass render_pass,
    VkFramebuffer *framebuffers,
    VkPipeline pipeline,
    VkPipelineLayout pipeline_layout,
    VkBuffer vertex_buffer,
    VkDescriptorSet *descriptor_sets,
    VkExtent2D extent
) {
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    
    VkClearValue clear_color = {
        .color = {
            .float32 = {0.0f, 0.0f, 0.0f, 1.0f}
        }
    };

    for (size_t i = 0; i < length; i++) {
        result = vkBeginCommandBuffer(command_buffers[i], &begin_info);
        assert(result == VK_SUCCESS);

        VkRenderPassBeginInfo render_pass_begin_info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = render_pass,
            .framebuffer = framebuffers[i],
            .renderArea = {
                .offset = { 0.0f, 0.0f },
                .extent = extent,
            },
            .clearValueCount = 1,
            .pClearValues = &clear_color,
        };

        VkDeviceSize vertex_buffer_offsets = 0.0f;
        
        vkCmdBeginRenderPass(command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindVertexBuffers(command_buffers[i], 0, 1, &vertex_buffer, &vertex_buffer_offsets);
        vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets[i], 0, 0);
        vkCmdDraw(command_buffers[i], 3, 1, 0, 0);
        vkCmdEndRenderPass(command_buffers[i]);

        result = vkEndCommandBuffer(command_buffers[i]);
        assert(result == VK_SUCCESS);
    }   
}

void gfx_update_uniform_buffers(VkDevice device, VkDeviceMemory memory, struct UBO *ubo) {
    void *data;
    vkMapMemory(device, memory, 0, sizeof *ubo, 0, &data);
    memcpy(data, ubo, sizeof *ubo);
    vkUnmapMemory(device, memory);
}

