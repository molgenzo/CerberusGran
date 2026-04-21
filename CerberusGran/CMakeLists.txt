cmake_minimum_required(VERSION 3.15)

project(PocketGranules VERSION 0.1.0)

set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE")

add_subdirectory(${JUCE_DIR} JUCE)

juce_add_plugin(PocketGranules
    COMPANY_NAME "Ishaan"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Ishn
    PLUGIN_CODE PkGr
    FORMATS VST3 AU
    PRODUCT_NAME "PocketGranules"
)

target_sources(PocketGranules PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/RingBuffer.h
    Source/Grain.h
    Source/WindowFunctions.h
    Source/GrainHead.h
    Source/GrainHead.cpp
    Source/GrainEngine.h
    Source/Parameters.h
    Source/Bitcrusher.h
    Source/HeadFXChain.h
    Source/CerberusLookAndFeel.h
    Source/RotaryKnob.h
    Source/FXSlotCard.h
    Source/FXChainPanel.h
    Source/WaveformDisplay.h
    Source/GrainShapeKnob.h
    Source/EngineColumn.h
    Source/GlobalBar.h
)

target_compile_definitions(PocketGranules_VST3 PRIVATE JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(PocketGranules PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
)

juce_generate_juce_header(PocketGranules)
