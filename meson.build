project('Test', 'c')
add_project_arguments('-Dc_std=c17', language : 'c')


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

linmath_lib = static_library('linmath', 'src/common/linmath.c', include_directories: inc)

if host_machine.system() == 'windows'
    window_source = [ 'src/window/win32.c' ]
    vulkan_defines = '-DVK_USE_PLATFORM_WIN32_KHR'
else
    error('Unsupported system')
endif

volk_lib = static_library('volk',
    [ 'src/volk/volk.c' ],
    include_directories: inc,
    c_args: vulkan_defines
)

window_lib = static_library('window',
    window_source,
    link_with: [volk_lib],
    include_directories: inc,
    c_args: vulkan_defines
)

graphics_lib = static_library('graphics', 
    [
        'src/graphics/graphics.c',
        'src/graphics/io.c',
    ],
    dependencies: [],
    link_with: [window_lib, volk_lib],
    include_directories: inc,
    c_args: vulkan_defines
)

executable('flicker', 'src/game/main.c',
    dependencies: [],
    link_with: [graphics_lib, window_lib, linmath_lib],
    include_directories: inc,
)

