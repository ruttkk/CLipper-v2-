cmake_minimum_required(VERSION 3.22)
project(SoftClipper VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(SoftClipper
    COMPANY_NAME "YourName"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    PLUGIN_MANUFACTURER_CODE Ycom
    PLUGIN_CODE Sclp
    FORMATS VST3
    PRODUCT_NAME "SoftClipper"
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(SoftClipper PRIVATE
    PluginProcessor.cpp
    PluginProcessor.h
    PluginEditor.cpp
    PluginEditor.h)

target_compile_definitions(SoftClipper PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

# Arka plan görseli: assets/background.png varsa otomatik pakete gömülür
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets/background.png)
    juce_add_binary_data(SoftClipperAssets SOURCES assets/background.png)
    target_link_libraries(SoftClipper PRIVATE SoftClipperAssets)
    target_compile_definitions(SoftClipper PRIVATE HAS_BACKGROUND=1)
endif()

target_link_libraries(SoftClipper
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
