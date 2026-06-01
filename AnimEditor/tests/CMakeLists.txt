find_package(GTest REQUIRED)

add_executable(test_AnimData test_AnimData.cpp)
target_include_directories(test_AnimData PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(test_AnimData PRIVATE GTest::gtest_main)
# Set rpath to GTest library location (homebrew/conda)
set_target_properties(test_AnimData PROPERTIES
    BUILD_RPATH "/opt/homebrew/anaconda3/lib"
    INSTALL_RPATH "/opt/homebrew/anaconda3/lib"
)
add_test(NAME AnimData COMMAND test_AnimData)

add_executable(test_Serializer
    test_Serializer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/Serializer.cpp
)
target_include_directories(test_Serializer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_SOURCE_DIR}/third_party/json/include
)
target_link_libraries(test_Serializer PRIVATE GTest::gtest_main)
set_target_properties(test_Serializer PROPERTIES
    BUILD_RPATH "/opt/homebrew/anaconda3/lib"
    INSTALL_RPATH "/opt/homebrew/anaconda3/lib"
)
add_test(NAME Serializer COMMAND test_Serializer)

add_executable(test_SceneGraph
    test_SceneGraph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/SceneGraph.cpp
)
target_include_directories(test_SceneGraph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(test_SceneGraph PRIVATE GTest::gtest_main)
set_target_properties(test_SceneGraph PROPERTIES
    BUILD_RPATH "/opt/homebrew/anaconda3/lib"
    INSTALL_RPATH "/opt/homebrew/anaconda3/lib"
)
add_test(NAME SceneGraph COMMAND test_SceneGraph)

add_executable(test_UndoSystem
    test_UndoSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/UndoSystem.cpp
)
target_include_directories(test_UndoSystem PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(test_UndoSystem PRIVATE GTest::gtest_main)
set_target_properties(test_UndoSystem PROPERTIES
    BUILD_RPATH "/opt/homebrew/anaconda3/lib"
    INSTALL_RPATH "/opt/homebrew/anaconda3/lib"
)
add_test(NAME UndoSystem COMMAND test_UndoSystem)

add_executable(test_Easing
    test_Easing.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/Easing.cpp
)
target_include_directories(test_Easing PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(test_Easing PRIVATE GTest::gtest_main)
set_target_properties(test_Easing PROPERTIES
    BUILD_RPATH "/opt/homebrew/anaconda3/lib"
    INSTALL_RPATH "/opt/homebrew/anaconda3/lib"
)
add_test(NAME Easing COMMAND test_Easing)

add_executable(test_AnimationEngine
    test_AnimationEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/AnimationEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/Easing.cpp
)
target_include_directories(test_AnimationEngine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(test_AnimationEngine PRIVATE GTest::gtest_main)
set_target_properties(test_AnimationEngine PROPERTIES
    BUILD_RPATH "/opt/homebrew/anaconda3/lib"
    INSTALL_RPATH "/opt/homebrew/anaconda3/lib"
)
add_test(NAME AnimationEngine COMMAND test_AnimationEngine)
