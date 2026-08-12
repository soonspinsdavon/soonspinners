cmake_minimum_required(VERSION 3.22)

project(SoonSpinner VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Pull in JUCE ------------------------------------------------------
# First build will clone JUCE from GitHub (a few hundred MB). Pin the tag
# so builds stay reproducible; bump it deliberately when you want to
# upgrade JUCE.
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# --- Plugin target -------------------------------------------------------
juce_add_plugin(SoonSpinner
    COMPANY_NAME                "Soonspins"
    PLUGIN_MANUFACTURER_CODE    Snsp
    PLUGIN_CODE                 Sspn
    FORMATS                     VST3 AU Standalone
    PRODUCT_NAME                "SoonSpinner"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            TRUE          # lets you MIDI-trigger the spin buttons later
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     TRUE
)

target_sources(SoonSpinner PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/VariSpinEngine.cpp
)

# --- Bundled art assets (backgrounds, logos, etc.) ------------------------
# Anything dropped in Source/Assets gets compiled straight into the plugin
# binary as BinaryData::<filename>_<ext>, so there's nothing extra to ship
# or point at on disk at runtime.
juce_add_binary_data(SoonSpinnerAssets SOURCES
    Source/Assets/background_clouds_orange.png
)

target_link_libraries(SoonSpinner PRIVATE SoonSpinnerAssets)

target_compile_definitions(SoonSpinner PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(SoonSpinner PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
