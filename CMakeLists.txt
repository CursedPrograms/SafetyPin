cmake_minimum_required(VERSION 3.22)
project(SafetyPin VERSION 0.2.0)

# Build the C++ runtime into the exe / VST3, so they run on PCs without the Visual C++ Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

set(SAFETY_PIN_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/PunkLook.cpp
    Source/PlateReverb.cpp
    Source/OctaveDown.cpp
    Source/Tuner.cpp
    Source/TunerDisplay.cpp
    Source/MidiLearn.cpp)

set(SAFETY_PIN_DEFINITIONS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_ASIO=1                      # Focusrite USB ASIO in the standalone app
    JUCE_VST3_CAN_REPLACE_VST2=0)

juce_add_plugin(SafetyPin
    COMPANY_NAME "Cursed Entertainment"
    PLUGIN_MANUFACTURER_CODE Crsd
    PLUGIN_CODE SfPn
    FORMATS VST3 Standalone
    PRODUCT_NAME "Safety Pin"
    DESCRIPTION "Electric violin distortion: octave down (cello), sub bass, plate reverb, violin tuner"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE                # MIDI learn
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx Distortion Reverb
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(SafetyPin PRIVATE ${SAFETY_PIN_SOURCES})
target_compile_features(SafetyPin PRIVATE cxx_std_17)
target_compile_definitions(SafetyPin PUBLIC ${SAFETY_PIN_DEFINITIONS})
target_link_libraries(SafetyPin
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# Offline test: renders the GUI to a PNG and a synthetic violin through the effect to WAVs.
juce_add_console_app(SafetyPinRender PRODUCT_NAME "SafetyPinRender")
target_sources(SafetyPinRender PRIVATE tools/render.cpp ${SAFETY_PIN_SOURCES})
target_compile_features(SafetyPinRender PRIVATE cxx_std_17)
target_compile_definitions(SafetyPinRender PRIVATE ${SAFETY_PIN_DEFINITIONS} JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(SafetyPinRender
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
