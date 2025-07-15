cmake_minimum_required(VERSION 3.10)

project(wm)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} X11 xcb xcb-errors uring)

