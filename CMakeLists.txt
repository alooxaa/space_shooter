cmake_minimum_required(VERSION 3.20)

# Build either this folder alone or as part of cg-course.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(space_shooter LANGUAGES C CXX)
endif()

if(NOT TARGET glfw)
    find_package(glfw3 3.3 QUIET)
    if(NOT TARGET glfw)
        include(FetchContent)
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
        # X11 also works under WSLg; no Wayland build tools are needed.
        set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(glfw
            URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
        )
        FetchContent_MakeAvailable(glfw)
    endif()
endif()

if(NOT TARGET glad)
    add_library(glad STATIC external/glad/src/gl.c)
    target_include_directories(glad PUBLIC external/glad/include)
endif()

add_executable(space_shooter main.cpp)
target_compile_features(space_shooter PRIVATE cxx_std_17)
set_target_properties(space_shooter PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
target_link_libraries(space_shooter PRIVATE glad glfw)
