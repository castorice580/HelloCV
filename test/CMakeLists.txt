cmake_minimum_required(VERSION 3.10)
project(OpenCVImageDemo)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

add_executable(image_demo main.cpp)

target_link_libraries(image_demo ${OpenCV_LIBS})
target_include_directories(image_demo PRIVATE ${OpenCV_INCLUDE_DIRS})