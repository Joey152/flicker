project('Flicker', 'c', default_options: ['c_std=c17'])

cc = meson.get_compiler('c')
libdl_dep = cc.find_library('dl')
libm_dep = cc.find_library('m')

glslangValidator = find_program('glslangValidator')
custom_target('main shaders',
    install: true,
    install_dir: 'asset/shader/main',
    input: files(
        'asset/shader/main/shader.vert',
        'asset/shader/main/shader.frag',
    ),
    output: [
        'vert.spv',
        'frag.spv',
    ],
    command: [glslangValidator, '--target-env', 'vulkan1.0',  '@INPUT@']
)

inc = include_directories('include')

linmath_lib = static_library(
    'linmath',
    'src/common/linmath.c',
    dependencies: [libm_dep],
    include_directories: inc,
    c_args: [ '-lm' ]
)

if host_machine.system() == 'windows'
    platform_source = ['src/platform/win32.c']
    vulkan_defines = '-DVK_USE_PLATFORM_WIN32_KHR'
    platform_links = ''
elif host_machine.system() == 'linux'
    # TODO: how to detect X11 vs wayland
    platform_source = ['src/platform/xcb.c']
    libxcb_dep = cc.find_library('xcb')
    platform_deps = [libxcb_dep]
    platform_links = ['-D_POSIX_C_SOURCE=199309L']
    vulkan_defines = '-DVK_USE_PLATFORM_XCB_KHR'
else
    error('Unsupported system')
endif

volk_lib = static_library('volk',
    ['src/volk/volk.c'],
    dependencies: [libdl_dep],
    include_directories: inc,
    c_args: vulkan_defines
)

platform_lib = static_library('platform',
    platform_source,
    dependencies: platform_deps,
    link_with: [volk_lib],
    include_directories: inc,
    c_args: [vulkan_defines, platform_links]
)

graphics_lib = static_library('graphics',
    [
        'src/graphics/graphics.c',
        'src/graphics/io.c',
    ],
    dependencies: [],
    link_with: [platform_lib, volk_lib],
    include_directories: inc,
    c_args: vulkan_defines
)

executable('flicker', 'src/game/main.c',
    dependencies: [],
    link_with: [graphics_lib, platform_lib, linmath_lib],
    include_directories: inc,
    c_args: ['-g'],
)

