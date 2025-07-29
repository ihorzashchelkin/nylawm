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
    dependency('epoxy'),
    dependency('x11'),
    dependency('x11-xcb'),
    dependency('xcb'),
    dependency('xcb-errors'),
    dependency('xcb-keysyms'),
    dependency('xcb-ewmh'),
    dependency('xcb-composite'),
    dependency('xcb-util'),
    dependency('xcb-randr'),
    dependency('xcb-damage'),
  ]
)

executable(
  'testegl',
  ['src/testegl.cpp'],
  dependencies: [
    dependency('gl'),
    dependency('egl'),
    dependency('epoxy'),
    dependency('x11'),
    dependency('x11-xcb'),
    dependency('xcb'),
    dependency('xcb-errors'),
    dependency('xcb-keysyms'),
    dependency('xcb-ewmh'),
    dependency('xcb-composite'),
    dependency('xcb-util'),
    dependency('xcb-randr'),
    dependency('xcb-damage'),
  ]
)

