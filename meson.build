project(
  'nylawm',
  'c',
)

executable(
  'nyla',
  ['src/nyla.c'],
  dependencies: [
    dependency('GL'),
    dependency('egl'),
    dependency('X11'),
    dependency('x11-xcb'),
    dependency('xcb'),
    dependency('xcb-keysyms'),
    dependency('xcb-util'),
    dependency('xcb-xkb'),
  ]
)


