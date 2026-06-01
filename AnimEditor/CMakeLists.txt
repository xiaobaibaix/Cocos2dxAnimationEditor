set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)

file(GLOB IMGUI_SOURCES
    ${IMGUI_DIR}/*.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(anim_editor
    src/main.cpp
    src/core/Serializer.cpp
    ${IMGUI_SOURCES}
)

target_include_directories(anim_editor PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${CMAKE_SOURCE_DIR}/third_party/glfw/include
    ${CMAKE_SOURCE_DIR}/third_party/json/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(anim_editor PRIVATE glfw)

# macOS requires the OpenGL framework
if(APPLE)
    find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
    target_link_libraries(anim_editor PRIVATE ${OPENGL_FRAMEWORK})
endif()

add_subdirectory(tests)
