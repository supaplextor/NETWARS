cmake_minimum_required(VERSION 3.16)
project(netwars_modern LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 REQUIRED)

add_executable(netwars-modern
    NETWARS/src/modern_main.cpp
)

target_include_directories(netwars-modern PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(netwars-modern PRIVATE ${SDL2_LIBRARIES})
