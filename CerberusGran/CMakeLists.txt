cmake_minimum_required(VERSION 3.15)
project(CerberusGran VERSION 0.1.0)
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE")
add_subdirectory(${JUCE_DIR} JUCE)

# Platform-specific plugin formats: AU is macOS-only
if(APPLE)
    set(PLUGIN_FORMATS VST3 AU)
else()
    set(PLUGIN_FORMATS VST3 Standalone)
endif()

juce_add_plugin(CerberusGran
    COMPANY_NAME "Ishaan"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Ishn
    PLUGIN_CODE CbrG
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "CerberusGran"
)
target_sources(CerberusGran PRIVATE
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
    Source/CombFilter.h
    Source/HeadFXChain.h
    Source/CerberusLookAndFeel.h
    Source/RotaryKnob.h
    Source/FXSlotCard.h
    Source/FilterResponseDisplay.h
    Source/FXChainPanel.h
    Source/WaveformDisplay.h
    Source/GrainShapeKnob.h
    Source/EngineColumn.h
    Source/GlobalBar.h
)

# Disable JUCE features we don't use. JUCE_WEB_BROWSER=0 is also required on
# Linux to avoid the juce_gui_extra.cpp gtk/gtk.h include error (JUCE doesn't
# add GTK headers to the include path unless webview is needed at runtime).
target_compile_definitions(CerberusGran PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)

target_compile_definitions(CerberusGran_VST3 PRIVATE JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(CerberusGran PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
)
juce_generate_juce_header(CerberusGran)

# ---- Platform-specific auto-install ----
if(APPLE)
    add_custom_command(TARGET CerberusGran_AU POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -rf "/Library/Audio/Plug-Ins/Components/CerberusGran.component"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<TARGET_BUNDLE_DIR:CerberusGran_AU>"
            "/Library/Audio/Plug-Ins/Components/CerberusGran.component"
        COMMENT "Installing AU to /Library/Audio/Plug-Ins/Components/"
    )
    add_custom_command(TARGET CerberusGran_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -rf "/Library/Audio/Plug-Ins/VST3/CerberusGran.vst3"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<TARGET_BUNDLE_DIR:CerberusGran_VST3>"
            "/Library/Audio/Plug-Ins/VST3/CerberusGran.vst3"
        COMMENT "Installing VST3 to /Library/Audio/Plug-Ins/VST3/"
    )
endif()
# Note: on Linux, JUCE's COPY_PLUGIN_AFTER_BUILD already installs VST3 to ~/.vst3