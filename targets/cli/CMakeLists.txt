project(euphoniumcli)
cmake_minimum_required(VERSION 3.18) 
set (CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_definitions(-DUSE_EXTERNAL_BELL)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../euphonium ${CMAKE_CURRENT_BINARY_DIR}/euphonium)


message(${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_web.sh)
execute_process(COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_web.sh" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

find_package( portaudio REQUIRED )
include_directories( ${PORTAUDIO_INCLUDE_DIRS} )
set(THINGS_TO_LINK ${THINGS_TO_LINK} ${PORTAUDIO_LIBRARIES})

file(GLOB SOURCES "*.cpp")
include_directories(".")

add_executable(euphoniumcli ${SOURCES})
target_link_libraries(euphoniumcli euphonium ${CMAKE_DL_LIBS} ${THINGS_TO_LINK} )
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
