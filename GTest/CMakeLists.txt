cmake_minimum_required(VERSION 3.5)

set (CMAKE_CXX_STANDARD 11)
set (SRC_FILES ./Test_utils.cpp)

# use:  cmake .. -DUTILS_PATH=`pwd`/../../utils/build/install
option(UTILS_PATH "specify libutils.so path" "../../utils/build/install")
message("option is ${UTILS_PATH}")
if(UTILS_PATH)
    message("use defined UTILS_PATH: ${UTILS_PATH}")
else()
    message("UTILS_PATH undefined")
endif()

# Locate GTest
find_package(GTest REQUIRED)

include_directories(${GTEST_INCLUDE_DIRS}   ${UTILS_PATH})
link_directories(${UTILS_PATH})

#boost_system, boost_thread for circ_buf.h
link_libraries( ${GTEST_LIBRARIES} crypt pthread utils  boost_system  boost_thread)

# Link runTests with what we want to test and the GTest and pthread library

add_executable(testUtils  ${SRC_FILES})
target_link_libraries(testUtils ${GTEST_LIBRARIES} pthread crypt utils)


#SET(CMAKE_INSTALL_PREFIX  ./)
#INSTALL(TARGETS testUtils DESTINATION install/)

#------------------------------------------------------
set (SRC_FILES ./Test_ThreadPool.cpp)

include_directories(${UTILS_PATH})
link_directories(${UTILS_PATH})
link_libraries(pthread utils boost_system  boost_thread)

add_executable(testThreadPool  ${SRC_FILES})
target_link_libraries(testThreadPool pthread crypt utils)

#-------------------------------------------------------
set (SRC_FILES ./Test_SyncQueue.cpp)

include_directories(${UTILS_PATH})
link_directories(${UTILS_PATH})
link_libraries(pthread utils boost_system  boost_thread)

add_executable(testSyncQueue ${SRC_FILES})
target_link_libraries(testSyncQueue pthread crypt utils)
