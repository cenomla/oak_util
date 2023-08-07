project('oak_util', 'cpp', default_options : ['cpp_std=c++17', 'warning_level=3'])

cxx = meson.get_compiler('cpp')

deps = [
	cxx.find_library('dl', required: false),
]

if host_machine.system() != 'emscripten'
	deps += [ dependency('threads'), ]
endif

includes = include_directories('include')

subdir('src')

reflection_sources = files([
  'include/oak_util/types.h',
])
