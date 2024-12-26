# Oak Util

A c++ standard library replacement

#### Meson Project Configuration

##### Add oak\_util.wrap to your subprojects

```meson-wrap
[wrap-git]
url = git@github.com:cenomla/oak_util.git
revision = <commithash>
```

##### Recommended default meson project file

```meson
project('project', ['c', 'cpp'],
  default_options : [
    'default_library=static',
    'cpp_eh=none',
    'cpp_rtti=false',
    'cpp_std=c++17',
    'warning_level=3',
    'werror=true',
    'b_ndebug=if-release' ])
```

##### Add specific warnings

```meson
cxx = meson.get_compiler('cpp')
if cxx.get_id() == 'gcc'
  add_global_arguments(
    '-Wconversion',
    '-Wdouble-promotion',
    '-Wshadow',
    '-Wno-maybe-uninitialized',
    '-Wno-error=class-memaccess',
    '-Wno-error=shadow',
    '-Wno-error=format-truncation',
    language: 'cpp')
elif cxx.get_id() == 'msvc'
  add_global_arguments(
    '/wd4324', # struct padding
    '/wd4251', # dll interface
    language: 'cpp')
endif
```

##### Include oak\_util in your project

```meson
oak_util = subproject('oak_util')
deps = [
  oak_util.get_variable('oak_util_dep'),
]

main = executable(
  'main',
  main_sources,
  gnu_symbol_visibility: 'hidden',
  dependencies : deps,
  install : true,
  build_rpath: '$ORIGIN',
  install_rpath: '$ORIGIN')
```

