cmake_minimum_required(VERSION 3.22)
project(STUCK VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if (NOT DEFINED JUCE_DIR)
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
else()
    add_subdirectory(${JUCE_DIR} JUCE)
endif()

juce_add_plugin(STUCK
    COMPANY_NAME "Pouria Motabean"
    PLUGIN_MANUFACTURER_CODE Pmtb
    PLUGIN_CODE Stck
    FORMATS VST3
    PRODUCT_NAME "STUCK"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_add_binary_data(STUCKAssets SOURCES Resources/panel.png Resources/knob.png)

target_sources(STUCK PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)

target_compile_definitions(STUCK PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(STUCK PRIVATE
    STUCKAssets
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
