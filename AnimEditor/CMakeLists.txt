cmake_minimum_required(VERSION 3.20)
project(AnimEditor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 REQUIRED)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)

file(GLOB IMGUI_SOURCES
    ${IMGUI_DIR}/*.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(anim_editor
    src/main.cpp
    src/core/Serializer.cpp
    src/core/SceneGraph.cpp
    src/core/UndoSystem.cpp
    src/core/Easing.cpp
    src/core/AnimationEngine.cpp
    src/renderer/CocosEmbed.cpp
    src/ui/FileBrowserPanel.cpp
    src/ui/NodeTreePanel.cpp
    src/ui/PreviewCanvas.cpp
    src/ui/PropertyPanel.cpp
    src/ui/EditorUI.cpp
    src/ui/TimelinePanel.cpp
    src/platform/NativeDialogs.mm
    ${IMGUI_SOURCES}
)

target_include_directories(anim_editor PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${CMAKE_SOURCE_DIR}/third_party/json/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(anim_editor PRIVATE glfw)

# macOS requires the OpenGL and AppKit frameworks
if(APPLE)
    find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
    find_library(APPKIT_FRAMEWORK AppKit REQUIRED)
    target_link_libraries(anim_editor PRIVATE ${OPENGL_FRAMEWORK} ${APPKIT_FRAMEWORK})
endif()

add_subdirectory(tests)
