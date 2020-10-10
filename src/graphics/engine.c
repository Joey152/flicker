#include "graphics/engine.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Private Declarations
VkInstance gfx_init_instance(void);
VkSurfaceKHR gfx_init_surface(VkInstance instance, GLFWwindow *window);
struct GfxPhysicalDevice gfx_init_physical_device(VkInstance instance, VkSurfaceKHR surface);
VkDevice gfx_init_device(VkInstance instance, struct GfxPhysicalDevice *physical_device);
VkSurfaceFormatKHR gfx_init_surface_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
VkExtent2D gfx_get_extent(VkPhysicalDevice physical_device, VkSurfaceKHR surface, GLFWwindow *window);

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
    VkExtent2D extent;
};

// Global Variables
static struct GfxEngine engine = {};
static VkResult result = VK_SUCCESS;

int gfx_init(GLFWwindow *window) {
    engine.instance = gfx_init_instance();
    engine.surface = gfx_init_surface(engine.instance, window);
    engine.physical_device = gfx_init_physical_device(engine.instance, engine.surface);
    engine.device = gfx_init_device(engine.instance, &engine.physical_device);
    vkGetDeviceQueue(engine.device, engine.physical_device.graphics_family_index, 0, &engine.graphics_queue);
    engine.surface_format = gfx_init_surface_format(engine.physical_device.gpu, engine.surface);
    engine.extent = gfx_get_extent(engine.physical_device.gpu, engine.surface, window);

    return 1;
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

VkDevice gfx_init_device(VkInstance instance, struct GfxPhysicalDevice *physical_device) {
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

VkSurfaceFormatKHR gfx_init_surface_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    uint32_t surface_formats_count = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_formats_count, 0);
    assert(result == VK_SUCCESS);
    VkSurfaceFormatKHR *surface_formats = malloc(surface_formats_count * sizeof *surface_formats);
    if (!surface_formats) {
        goto fail_surface_formats_alloc;
    }
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_formats_count, surface_formats);
    assert(result == VK_SUCCESS);

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
  fail_surface_formats_alloc:
  
    return surface_formats[0];
}

VkExtent2D gfx_get_extent(VkPhysicalDevice physical_device, VkSurfaceKHR surface, GLFWwindow *window) {
    VkSurfaceCapabilitiesKHR surface_capabilities = {};
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
