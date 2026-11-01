add_library(cs80_dsp STATIC
    Params.cpp
    Engine.cpp
    Presets.cpp
)

target_include_directories(cs80_dsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(cs80_dsp PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(cs80_dsp PRIVATE /W4)
else()
    target_compile_options(cs80_dsp PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(CS80_BUILD_TESTS)
    find_package(Threads REQUIRED)
    add_executable(cs80_tests tests/DspTests.cpp)
    target_link_libraries(cs80_tests PRIVATE cs80_dsp Threads::Threads)

    if(NOT WIN32)
        target_link_libraries(cs80_tests PRIVATE m)
    endif()

    add_test(NAME cs80_dsp_tests COMMAND cs80_tests)
    set_tests_properties(cs80_dsp_tests PROPERTIES
        TIMEOUT 180
        LABELS "dsp;unit"
    )
endif()
