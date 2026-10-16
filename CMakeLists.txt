cmake_minimum_required(VERSION 3.16)

project(AlmightyPain VERSION 1.0.0)

# Directly include JUCE from the path set up by GitHub Actions
add_subdirectory(C:/JUCE JUCE)

juce_add_plugin(AlmightyPain
    COMPANY_NAME "Almighty Audio"
    PLUGIN_CODE "ApnP"
    FORMATS VST3
    PRODUCT_NAME "Almighty Pain"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE)

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

target_link_libraries(AlmightyPain
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
