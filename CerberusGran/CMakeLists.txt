cmake_minimum_required(VERSION 3.15)
project(CerberusGran VERSION 0.1.0)

set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE")
add_subdirectory(${JUCE_DIR} JUCE)

# Only build AU on macOS; VST3 everywhere
if(APPLE)
    set(PLUGIN_FORMATS VST3 AU)
else()
    set(PLUGIN_FORMATS VST3)
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
    Source/HeadFilter.h
)

target_compile_definitions(CerberusGran_VST3 PRIVATE JUCE_VST3_CAN_REPLACE_VST2=0)

# Disable web browser and curl to avoid webkit2 dependency on Linux
target_compile_definitions(CerberusGran PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(CerberusGran PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
)

juce_generate_juce_header(CerberusGran)

# Fix GTK include paths on Linux (JUCE sometimes fails to propagate them)
if(NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    target_include_directories(CerberusGran PRIVATE ${GTK3_INCLUDE_DIRS})
    target_link_libraries(CerberusGran PRIVATE ${GTK3_LIBRARIES})
endif()

# Auto-install plugin after build
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
else()
    # Linux: VST3 is a directory structure, not a bundle.
    add_custom_command(TARGET CerberusGran_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/.vst3"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "$ENV{HOME}/.vst3/CerberusGran.vst3"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_BINARY_DIR}/CerberusGran_artefacts/$<CONFIG>/VST3/CerberusGran.vst3"
            "$ENV{HOME}/.vst3/CerberusGran.vst3"
        COMMENT "Installing VST3 to ~/.vst3/"
    )
endif()