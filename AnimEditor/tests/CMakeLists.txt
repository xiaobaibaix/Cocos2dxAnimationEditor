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
