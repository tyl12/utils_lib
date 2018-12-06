cmake_minimum_required(VERSION 3.5)
PROJECT(utils)
set(CMAKE_CXX_STANDARD 11)

# Find source files
file(GLOB SRC_FILES ./*.cpp)
# Find header files
file(GLOB HEADERS ./*.h)

# set include path
include_directories(.)

# Create shared library
add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
##or:
#add_library(utils SHARED ${SRC_FILES})

SET(CMAKE_INSTALL_PREFIX  ./)

# Install library
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION install/)
#INSTALL(TARGETS utils DESTINATION install)


# Install library headers
INSTALL(FILES ${HEADERS} DESTINATION install/)
#INSTALL(FILES ./utils.h  ./UtilSingleton.h  DESTINATION install)

INSTALL(DIRECTORY ./rapidjson DESTINATION install/)

