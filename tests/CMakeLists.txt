add_executable(test_basic basic.cpp ../src/wm_instance.cpp)
target_link_libraries(test_basic
    xcb
    xcb-errors
    xcb-keysyms)

