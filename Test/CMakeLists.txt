cmake_minimum_required(VERSION 3.5)

set (CMAKE_CXX_STANDARD 11)

# Locate GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS}  ../build/install)
link_directories(../build/install)
link_libraries( ${GTEST_LIBRARIES} crypt pthread utils )

# Link runTests with what we want to test and the GTest and pthread library

add_executable(runTests Test_utils.cpp)
target_link_libraries(runTests ${GTEST_LIBRARIES} pthread crypt utils)

