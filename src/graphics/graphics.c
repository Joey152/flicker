#include <volk/volk.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "graphics/graphics.h"
#include "graphics/io.h"
#include "graphics/vertex.h"
#include "window/window.h"

#define MAX_FRAMES_IN_FLIGHT 2

/* Private Structures */
struct GfxPhysicalDevice {
    VkPhysicalDevice gpu;
    uint32_t graphics_family_index; 
    VkQueueFamilyProperties graphics_family_properties;
};

struct GfxResource {
    VkBuffer buffer;
    VkDeviceMemory memory;
};

/* Private Data */
static VkResult result;
static VkInstance instance;
static VkSurfaceKHR surface;
static struct GfxPhysicalDevice physical_device;
static VkDevice device;
static VkQueue graphics_queue;
static VkSurfaceFormatKHR surface_format;
static VkExtent2D extent;
static VkSwapchainKHR swapchain;
static uint32_t swapchain_length;
static VkImage *swapchain_images;
static VkImageView *swapchain_image_views;
static VkCommandPool graphics_command_pool;
static VkDescriptorPool descriptor_pool;
static VkSemaphore *is_image_available_semaphore;
static VkSemaphore *is_present_ready_semaphore;
static VkFence *is_main_render_done;
static VkDescriptorSetLayout descriptor_layout;
static VkPipelineLayout pipeline_layout;
static VkRenderPass render_pass;
static VkBuffer vertex_buffer;
static VkDeviceMemory vertex_memory;
static struct GfxResource *uniform_resources;
static VkDescriptorSet *descriptor_sets;
static VkImage depth_image;
static VkDeviceMemory depth_image_memory;
static VkImageView depth_image_view;
static VkPipeline pipeline;
static VkFramebuffer *framebuffers;
static VkCommandBuffer *command_buffers;

/* Private Function Declarations */
static void
init_instance(VkInstance *instance);

static void
init_physical_device(
    VkInstance const instance,
    struct GfxPhysicalDevice *physical_device);

static void
init_surface(
    VkInstance const instance,
    VkSurfaceKHR *surface);

static void
init_device(
    struct GfxPhysicalDevice const *physical_device,
    VkDevice *device);

static void
get_surface_format(
    VkPhysicalDevice const physical_device,
    VkSurfaceKHR const surface,
    VkSurfaceFormatKHR *surface_format);

static void
get_extent(
    VkPhysicalDevice const physical_device,
    VkSurfaceKHR const surface,
    VkExtent2D *extent);

static void
init_swapchain(
    VkDevice const device,
    VkPhysicalDevice const physical_device,
    VkSurfaceKHR const surface,
    struct VkSurfaceFormatKHR const surface_format,
    struct VkExtent2D const extent,
    VkSwapchainKHR *swapchain);

static void
init_swapchain_image_views(
    VkDevice const device,
    struct VkSurfaceFormatKHR const *const surface_format,
    uint32_t const length,
    VkImage const swapchain_images[static const length],
    VkImageView swapchain_image_views[static const length]);

static void
init_descriptor_pool(
    VkDevice const device,
    uint32_t const swapchain_length,
    VkDescriptorPool *descriptor_pool); 

static void
init_descriptor_layout(VkDevice const device, VkDescriptorSetLayout *descriptor_layout);

static void
init_pipeline_layout(
    VkDevice const device,
    VkDescriptorSetLayout const descriptor_layout,
    VkPipelineLayout *pipeline_layout);

static void
init_render_pass(
    VkPhysicalDevice const physical_device,
    VkDevice const device,
    VkFormat const format, 
    VkRenderPass *render_pass);

static uint32_t
get_memory_type(
    VkPhysicalDevice const physical_device,
    uint32_t const type_filter,
    VkMemoryPropertyFlags const flags);

static void
init_uniform_resources(
    VkDevice const device,
    VkPhysicalDevice const physical_device,
    uint32_t const length,
    struct GfxResource resources[static const length]);

static void
init_descriptor_sets(
    VkDevice const device,
    VkDescriptorSetLayout descriptor_layout,
    VkDescriptorPool descriptor_pool,
    uint32_t const length,
    struct GfxResource const uniform_resources[static const length],
    VkDescriptorSet descriptor_sets[static const length]);

static void
init_with_extent(void);

static void
deinit_with_extent(void);

static void
reinit_swapchain(void);

static void
init_pipeline(
    VkDevice const device,
    struct VkExtent2D extent,
    VkPipelineLayout const pipeline_layout,
    VkRenderPass const render_pass,
    VkPipeline *pipeline);

static void
init_shader_module(
    VkDevice const device,
    uint32_t const size,
    uint32_t const *code,
    VkShaderModule *shader_module);

static void
init_framebuffers(
    VkDevice const device,
    VkExtent2D const extent,
    VkImageView const depth_image_view,
    VkRenderPass const render_pass,
    uint32_t const image_view_length,
    VkImageView const image_views[static const image_view_length],
    VkFramebuffer framebuffers[static const image_view_length]);

static void
init_command_buffers(
    VkDevice const device,
    VkCommandPool const command_pool,
    uint32_t const length,
    VkCommandBuffer command_buffers[static const length]);

static void 
record_command_buffers(
    uint32_t const length,
    VkCommandBuffer const command_buffers[static const length],
    VkFramebuffer const framebuffers[static const length],
    VkDescriptorSet const descriptor_sets[static const length],
    VkRenderPass const render_pass,
    VkPipeline const pipeline,
    VkPipelineLayout const pipeline_layout,
    VkBuffer const vertex_buffer,
    VkExtent2D const extent);

static void 
update_uniform_buffers(
    VkDevice const device,
    VkDeviceMemory const memory,
    struct UBO const *ubo);

/* Private Functions */
static void
init_instance(VkInstance *instance)
{
    char const *extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef _WIN32
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#else
#error Unsupported system
#endif
    };

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Flicker",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Flicker",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_1,
    };

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = sizeof extensions / sizeof *extensions,
        .ppEnabledExtensionNames = extensions,
    };

    result = vkCreateInstance(&create_info, 0, instance);
    assert(result == VK_SUCCESS);
}

static void
init_physical_device(
    VkInstance const instance,
    struct GfxPhysicalDevice *physical_device)
{
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
                    memcpy(&physical_device->gpu, &physical_devices[i], sizeof physical_device->gpu);
                    physical_device->graphics_family_index = j;
                    memcpy(
                        &physical_device->graphics_family_properties,
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
    ;
}

static void
init_surface(
    VkInstance const instance,
    VkSurfaceKHR *surface)
{
    result = window.create_surface(instance, surface);
    assert(result == VK_SUCCESS);
}

static void
init_device(
    struct GfxPhysicalDevice const *physical_device,
    VkDevice *device)
{
    float *queue_priorities = calloc(physical_device->graphics_family_properties.queueCount, sizeof *queue_priorities);
    if (queue_priorities) {
        // TODO: malloc fails
    }

    VkDeviceQueueCreateInfo queue_create_info[] = {
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = physical_device->graphics_family_index,
            .queueCount = physical_device->graphics_family_properties.queueCount,
            .pQueuePriorities = queue_priorities,
        }
    };

    char const *extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = sizeof queue_create_info / sizeof *queue_create_info,
        .pQueueCreateInfos = queue_create_info,
        .enabledExtensionCount = sizeof extensions / sizeof *extensions,
        .ppEnabledExtensionNames = extensions,
    };

    result = vkCreateDevice(physical_device->gpu, &device_create_info, 0, device);
    assert(result == VK_SUCCESS);

    free(queue_priorities);
}

static void
get_surface_format(
    VkPhysicalDevice const physical_device,
    VkSurfaceKHR const surface,
    VkSurfaceFormatKHR *surface_format)
{
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
        *surface_format = preferred_format;
        return;
    }

    for (size_t i = 0; i < surface_formats_count; i++) {
        if (surface_formats[i].format == preferred_format.format && surface_formats[i].colorSpace == preferred_format.colorSpace) {
            *surface_format = preferred_format;
            return;
        }
    }

    // TODO: how to free when returning surface_format[0] as default
    free(surface_formats);
  fail_surface_formats_alloc:

    *surface_format = default_surface_format;
}

static void
get_extent(
    VkPhysicalDevice const physical_device,
    VkSurfaceKHR const surface,
    VkExtent2D *extent)
{
    VkSurfaceCapabilitiesKHR surface_capabilities;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);
    assert(result == VK_SUCCESS);
    if (surface_capabilities.currentExtent.width != UINT32_MAX) {
        *extent = surface_capabilities.currentExtent;
    } else {
        int width = 0;
        int height = 0;
        window.get_window_size(&width, &height);
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

        VkExtent2D ext = {
            .width = width,
            .height = height,
        };

        *extent = ext;
    }
}

static void
init_swapchain(
    VkDevice const device,
    VkPhysicalDevice const physical_device,
    VkSurfaceKHR const surface,
    struct VkSurfaceFormatKHR const surface_format,
    struct VkExtent2D const extent,
    VkSwapchainKHR *swapchain)
{
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

    result = vkCreateSwapchainKHR(device, &create_info, 0, swapchain);
    assert(result == VK_SUCCESS);

    free(present_modes);
  fail_present_modes_alloc: ;
}

static void
init_swapchain_images(
    VkDevice const device,
    VkSwapchainKHR const swapchain,
    uint32_t *length,
    VkImage **swapchain_images)
{
    result = vkGetSwapchainImagesKHR(device, swapchain, length, 0);
    assert(result == VK_SUCCESS);
    *swapchain_images = malloc(*length * sizeof **swapchain_images);
    result = vkGetSwapchainImagesKHR(device, swapchain, length, *swapchain_images);
    assert(result == VK_SUCCESS);
}

static void
init_swapchain_image_views(
    VkDevice const device,
    struct VkSurfaceFormatKHR const *const surface_format,
    uint32_t const length,
    VkImage const swapchain_images[static const length],
    VkImageView swapchain_image_views[static const length])
{
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

    if (!swapchain_image_views)
    {
        goto fail_image_views_alloc;
    }
    for (size_t i = 0; i < length; i++)
    {
        create_info.image = swapchain_images[i];
        VkImageView swapchain_image_view;
        result = vkCreateImageView(device, &create_info, 0, &swapchain_image_view);
        assert(result == VK_SUCCESS);

        swapchain_image_views[i] = swapchain_image_view;
    }

  fail_image_views_alloc: ;
}

static void
init_descriptor_pool(
    VkDevice const device,
    uint32_t const swapchain_length,
    VkDescriptorPool *descriptor_pool)
{
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

    result = vkCreateDescriptorPool(device, &create_info, 0, descriptor_pool);
    assert(result == VK_SUCCESS);
}

static void
init_descriptor_layout(VkDevice const device, VkDescriptorSetLayout *descriptor_layout)
{
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
    result = vkCreateDescriptorSetLayout(device, &create_info, 0, descriptor_layout);
    assert(result == VK_SUCCESS);
}

static void
init_pipeline_layout(
    VkDevice const device,
    VkDescriptorSetLayout const descriptor_layout,
    VkPipelineLayout *pipeline_layout)
{
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptor_layout,
        .pushConstantRangeCount = 0,
    };

    result = vkCreatePipelineLayout(device, &pipeline_layout_create_info, 0, pipeline_layout);
    assert(result == VK_SUCCESS);
}

static void
init_render_pass(
    VkPhysicalDevice const physical_device,
    VkDevice const device,
    VkFormat const format, 
    VkRenderPass *render_pass)
{
    VkFormat depth_formats[3] = {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    VkFormat depth_format = VK_FORMAT_UNDEFINED;
    for (size_t i = 0; i < 3; i++) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_device, depth_formats[i], &props);

        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            depth_format = depth_formats[i];
            break;
        }
    }
    assert(depth_format != VK_FORMAT_UNDEFINED); 

    VkAttachmentDescription attachment_descriptions[] = {
        {
            .format = format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
        {
            .format = depth_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        }
    };

    VkAttachmentReference color_attachment_refs[] = {
        {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        }
    };

    VkAttachmentReference depth_attachment_ref = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpasses[] = {
         {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = sizeof color_attachment_refs / sizeof color_attachment_refs[0],
            .pColorAttachments = color_attachment_refs,
            .pDepthStencilAttachment = &depth_attachment_ref
        },
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = 0,
    };

    VkRenderPassCreateInfo create_info =  {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = sizeof attachment_descriptions / sizeof attachment_descriptions[0],
        .pAttachments = attachment_descriptions,
        .subpassCount = sizeof subpasses / sizeof *subpasses,
        .pSubpasses = subpasses,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    result = vkCreateRenderPass(device, &create_info, 0, render_pass);
    assert(result == VK_SUCCESS);
}

static uint32_t
get_memory_type(
    VkPhysicalDevice const physical_device,
    uint32_t const type_filter,
    VkMemoryPropertyFlags const flags)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties); 

    for (size_t i = 0; i < memory_properties.memoryTypeCount; i++)
    {
        uint32_t is_type_filter_present = type_filter & (1 << i);
        uint32_t is_flags_present = (memory_properties.memoryTypes[i].propertyFlags & flags) == flags;
        if (is_type_filter_present && is_flags_present)
        {
            return i;
        }
    }

    assert(0);
}

static void
init_uniform_resources(
    VkDevice const device,
    VkPhysicalDevice const physical_device,
    uint32_t const length,
    struct GfxResource resources[static const length])
{
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
            .memoryTypeIndex = get_memory_type(
                physical_device,
                memory_requirements.memoryTypeBits, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            ),
        };

        result = vkAllocateMemory(device, &buffer_alloc_info, 0, &resources[i].memory);
        assert(result == VK_SUCCESS);

        vkBindBufferMemory(device, resources[i].buffer, resources[i].memory, 0);
    }
}

static void
init_descriptor_sets(
    VkDevice const device,
    VkDescriptorSetLayout descriptor_layout,
    VkDescriptorPool descriptor_pool,
    uint32_t const length,
    struct GfxResource const uniform_resources[static const length],
    VkDescriptorSet descriptor_sets[static const length])
{
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
  fail_layouts_alloc:;
}

static void
init_pipeline(
    VkDevice const device,
    struct VkExtent2D extent,
    VkPipelineLayout const pipeline_layout,
    VkRenderPass const render_pass,
    VkPipeline *pipeline)
{
    // TODO change cwd() to install path
    uint32_t vert_shader_code_size = 0; 
    uint32_t *vert_shader_code = 0;
    io_read_spirv(".\\build\\vert.spv", &vert_shader_code_size, &vert_shader_code);
    uint32_t frag_shader_code_size = 0; 
    uint32_t *frag_shader_code = 0;
    io_read_spirv(".\\build\\frag.spv", &frag_shader_code_size, &frag_shader_code);
    VkShaderModule vert_shader_module;
    init_shader_module(device, vert_shader_code_size, vert_shader_code, &vert_shader_module);
    VkShaderModule frag_shader_module;
    init_shader_module(device, frag_shader_code_size, frag_shader_code, &frag_shader_module);

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
         // {
         //     .binding = 0,
         //     .location = 1,
         //     .format = VK_FORMAT_R32G32B32_SFLOAT,
         //     .offset = offsetof(struct Vertex, centroid),
         // }
    };

    VkPipelineVertexInputStateCreateInfo vertex_input = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &binding_description,
        .vertexAttributeDescriptionCount = sizeof attribute_descriptions / sizeof attribute_descriptions[0],
        .pVertexAttributeDescriptions = attribute_descriptions,
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

    VkPipelineDepthStencilStateCreateInfo depth_stencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };
    
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
        .pDepthStencilState = &depth_stencil,
        .pColorBlendState = &color_blend,
        .pDynamicState = 0,
        .layout = pipeline_layout,
        .renderPass = render_pass,
        .subpass = 0,
        .basePipelineHandle = 0,
        .basePipelineIndex = -1,
    };

    result = vkCreateGraphicsPipelines(device, 0, 1, &graphics_pipeline_create_info, 0, pipeline);
    assert(result == VK_SUCCESS);

#ifdef _WIN32
    _aligned_free(vert_shader_code);
    _aligned_free(frag_shader_code);
#else
    free(vert_shader_code);
    free(frag_shader_code);
#endif
    vkDestroyShaderModule(device, vert_shader_module, 0);
    vkDestroyShaderModule(device, frag_shader_module, 0);
}

static void
init_shader_module(
    VkDevice const device,
    uint32_t const size,
    uint32_t const *code,
    VkShaderModule *shader_module)
{
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = code,
    };

    result = vkCreateShaderModule(device, &create_info, 0, shader_module);
    assert(result == VK_SUCCESS);
}

static void
init_framebuffers(
    VkDevice const device,
    VkExtent2D const extent,
    VkImageView const depth_image_view,
    VkRenderPass const render_pass,
    uint32_t const image_view_length,
    VkImageView const image_views[static const image_view_length],
    VkFramebuffer framebuffers[static const image_view_length])
{
    VkFramebufferCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = render_pass,
        .width = extent.width,
        .height = extent.height,
        .layers = 1,
    };
    
    for (size_t i = 0; i < image_view_length; i++)
    {
        VkImageView attachments[2] = {
            image_views[i],
            depth_image_view
        };

        create_info.attachmentCount = sizeof attachments / sizeof attachments[0];
        create_info.pAttachments = attachments;

        result = vkCreateFramebuffer(device, &create_info, 0, &framebuffers[i]);
        assert(result == VK_SUCCESS);
    }
}

static void
init_command_buffers(
    VkDevice const device,
    VkCommandPool const command_pool,
    uint32_t const length,
    VkCommandBuffer command_buffers[static const length])
{
    VkCommandBufferAllocateInfo command_buffer_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = length,
    };
    result = vkAllocateCommandBuffers(device, &command_buffer_info, command_buffers);
    assert(result == VK_SUCCESS);
}

static void 
record_command_buffers(
    uint32_t const length,
    VkCommandBuffer const command_buffers[static const length],
    VkFramebuffer const framebuffers[static const length],
    VkDescriptorSet const descriptor_sets[static const length],
    VkRenderPass const render_pass,
    VkPipeline const pipeline,
    VkPipelineLayout const pipeline_layout,
    VkBuffer const vertex_buffer,
    VkExtent2D const extent)
{
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    
    VkClearValue clear_color[2] = {
        {
            .color = {
                .float32 = {0.0f, 0.0f, 0.0f, 1.0f}
            }
        },
        {
            .depthStencil = {1.0f, 0.0f}
        }
    };

    for (size_t i = 0; i < length; i++)
    {
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
            .clearValueCount = sizeof clear_color / sizeof clear_color[0],
            .pClearValues = clear_color,
        };

        VkDeviceSize offsets[1] = {0};

        vkCmdBeginRenderPass(command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindVertexBuffers(command_buffers[i], 0, 1, &vertex_buffer, offsets);
        vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets[i], 0, 0);
        vkCmdDraw(command_buffers[i], 3, 1, 0, 0);
        vkCmdEndRenderPass(command_buffers[i]);

        result = vkEndCommandBuffer(command_buffers[i]);
        assert(result == VK_SUCCESS);
    }   
}

static void
reinit_swapchain(void) 
{
    vkDeviceWaitIdle(device);

    for (size_t i = 0; i < swapchain_length; i++)
    {
        vkDestroyImageView(device, swapchain_image_views[i], 0);
    }
    vkDestroySwapchainKHR(device, swapchain, 0);
    get_extent(physical_device.gpu, surface, &extent);
    init_swapchain(device, physical_device.gpu, surface, surface_format, extent, &swapchain);
    init_swapchain_images(device, swapchain, &swapchain_length, &swapchain_images);
    init_swapchain_image_views(device, &surface_format, swapchain_length, swapchain_images, swapchain_image_views);

    deinit_with_extent();
    init_with_extent();
}

static void
init_with_extent(void)
{
    VkFormat depth_formats[3] = {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    VkFormat depth_format = VK_FORMAT_UNDEFINED;
    for (size_t i = 0; i < 3; i++)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_device.gpu, depth_formats[i], &props);

        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            depth_format = depth_formats[i];
            break;
        }
    }
    assert(depth_format != VK_FORMAT_UNDEFINED); 

    VkImageCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .extent.width = extent.width,
        .extent.height = extent.height,
        .extent.depth = 1,
        .mipLevels = 1,
        .arrayLayers = 1,
        .format = depth_format,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    result = vkCreateImage(device, &create_info, 0, &depth_image);
    assert(result == VK_SUCCESS);

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device, depth_image, &memory_requirements);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = get_memory_type(
            physical_device.gpu,
            memory_requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        ),
    };

    result = vkAllocateMemory(device, &alloc_info, 0, &depth_image_memory);
    assert(result == VK_SUCCESS);

    vkBindImageMemory(device, depth_image, depth_image_memory, 0);

    VkImageViewCreateInfo view_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = depth_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = depth_format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    result = vkCreateImageView(device, &view_create_info, 0, &depth_image_view);
    assert(result == VK_SUCCESS);

    init_pipeline(device, extent, pipeline_layout, render_pass, &pipeline);
    framebuffers = malloc(swapchain_length * sizeof *framebuffers);
    init_framebuffers(
        device,
        extent,
        depth_image_view,
        render_pass,
        swapchain_length,
        swapchain_image_views,
        framebuffers
    );
    command_buffers = malloc(swapchain_length * sizeof *command_buffers);
    init_command_buffers(device, graphics_command_pool, swapchain_length, command_buffers);
    record_command_buffers(
        swapchain_length,
        command_buffers,
        framebuffers,
        descriptor_sets,
        render_pass,
        pipeline,
        pipeline_layout,
        vertex_buffer,
        extent
    );
}

static void
deinit_with_extent(void)
{
    vkFreeCommandBuffers(device, graphics_command_pool, swapchain_length, command_buffers);
    free(command_buffers);
    for (size_t i = 0; i < swapchain_length; i++) {
        vkDestroyFramebuffer(device, framebuffers[i], 0);
    }
    free(framebuffers);
    vkDestroyPipeline(device, pipeline, 0);
    vkDestroyImageView(device, depth_image_view, 0);
    vkFreeMemory(device, depth_image_memory, 0);
    vkDestroyImage(device, depth_image, 0);
}

static void
update_uniform_buffers(
    VkDevice const device,
    VkDeviceMemory const memory,
    struct UBO const *ubo)
{
    void *data;
    vkMapMemory(device, memory, 0, sizeof *ubo, 0, &data);
    memcpy(data, ubo, sizeof *ubo);
    vkUnmapMemory(device, memory);
}


/* Public Functions */
void
init(void)
{
    result = volkInitialize();
    assert(result == VK_SUCCESS);

    init_instance(&instance);
    volkLoadInstance(instance);

    init_surface(instance, &surface);
    init_physical_device(instance, &physical_device);
    init_device(&physical_device, &device);
    volkLoadDevice(device);

    vkGetDeviceQueue(device, physical_device.graphics_family_index, 0, &graphics_queue);
    get_surface_format(physical_device.gpu, surface, &surface_format);
    get_extent(physical_device.gpu, surface, &extent);

    init_swapchain(device, physical_device.gpu, surface, surface_format, extent, &swapchain);
    result = vkGetSwapchainImagesKHR(device, swapchain, &swapchain_length, 0);
    assert(result == VK_SUCCESS);
    swapchain_images = malloc(swapchain_length * sizeof *swapchain_images);
    result = vkGetSwapchainImagesKHR(device, swapchain, &swapchain_length, swapchain_images);
    assert(result == VK_SUCCESS);
    swapchain_image_views = malloc(swapchain_length * sizeof *swapchain_image_views);
    init_swapchain_image_views(device, &surface_format, swapchain_length, swapchain_images, swapchain_image_views);

    VkCommandPoolCreateInfo graphics_command_pool_info =  {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = physical_device.graphics_family_index,
    };
    result = vkCreateCommandPool(device, &graphics_command_pool_info, 0, &graphics_command_pool);
    assert(result == VK_SUCCESS);

    init_descriptor_pool(device, swapchain_length, &descriptor_pool);

    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    is_image_available_semaphore = malloc(MAX_FRAMES_IN_FLIGHT * sizeof *is_image_available_semaphore);
    is_present_ready_semaphore = malloc(MAX_FRAMES_IN_FLIGHT * sizeof *is_present_ready_semaphore);
    is_main_render_done = malloc(MAX_FRAMES_IN_FLIGHT * sizeof *is_main_render_done);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        result = vkCreateSemaphore(device, &semaphore_info, 0, &is_image_available_semaphore[i]);
        assert(result == VK_SUCCESS);
        result = vkCreateSemaphore(device, &semaphore_info, 0, &is_present_ready_semaphore[i]);
        assert(result == VK_SUCCESS);
        result = vkCreateFence(device, &fence_info, 0, &is_main_render_done[i]);
        assert(result == VK_SUCCESS);
    }

    init_descriptor_layout(device, &descriptor_layout);
    init_pipeline_layout(device, descriptor_layout, &pipeline_layout);
    init_render_pass(physical_device.gpu, device, surface_format.format, &render_pass);

    struct Vertex vertices[] = {
        { .pos = { .x = 0.0f, .y = 1.0f,  .z = 1.0f } },
        { .pos = { .x = 0.0f, .y = -1.0f, .z = 1.0f } },
        { .pos = { .x = 1.0f, .y = 0.0f,  .z = 1.0f } },
    };

    VkBufferCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof vertices,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    result = vkCreateBuffer(device, &create_info, 0, &vertex_buffer);
    assert(result == VK_SUCCESS);

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device, vertex_buffer, &memory_requirements);
    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = get_memory_type(
            physical_device.gpu,
            memory_requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        ),
    };
    result = vkAllocateMemory(device, &alloc_info, 0, &vertex_memory);
    assert(result == VK_SUCCESS);

    void *data;
    vkBindBufferMemory(device, vertex_buffer, vertex_memory, 0);
    vkMapMemory(device, vertex_memory, 0, sizeof vertices, 0, &data);
    memcpy(data, &vertices, sizeof vertices);
    vkUnmapMemory(device, vertex_memory);

    // engine.vertex_buffers_length = number_of_objs;
    // engine.total_vertex_size = 0;
    // for (size_t i = 0; i < number_of_objs; i++) {
    //     engine.total_vertex_size += obj_sizes[i];
    // }

    // struct VkBufferCreateInfo create_info = {
    //     .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    //     .size = engine.total_vertex_size,
    //     .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    //     .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    // };
    // result = vkCreateBuffer(engine.device, &create_info, 0, &engine.vertex_buffer);
    // assert(result == VK_SUCCESS);

    // VkMemoryRequirements memory_requirements;
    // vkGetBufferMemoryRequirements(engine.device, engine.vertex_buffer, &memory_requirements);

    // VkMemoryAllocateInfo alloc_info = {
    //     .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    //     .allocationSize = memory_requirements.size,
    //     .memoryTypeIndex = gfx_get_memory_type(
    //         engine.physical_device.gpu,
    //         memory_requirements.memoryTypeBits,
    //         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    //     ),
    // };

    // result = vkAllocateMemory(engine.device, &alloc_info, 0, &engine.vertex_memory);
    // assert(result == VK_SUCCESS);

    // size_t current_offset = 0;
    // void *data;
    // vkBindBufferMemory(engine.device, engine.vertex_buffer, engine.vertex_memory, 0);
    // vkMapMemory(engine.device, engine.vertex_memory, 0, engine.total_vertex_size, 0, &data);
    // for (size_t i = 0; i < number_of_objs; i++) {

    //     FILE *file = fopen(path_to_obj_files[i], "rb");
    //     char *v = malloc(obj_sizes[i]);
    //     fseek(file, sizeof(uint32_t), SEEK_SET);
    //     fread(v, sizeof *v, obj_sizes[i], file);

    //     memcpy(&data[current_offset], v, obj_sizes[i]);

    //     current_offset = current_offset + obj_sizes[i];

    //     free(v);
    // }
    // vkUnmapMemory(engine.device, engine.vertex_memory);

    uniform_resources = malloc(swapchain_length * sizeof *uniform_resources);
    init_uniform_resources(device, physical_device.gpu, swapchain_length, uniform_resources);
    descriptor_sets = malloc(swapchain_length * sizeof *descriptor_sets);
    init_descriptor_sets(
        device,
        descriptor_layout,
        descriptor_pool,
        swapchain_length,
        uniform_resources,
        descriptor_sets
    );

    init_with_extent();
}

void
deinit(void)
{
    vkDeviceWaitIdle(device);

    deinit_with_extent();

    for (size_t i = 0; i < swapchain_length; i++)
    {
        vkFreeMemory(device, uniform_resources[i].memory, 0);
        vkDestroyBuffer(device, uniform_resources[i].buffer, 0);
    }
    free(uniform_resources);
    vkFreeMemory(device, vertex_memory, 0);
    vkDestroyBuffer(device, vertex_buffer, 0);
    vkDestroyRenderPass(device, render_pass, 0);
    vkDestroyPipelineLayout(device, pipeline_layout, 0);
    vkDestroyDescriptorSetLayout(device, descriptor_layout, 0);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(device, is_image_available_semaphore[i], 0);
        vkDestroySemaphore(device, is_present_ready_semaphore[i], 0);
        vkDestroyFence(device, is_main_render_done[i], 0);
    }
    vkDestroyDescriptorPool(device, descriptor_pool, 0);
    free(descriptor_sets);
    vkDestroyCommandPool(device, graphics_command_pool, 0);
    for (size_t i = 0; i < swapchain_length; i++)
    {
        vkDestroyImageView(device, swapchain_image_views[i], 0);
    }
    free(swapchain_image_views);
    vkDestroySwapchainKHR(device, swapchain, 0);
    free(swapchain_images);
    vkDestroyDevice(device, 0);
    vkDestroySurfaceKHR(instance, surface, 0);
    vkDestroyInstance(instance, 0);
}

void
draw_frame(struct UBO *ubo)
{
    static uint32_t current_frame = 0;

    result = vkWaitForFences(device, 1, &is_main_render_done[current_frame], VK_TRUE, UINT64_MAX);
    assert(result == VK_SUCCESS);

    result = vkResetFences(device, 1, &is_main_render_done[current_frame]);
    assert(result == VK_SUCCESS);

    uint32_t image_index;
    result = vkAcquireNextImageKHR(
        device,
        swapchain,
        UINT64_MAX,
        is_image_available_semaphore[current_frame],
        0,
        &image_index
    );
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        reinit_swapchain();
    }

    update_uniform_buffers(device, uniform_resources[current_frame].memory, ubo);

    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &is_image_available_semaphore[current_frame],
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffers[image_index],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &is_present_ready_semaphore[current_frame],
    };

    result = vkQueueSubmit(graphics_queue, 1, &submit_info, is_main_render_done[current_frame]);
    assert(result == VK_SUCCESS);

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &is_present_ready_semaphore[current_frame],
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &image_index,
    };

    result = vkQueuePresentKHR(graphics_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        reinit_swapchain();
    }

    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

/* Export Graphics Library */
const struct graphics graphics = {
    .init = init,
    .deinit = deinit,
    .draw_frame = draw_frame,
};

