juce_add_console_app(Cs80Screenshots PRODUCT_NAME "cs80-screenshots")

target_sources(Cs80Screenshots PRIVATE screenshot/ScreenshotMain.cpp)

target_link_libraries(Cs80Screenshots
    PRIVATE
        Cs80PluginCore
        cs80_juce_flags
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
)

if(NOT MSVC)
    target_compile_options(Cs80Screenshots PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
