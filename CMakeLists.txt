cmake_minimum_required(VERSION 3.20)

project(HOI4MapPointsGenerator)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++ -mwindows")

add_executable(HOI4MapPointsGenerator
        src/H2A_Map.cpp
        src/stb_image.h
        app.rc)

target_link_libraries(${PROJECT_NAME} PRIVATE
        comctl32
)
