cmake_minimum_required(VERSION 3.15)

#  It is used as your project's main executable name.
set(PROJECT_NAME depth_estimation)
project(${PROJECT_NAME} CXX)

set(CMAKE_CXX_STANDARD 20)

file(GLOB sources src/*.cpp include/*.hpp)

#! Add external packages
find_package(OpenCV REQUIRED)

##########################################################
# Project files, packages, libraries and so on
##########################################################

add_executable(${PROJECT_NAME} ${sources})

#! Put path to your project headers
target_include_directories(${PROJECT_NAME} PRIVATE include)

target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})
