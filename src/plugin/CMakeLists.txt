set(CS80_FORMATS VST3 Standalone)

if(APPLE)
    list(APPEND CS80_FORMATS AU)
endif()

juce_add_plugin(Cs80Plugin
    COMPANY_NAME "Freebuff Audio"
    BUNDLE_ID "com.freebuff.cs80e"
    PLUGIN_MANUFACTURER_CODE Free
    PLUGIN_CODE Cs80
    PRODUCT_NAME "CS-80E"
    FORMATS ${CS80_FORMATS}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES "Instrument Synth"
    AU_MAIN_TYPE "kAudioUnitType_MusicDevice"
    COMPANY_COPYRIGHT "Freebuff Audio"
)

# Shared settings for every target that compiles JUCE modules, so the module
# implementation stays identical between the plugin and the offline renderer.
add_library(cs80_juce_flags INTERFACE)
target_compile_definitions(cs80_juce_flags INTERFACE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

juce_add_binary_data(Cs80FontData SOURCES
    ${CMAKE_SOURCE_DIR}/assets/fonts/ChakraPetchRegular.ttf
    ${CMAKE_SOURCE_DIR}/assets/fonts/ChakraPetchSemiBold.ttf
    ${CMAKE_SOURCE_DIR}/assets/fonts/ChakraPetchBold.ttf
    ${CMAKE_SOURCE_DIR}/assets/fonts/MichromaRegular.ttf
    ${CMAKE_SOURCE_DIR}/assets/fonts/JetBrainsMonoRegular.ttf
    ${CMAKE_SOURCE_DIR}/assets/fonts/JetBrainsMonoBold.ttf
)

add_library(Cs80PluginCore STATIC
    PluginProcessor.cpp
    PluginEditor.cpp
    ui/Theme.cpp
    ui/LookAndFeel.cpp
    ui/Widgets.cpp
    ui/Performance.cpp
    ui/PresetUI.cpp
    ui/AdvancedPanel.cpp
)

target_include_directories(Cs80PluginCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/dsp
)

target_link_libraries(Cs80PluginCore
    PUBLIC cs80_dsp
    PRIVATE
        Cs80FontData
        cs80_juce_flags
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
)

target_sources(Cs80Plugin PRIVATE PluginEntry.cpp)

target_link_libraries(Cs80Plugin
    PRIVATE
        Cs80PluginCore
        cs80_juce_flags
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
)

if(NOT MSVC)
    target_compile_options(Cs80PluginCore PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
