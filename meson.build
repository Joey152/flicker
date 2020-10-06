project('Flicker', 'c')

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

executable('flicker', 'src/game/main.c',
    dependencies: [glfw_dep],
    link_args: ['-ldl']
)
