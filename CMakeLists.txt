cmake_minimum_required(VERSION 3.22)

project(ALMIGHTY_PAIN VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Path to the JUCE source tree (JUCE 8.0.0). The CI workflow passes -DJUCE_DIR=...
# Locally: git clone --branch 8.0.0 https://github.com/juce-framework/JUCE.git JUCE
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/JUCE" CACHE PATH "Path to the JUCE source tree")

if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found at '${JUCE_DIR}'. Clone JUCE 8.0.0 there or pass -DJUCE_DIR=<path>.")
endif()

add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/juce_build")

juce_add_plugin(AlmightyPain
    PLUGIN_NAME                 "ALMIGHTY PAIN"
    PRODUCT_NAME                "ALMIGHTY PAIN"
    DESCRIPTION                 "Dual ping-pong delay with PAIN macro, chorus, flanger and feedback filters"
    VERSION                     ${PROJECT_VERSION}
    COMPANY_NAME                "Almighty Audio"
    BUNDLE_ID                   com.almightyaudio.almightypain
    PLUGIN_MANUFACTURER_CODE    AAPa
    PLUGIN_CODE                 ApnP
    FORMATS                     VST3
    VST3_CATEGORIES             Fx Delay
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE)

target_sources(AlmightyPain
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h)

target_compile_definitions(AlmightyPain
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(AlmightyPain)
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
