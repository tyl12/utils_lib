cmake_minimum_required(VERSION 3.5)
project(utils)
set (CMAKE_CXX_STANDARD 11)


# Find source files
file(GLOB SRC_FILES src/*.cpp)
##or:
#SET(SRC_FILES src/utils.cpp)

# Include header files
include_directories(src)

# Create shared library
add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
##or:
#add_library(utils SHARED ${SRC_FILES})

SET(CMAKE_INSTALL_PREFIX  ./)

#INSTALL(FILES src/utils.h  src/UtilSingleton.h  DESTINATION install)
#INSTALL(TARGETS utils DESTINATION install)


# Install library
install(TARGETS ${PROJECT_NAME} DESTINATION install/)

# Install library headers
file(GLOB HEADERS src/*.h)
install(FILES ${HEADERS} DESTINATION install/)
install(DIRECTORY src/rapidjson DESTINATION install/)


