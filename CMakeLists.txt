cmake_minimum_required(VERSION 3.22)

project(XenhuskMixer VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(XenhuskMixer
    PRODUCT_NAME "Xenhusk Mixer"
    COMPANY_NAME "Xenhusk"
    BUNDLE_ID com.xenhusk.mixer
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Assets/icon.png")

juce_add_binary_data(XenhuskMixerData
    SOURCES Assets/icon.png)

target_sources(XenhuskMixer
    PRIVATE
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/AppleMixerLookAndFeel.cpp
        Source/ui/Console.cpp
        Source/ui/Details.cpp
        Source/engine/SettingsDocument.cpp
        Source/engine/RoutingMatrix.cpp
        Source/engine/Mixer.cpp
        Source/engine/MmcssTask.cpp
        Source/engine/AudioEngine.cpp)

target_include_directories(XenhuskMixer PRIVATE Source)

target_compile_definitions(XenhuskMixer
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:XenhuskMixer,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}")

target_link_libraries(XenhuskMixer
    PRIVATE
        XenhuskMixerData
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_gui_extra
        juce::juce_gui_basics
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

if(WIN32)
    target_link_libraries(XenhuskMixer PRIVATE avrt)
endif()

juce_add_console_app(XenhuskMixerTests
    PRODUCT_NAME "Xenhusk Mixer Tests")

target_sources(XenhuskMixerTests
    PRIVATE
        tests/EngineTests.cpp
        Source/engine/RoutingMatrix.cpp
        Source/engine/Mixer.cpp
        Source/engine/SettingsDocument.cpp)

target_include_directories(XenhuskMixerTests PRIVATE Source)

target_compile_definitions(XenhuskMixerTests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)

target_link_libraries(XenhuskMixerTests
    PRIVATE
        juce::juce_audio_basics
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

enable_testing()
add_test(NAME engine COMMAND $<TARGET_FILE:XenhuskMixerTests>)
