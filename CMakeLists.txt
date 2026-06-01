cmake_minimum_required(VERSION 3.20)
project(AnimToolkit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# GLFW options (set before add_subdirectory)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# Third-party dependencies
add_subdirectory(third_party/glfw)
# ImGui sources are collected directly by AnimEditor/CMakeLists.txt

# Editor application
enable_testing()
add_subdirectory(AnimEditor)
