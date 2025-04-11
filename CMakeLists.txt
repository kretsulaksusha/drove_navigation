cmake_minimum_required(VERSION 3.15)

#  It is used as your project's main executable name.
set(PROJECT_NAME drone_navigation)
project(${PROJECT_NAME} CXX)

set(CMAKE_CXX_STANDARD 20)

# Collect sources
file(GLOB sources src/main.cpp
        src/depth/*.cpp src/detectors/*.cpp src/filters/*.cpp src/video_processor/*.cpp src/utils/*.cpp
        include/depth/*.hpp include/detectors/*.hpp include/filters/*.hpp
        include/video_processor/*.hpp include/utils/*.hpp)

file(GLOB test_depth_estimation_sources tests/test_depth_estimation.cpp
        src/depth/*.cpp
        include/depth/*.hpp
        src/utils/path_utils.cpp
        include/utils/*.cpp)

file(GLOB test_fast_detector_sources tests/test_fast_detector.cpp
        src/detectors/*.cpp
        include/detectors/*.hpp
        src/utils/path_utils.cpp
        include/utils/path_utils.cpp)

file(GLOB test_kalman_sources tests/test_kalman.cpp
        src/filters/*.cpp
        include/filters/*.hpp)

#! Add external packages
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(EIGEN3 REQUIRED eigen3)

##########################################################
# Project files, packages, libraries and so on
##########################################################

# Main project executable
add_executable(${PROJECT_NAME} ${sources})

# Test executables
add_executable(test_depth_estimation ${test_depth_estimation_sources})
add_executable(test_fast_detector ${test_fast_detector_sources})
add_executable(test_kalman ${test_kalman_sources})

##########################################################
# Include directories
##########################################################
#! Put path to your project headers

target_include_directories(${PROJECT_NAME} PRIVATE
        include/depth
        include/detectors
        include/filters
        include/video_processor
        include/utils
        ${OpenCV_INCLUDE_DIRS}
        ${EIGEN3_INCLUDE_DIRS}
)

target_include_directories(test_depth_estimation PRIVATE
        include/depth
        include/utils
        ${OpenCV_INCLUDE_DIRS}
)

target_include_directories(test_fast_detector PRIVATE
        include/detectors
        include/utils
        ${OpenCV_INCLUDE_DIRS}
)

target_include_directories(test_kalman PRIVATE
        include/filters
        ${OpenCV_INCLUDE_DIRS}
        ${EIGEN3_INCLUDE_DIRS}
)

##########################################################
# Link libraries
##########################################################

target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})
target_link_libraries(test_depth_estimation ${OpenCV_LIBS})
target_link_libraries(test_fast_detector ${OpenCV_LIBS})
target_link_libraries(test_kalman ${OpenCV_LIBS})
