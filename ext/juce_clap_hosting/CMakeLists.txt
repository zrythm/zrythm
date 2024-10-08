cmake_minimum_required(VERSION 3.21)
project (juce_clap_hosting VERSION 0.0.1)

get_directory_property(parent_dir PARENT_DIRECTORY)
if("${parent_dir}" STREQUAL "")
    set(is_toplevel 1)
else()
    set(is_toplevel 0)
endif()

if(${is_toplevel})
    include(cmake/CPM.cmake)

    # download JUCE
    CPMAddPackage(
      NAME juce
      GITHUB_REPOSITORY juce-framework/juce
      GIT_TAG 7.0.2
    )

    add_subdirectory(examples)
else()
    juce_add_module(.)
endif()
