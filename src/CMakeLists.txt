cmake_minimum_required( VERSION 3.0 )

set(CMAKE_CXX_STANDARD 20)

project( ThreadPoolLib )


#find_package(Boost REQUIRED)

#if(NOT  ${Boost_FOUND})
#MESSAGE(FATAL_ERROR "boost not found")
#endif()

file (GLOB SOURCE_FILES *.cpp *.cppm )

#include_directories(${Boost_INCLUDE_DIRS})

add_executable( ${PROJECT_NAME} ${SOURCE_FILES}) 

target_link_libraries( ${PROJECT_NAME} pthread)
