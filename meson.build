project('oak_util', 'cpp')

cxx = meson.get_compiler('cpp')

deps = [
	dependency('threads'),
	cxx.find_library('m'),
	cxx.find_library('dl'),
]

includes = include_directories('include')

subdir('src')

test_deps = deps + [ oak_util_dep ]
subdir('tests')
