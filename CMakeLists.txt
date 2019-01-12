cmake_minimum_required(VERSION 3.1)

set (CMAKE_CXX_STANDARD 17)

project(cfw)

add_library(cfw_lib INTERFACE)

target_include_directories(cfw_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

find_package(X11 REQUIRED)

include_directories(${X11_INCLUDE_DIR})
link_directories(${X11_LIBRARIES})
target_link_libraries(cfw_lib INTERFACE ${X11_LIBRARIES})

FIND_PACKAGE(Threads REQUIRED)
target_link_libraries(cfw_lib INTERFACE ${CMAKE_THREAD_LIBS_INIT})

add_executable(example example.cpp)

target_link_libraries(example PRIVATE cfw_lib)
set_target_properties(example PROPERTIES EXCLUDE_FROM_ALL TRUE)
