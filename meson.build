
project('bufio', 'c',
   version:'1.0.0',
  license:'MPL-2.0',
  license_files: 'LICENSE',
  default_options : [
        'warning_level=3',
        'c_std=c99',
        'buildtype=release',
        'default_library=both',
        'prefer_static=true',
   ]
)

bufio_inc = include_directories('src')

subdir('src')

bufio_dep = declare_dependency(include_directories : bufio_inc, link_with : bufio_lib, sources : bufio_sources)

subdir('examples')
subdir('tests')

pkg_mod = import('pkgconfig')
pkg_mod.generate(bufio_lib)
