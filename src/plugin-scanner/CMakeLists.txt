
add_executable(plugin-scanner
  plugin_scanner_subprocess.h
  plugin_scanner_subprocess.cpp
)
target_link_libraries(plugin-scanner
  juce_lib
  juce_clap_hosting_lib
)
target_include_directories(plugin-scanner
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/src)
install(TARGETS plugin-scanner RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
