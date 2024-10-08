set(CMAKE_CXX_STANDARD 20)

CPMAddPackage(
    NAME clap-juce-extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG main
)

juce_add_plugin(TestHostPlugin
    COMPANY_NAME chowdsp
    PLUGIN_MANUFACTURER_CODE Chow
    PLUGIN_CODE ${code}
    FORMATS VST3 AU Standalone
    PRODUCT_NAME ${target}
    MICROPHONE_PERMISSION_ENABLED TRUE
)

target_sources(TestHostPlugin
    PRIVATE
        TestHostPlugin.cpp

        ../juce_clap_hosting.cpp
)

target_compile_definitions(TestHostPlugin
    PUBLIC
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0

        JUCE_PLUGINHOST_LV2=1
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_VST=0
        JUCE_PLUGINHOST_AU=1
)

target_include_directories(TestHostPlugin PRIVATE ../..)

target_link_libraries(TestHostPlugin
    PRIVATE
        juce::juce_audio_utils
        clap-core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

clap_juce_extensions_plugin(
    TARGET TestHostPlugin
    CLAP_ID "org.chowdsp.test_hosting"
    CLAP_FEATURES audio-effect testing
)
