cmake_minimum_required(VERSION 3.10)
project(CPPOpenGL)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(OpenGL REQUIRED)

# Add include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Add the library directory
link_directories(${CMAKE_SOURCE_DIR}/lib)

# Add source files
add_executable(CPPOpenGL 
    ${CMAKE_SOURCE_DIR}/src/Utils.cpp
    ${CMAKE_SOURCE_DIR}/src/TextRenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/Button.cpp
    ${CMAKE_SOURCE_DIR}/src/Object.cpp
    ${CMAKE_SOURCE_DIR}/src/Slider.cpp
    ${CMAKE_SOURCE_DIR}/src/Joystick.cpp
    ${CMAKE_SOURCE_DIR}/src/EntityManager.cpp
    ${CMAKE_SOURCE_DIR}/src/main.cpp
)

# Link the libraries
target_link_libraries(CPPOpenGL glew32 glfw3 soil2-debug opengl32)

# Copy GLEW DLL to the output directory
add_custom_command(TARGET CPPOpenGL POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/glew32.dll"
    $<TARGET_FILE_DIR:CPPOpenGL>)

file(COPY ${CMAKE_SOURCE_DIR}/src/shaders DESTINATION ${CMAKE_BINARY_DIR})

add_custom_command(
    TARGET CPPOpenGL POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/src/shaders $<TARGET_FILE_DIR:CPPOpenGL>/shaders
)

file(COPY ${CMAKE_SOURCE_DIR}/src/assets DESTINATION ${CMAKE_BINARY_DIR})

# Post-build event to copy shaders again after each build
add_custom_command(
    TARGET CPPOpenGL POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/src/assets $<TARGET_FILE_DIR:CPPOpenGL>/assets
)