cmake_minimum_required(VERSION 3.22)

project(CS80E VERSION 5.2.0 LANGUAGES C CXX)

if(MSVC)
    # The editor uses UTF-8 punctuation in its labels. Compile every target as
    # UTF-8, and keep the object-section limit raised for the large JUCE UI
    # and test translation units (JUCE already does this for its own modules).
    add_compile_options(/utf-8 $<$<COMPILE_LANGUAGE:CXX>:/bigobj>)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(CS80_BUILD_PLUGIN "Build the JUCE plugin and editor" ON)
option(CS80_BUILD_TESTS "Build the headless DSP test suite" ON)
option(CS80_BUILD_TOOLS "Build the offline screenshot/demo renderer" ON)

enable_testing()

add_subdirectory(src/dsp)

if(CS80_BUILD_TESTS)
    add_subdirectory(tools/analysis)
endif()

if(CS80_BUILD_PLUGIN)
    include(cmake/JUCE.cmake)
    add_subdirectory(src/plugin)

    if(CS80_BUILD_TOOLS)
        add_subdirectory(tools)
    endif()
endif()
