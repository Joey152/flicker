#include "graphics/engine.h"

#include <assert.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <stdlib.h>
#include <string.h>

// Private Declarations
VkInstance gfx_init_instance(void);

// Global Variables
static struct gfx_engine engine = {};
static VkResult result = VK_SUCCESS;

int gfx_init(struct gfx_engine e[static 1]) {
    engine.instance = gfx_init_instance();
    return 1;
}

VkInstance gfx_init_instance(void) {
    char const * layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    uint32_t glfw_extension_count = 0; 
    char const ** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    char const * other_extensions[] = {
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME
    };

    unsigned int extensions_count = glfw_extension_count + sizeof other_extensions / sizeof other_extensions[0];
    char ** extensions = malloc(extensions_count * sizeof *extensions);
    if (!extensions) {
        // TODO: handle malloc
    }

    // TODO: handle errors?
    memcpy(&extensions, &glfw_extensions, glfw_extension_count * sizeof glfw_extensions);
    memcpy(&extensions[glfw_extension_count], &other_extensions, sizeof other_extensions);

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

    VkInstance instance = 0;
    result = vkCreateInstance(&create_info, 0, &instance);
    assert(result == VK_SUCCESS);

    return instance;
}

