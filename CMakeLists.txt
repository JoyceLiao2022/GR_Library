cmake_minimum_required(VERSION 3.16)

project(gr_library LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(gr STATIC
    src/gsr.cpp
    src/gnr.cpp
)

target_include_directories(gr
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(gsr
    tools/gsr_main.cpp
)

target_include_directories(gsr PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tools
)

target_link_libraries(gsr PRIVATE gr)

add_executable(gnr
    tools/gnr_main.cpp
)

target_include_directories(gnr PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tools
)

target_link_libraries(gnr PRIVATE gr)