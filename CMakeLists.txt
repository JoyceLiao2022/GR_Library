cmake_minimum_required(VERSION 3.16)

project(gr_library LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Python COMPONENTS Interpreter Development REQUIRED)

include(FetchContent)

FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v3.0.1
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(pybind11)

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

set_target_properties(gr PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

add_executable(gnr
    tools/gnr_main.cpp
)

target_include_directories(gnr PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tools
)

target_link_libraries(gnr PRIVATE gr)

pybind11_add_module(gr_python
    python/bindings.cpp
)

target_link_libraries(gr_python PRIVATE gr)

add_executable(api_smoke
    tests/api_smoke.cpp
)

target_link_libraries(api_smoke PRIVATE gr)

include(CTest)

if(BUILD_TESTING)

    add_test(
        NAME cpp_api_smoke
        COMMAND api_smoke
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/toy.log
    )

    add_test(
        NAME python_api_smoke
        COMMAND
            ${CMAKE_COMMAND} -E env
            "PYTHONPATH=$<TARGET_FILE_DIR:gr_python>"
            ${Python_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/python_smoke.py
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/toy.log
    )

endif()

add_executable(gr_example
    examples/example.cpp
)

target_link_libraries(gr_example PRIVATE gr)