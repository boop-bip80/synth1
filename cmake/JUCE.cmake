include(FetchContent)

set(JUCE_VERSION "8.0.10" CACHE STRING "JUCE release tag to build against")
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(JUCE
    URL "https://github.com/juce-framework/JUCE/archive/refs/tags/${JUCE_VERSION}.tar.gz"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/_deps"
)

FetchContent_MakeAvailable(JUCE)
