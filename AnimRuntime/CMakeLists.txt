cmake_minimum_required(VERSION 3.20)

add_library(anim_runtime STATIC
    src/Easing.cpp
    src/AnimLoader.cpp
    src/AnimPlayer.cpp
)

target_include_directories(anim_runtime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party/json/include
)

# Cocos2d-x 链接（引擎集成后启用）
# target_link_libraries(anim_runtime PUBLIC cocos2d)
