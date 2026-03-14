cmake_minimum_required(VERSION 3.15)

project(project_name_XYZ
    VERSION 0.1.0
    LANGUAGES C CXX)

# Default Toolchain
if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/toolchain/linux-native.cmake" CACHE STRING "Default toolchain" FORCE)
endif()

# Compiler standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) #Generates compile_commands.json for clangd

# Gather Source Files
file(GLOB_RECURSE SRC_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# Debug: print source files
message(STATUS "SRC_FILES=${SRC_FILES}")

# Add executable target
add_executable(${PROJECT_NAME} ${SRC_FILES})

# Include directories
target_include_directories(${PROJECT_NAME} 
    PRIVATE #only used for this target, not propagated to dependents
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# Compiler Flags
target_compile_options(${PROJECT_NAME} PRIVATE
    # Common flags
    -Wall
    -Wextra
    -Wpedantic
    # Add more flags as needed, e.g., for debugging or optimization
)


# -------------------------------
# Output name
# -------------------------------
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "PROJECT_NAME_EXECUTABLE"
)

# -------------------------------
# Post-build: generate .hex and .bin
# -------------------------------
#add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#    COMMAND ${CMAKE_OBJCOPY} -O ihex
#        $<TARGET_FILE:${PROJECT_NAME}>
#        ${PROJECT_NAME}.hex
#
#    COMMAND ${CMAKE_OBJCOPY} -O binary
#        $<TARGET_FILE:${PROJECT_NAME}>
#        ${PROJECT_NAME}.bin
#
#    COMMAND ${CMAKE_SIZE}
#        $<TARGET_FILE:${PROJECT_NAME}>
#)
