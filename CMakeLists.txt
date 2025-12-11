cmake_minimum_required(VERSION 3.15)

project(BuildUpVerb VERSION 1.1.6)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
)
FetchContent_MakeAvailable(JUCE)

# JUCE plugin configuration
juce_add_plugin(BuildUpVerb
    COMPANY_NAME "Plugin Freedom System"
    PLUGIN_MANUFACTURER_CODE PFS1
    PLUGIN_CODE Bldv
    FORMATS VST3 AU
    PRODUCT_NAME "BuildUp Reverb"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    NEEDS_WEB_BROWSER TRUE)

# Source files
target_sources(BuildUpVerb PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/FreeverbWrapper.cpp
    Source/VocoderProcessor.cpp
    Source/VocoderFilterbank.cpp
    Source/VocoderSimple.cpp
    Source/VocoderGated.cpp
    Source/revmodel.cpp
    Source/comb.cpp
    Source/allpass.cpp)

# Link libraries
target_link_libraries(BuildUpVerb
    PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# Generate JUCE header
juce_generate_juce_header(BuildUpVerb)

# Plugin-specific preprocessor definitions
target_compile_definitions(BuildUpVerb PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)