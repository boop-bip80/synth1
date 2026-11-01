add_executable(cs80_analysis AnalysisMain.cpp)
target_link_libraries(cs80_analysis PRIVATE cs80_dsp)
target_include_directories(cs80_analysis PRIVATE ${CMAKE_SOURCE_DIR}/src/dsp/tests)
target_compile_features(cs80_analysis PRIVATE cxx_std_17)

if(NOT WIN32)
    target_link_libraries(cs80_analysis PRIVATE m)
endif()

if(NOT MSVC)
    target_compile_options(cs80_analysis PRIVATE -O2 -Wall -Wextra)
endif()

add_custom_target(cs80-bench
    COMMAND cs80_analysis bench
    DEPENDS cs80_analysis
    COMMENT "Run the CS-80E CPU benchmark")

add_custom_target(cs80-alias
    COMMAND cs80_analysis alias
    DEPENDS cs80_analysis
    COMMENT "Run the CS-80E aliasing analysis")
