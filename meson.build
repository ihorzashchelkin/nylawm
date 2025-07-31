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
    dependency('xcb'),
    dependency('xcb-composite'),
    dependency('xcb-damage'),
    dependency('xcb-dri3'),
    dependency('xcb-errors'),
    dependency('xcb-keysyms'),
    dependency('xcb-util'),
  ]
)


