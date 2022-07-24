sources = [
  'algorithm.cpp',
  'fmt.cpp',
  'memory.cpp',
  'random.cpp',
]

oak_util = library(
    'oakutil',
    sources,
    gnu_symbol_visibility: 'hidden',
    include_directories: includes,
    install : true)

oak_util_dep = declare_dependency(link_with: oak_util, include_directories: includes)
