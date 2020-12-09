project('Flicker', 'c')
add_project_arguments('-Dc_std=c17', language : 'c')

glslc = find_program('glslc')
custom_target('compile main shaders',
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
    command: [glslc, '-c', '@INPUT@']
)

cmake = import('cmake')
glfw_opt = cmake.subproject_options()
glfw_opt.append_compile_args('c', '-w')
glfw_opt.add_cmake_defines({
    'GLFW_BUILD_EXAMPLES': 'OFF',
    'GLFW_BUILD_TESTS': 'OFF',
    'GLFW_BUILD_DOCS': 'OFF'
})
glfw_proj = cmake.subproject('glfw-3.3.2', options: glfw_opt)
glfw_dep = glfw_proj.dependency('glfw')

vulkan_dep = dependency('vulkan')

inc = include_directories('include')

linmath_lib = static_library('linmath', 'src/common/linmath.c', include_directories: inc)

graphics_lib = static_library('graphics', 
    [
        'src/graphics/engine.c',
        'src/graphics/io.c',
        'src/graphics/resource.c',
        'src/graphics/vertex.c',
    ],
    dependencies: [vulkan_dep],
    include_directories: inc
)

executable('flicker', 'src/game/main.c',
    dependencies: [glfw_dep],
    include_directories: inc,
    link_with: [graphics_lib, linmath_lib],
    link_args: ['-ldl']
)
