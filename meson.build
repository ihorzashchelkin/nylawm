project(
  'nylawm',
  'cpp',
  default_options : ['cpp_std=c++26']
)

executable(
  'nyla',
  ['src/main.cpp'],
  dependencies: [
    dependency('gl'),
    dependency('egl'),
    dependency('X11'),
    dependency('x11-xcb'),
    dependency('xcb'),
    dependency('xcb-composite'),
    dependency('xcb-dri3'),
  ]
)


