project('oak_util', 'cpp')

cxx = meson.get_compiler('cpp')

deps = [
	dependency('threads'),
	cxx.find_library('m'),
	cxx.find_library('dl'),
]

includes = include_directories('include')

subdir('src')
subdir('tests')
