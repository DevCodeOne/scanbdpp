cmake_minimum_required(VERSION 3.6.2)
project(scanbdpp C CXX)
set(EXECUTABLE_OUTPUT_PATH build)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_subdirectory(confusepp)
add_subdirectory(sanepp)

include_directories(include)
include_directories(confusepp/include)
include_directories(sanepp/include)

file(GLOB ALL_SOURCE_FILES "src/*cpp")

find_package(DBusCpp)
find_package(UDev)

include_directories(${DBUSCPP_INCLUDE_DIR})
include_directories(${UDEV_INCLUDE_DIR})

add_executable(scanbdpp ${ALL_SOURCE_FILES})

target_link_libraries(scanbdpp ${DBUSCPP_LIBRARIES})
target_link_libraries(scanbdpp ${UDEV_LIBRARIES})
target_link_libraries(scanbdpp confusepp)
target_link_libraries(scanbdpp sanepp)
