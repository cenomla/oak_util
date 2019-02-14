project('oak_util', 'cpp', default_options : ['cpp_std=c++17', 'warning_level=3'])

cxx = meson.get_compiler('cpp')

deps = [
	dependency('threads'),
	cxx.find_library('m'),
	cxx.find_library('dl'),
]

includes = include_directories('include')

subdir('src')
subdir('tests')
