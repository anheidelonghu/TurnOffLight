# cmake needs this line
cmake_minimum_required(VERSION 2.8)

#set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_BUILD_TYPE RelWithDebInfo)

find_package(OpenCV REQUIRED)
message(STATUS "OpenCV library status:")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

if(CMAKE_VERSION VERSION_LESS "2.8.11")
  # Add OpenCV headers location to your include paths
  include_directories(${OpenCV_INCLUDE_DIRS})
endif()

# Declare the executable target built from your sources

#add_executable(main main_mul.cpp)
add_executable(main main.cpp)
#add_executable(test test.cpp)
#add_executable(v4l2Demo v4l2Demo.cpp)

#target_link_libraries(main ${OpenCV_LIBS} wiringPi pthread)
target_link_libraries(main ${OpenCV_LIBS} wiringPi pthread)
#target_link_libraries(test ${OpenCV_LIBS} wiringPi pthread)
#target_link_libraries(v4l2Demo ${OpenCV_LIBS} wiringPi pthread)