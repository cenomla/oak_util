sources = [
  'algorithm.cpp',
  'fmt.cpp',
  'memory.cpp',
  'random.cpp',
  'type_info.cpp',
]

oak_util = library('oakutil', sources, include_directories: includes)

oak_util_dep = declare_dependency(link_with: oak_util, include_directories: includes)
