# Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

cmake_minimum_required(VERSION 3.5)

project(TYPE_SAFE)

include(external/external.cmake)

# options
if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(_type_safe_default_assertions ON)
else()
    set(_type_safe_default_assertions OFF)
endif()

option(TYPE_SAFE_ENABLE_ASSERTIONS "whether or not to enable internal assertions for the type_safe library" ${_type_safe_default_assertions})
if(${TYPE_SAFE_ENABLE_ASSERTIONS})
    set(_type_safe_enable_assertions 1)
else()
    set(_type_safe_enable_assertions 0)
endif()

option(TYPE_SAFE_ENABLE_PRECONDITION_CHECKS "whether or not to enable precondition checks" ON)
if(${TYPE_SAFE_ENABLE_PRECONDITION_CHECKS})
    set(_type_safe_enable_precondition_checks 1)
else()
    set(_type_safe_enable_precondition_checks 0)
endif()

option(TYPE_SAFE_ENABLE_WRAPPER "whether or not the wrappers in types.hpp are used" ON)
if(${TYPE_SAFE_ENABLE_WRAPPER})
    set(_type_safe_enable_wrapper 1)
else()
    set(_type_safe_enable_wrapper 0)
endif()

set(TYPE_SAFE_ARITHMETIC_POLICY "ub" CACHE STRING "which policy (ub/checked/default) is going to be used")
option(TYPE_SAFE_ARITHMETIC_UB "deprecated" ON)

if(NOT ${TYPE_SAFE_ARITHMETIC_UB})
    set(_type_safe_arithmetic_policy 0)
    message(DEPRECATION "option TYPE_SAFE_ARITHMETIC_UB is deprecated, use TYPE_SAFE_ARITHMETIC_POLICY instead")
    message(STATUS "arithmetic_policy_default is default_arithmetic")
elseif(${TYPE_SAFE_ARITHMETIC_POLICY} STREQUAL "ub")
    set(_type_safe_arithmetic_policy 1)
    message(STATUS "arithmetic_policy_default is undefined_behavior_arithmetic")
elseif(${TYPE_SAFE_ARITHMETIC_POLICY} STREQUAL "checked")
    set(_type_safe_arithmetic_policy 2)
    message(STATUS "arithmetic_policy_default is checked_arithmetic")
else()
    set(_type_safe_arithmetic_policy 0)
    message(STATUS "arithmetic_policy_default is default_arithmetic")
endif()

option(TYPE_SAFE_IMPORT_STD_MODULE "Build using the standard library modules rather than #include" OFF)

# interface target
set(detail_header_files
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/detail/aligned_union.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/detail/all_of.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/detail/assert.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/detail/assign_or_construct.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/detail/constant_parser.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/detail/copy_move_control.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/detail/force_inline.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/detail/is_nothrow_swappable.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/detail/map_invoke.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/detail/variant_impl.hpp)
set(header_files
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/config.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/arithmetic_policy.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/boolean.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/bounded_type.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/compact_optional.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/constrained_type.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/deferred_construction.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/downcast.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/flag.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/flag_set.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/floating_point.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/index.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/integer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/narrow_cast.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/optional.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/optional_ref.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/output_parameter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/reference.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/strong_typedef.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/tagged_union.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/variant.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/visitor.hpp)

add_library(type_safe INTERFACE)
target_sources(type_safe INTERFACE "$<BUILD_INTERFACE:${detail_header_files};${header_files}>")
target_include_directories(type_safe INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>)
target_include_directories(type_safe SYSTEM INTERFACE $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)
target_compile_definitions(type_safe INTERFACE
                                     TYPE_SAFE_ENABLE_ASSERTIONS=${_type_safe_enable_assertions}
                                     TYPE_SAFE_ENABLE_PRECONDITION_CHECKS=${_type_safe_enable_precondition_checks}
                                     TYPE_SAFE_ENABLE_WRAPPER=${_type_safe_enable_wrapper}
                                     TYPE_SAFE_ARITHMETIC_POLICY=${_type_safe_arithmetic_policy})
target_link_libraries(type_safe INTERFACE debug_assert)


if(TYPE_SAFE_IMPORT_STD_MODULE)
    target_compile_definitions(type_safe INTERFACE TYPE_SAFE_IMPORT_STD_MODULE)

    # If a user has set TYPE_SAFE_IMPORT_STD_MODULE to ON this indicates they want the type_safe headers to import std
    # This is a C++ 23 feature so we require them to use C++ in their build
    target_compile_features(type_safe INTERFACE cxx_std_23)
endif()

if(MSVC)
   target_compile_options(type_safe INTERFACE /wd4800) # truncation to bool warning
endif()

# Setup package config
include( CMakePackageConfigHelpers )
set(CONFIG_PACKAGE_INSTALL_DIR lib/cmake/type_safe)

set(TYPE_SAFE_CMAKE_SIZEOF_VOID_P ${CMAKE_SIZEOF_VOID_P})
set(CMAKE_SIZEOF_VOID_P "")
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/type_safe-config-version.cmake
  VERSION 0.2.4
  COMPATIBILITY SameMajorVersion
)
set(CMAKE_SIZEOF_VOID_P ${TYPE_SAFE_CMAKE_SIZEOF_VOID_P})

# Install target
install(DIRECTORY include/type_safe DESTINATION include)

# Only export target when using imported targets
if(TYPE_SAFE_HAS_IMPORTED_TARGETS)
    install(TARGETS type_safe
        EXPORT type_safe-targets
        DESTINATION lib)

    install( EXPORT type_safe-targets
      DESTINATION
        ${CONFIG_PACKAGE_INSTALL_DIR}
    )

    install( FILES
      ${CMAKE_CURRENT_SOURCE_DIR}/type_safe-config.cmake
      ${CMAKE_CURRENT_BINARY_DIR}/type_safe-config-version.cmake
      DESTINATION
        ${CONFIG_PACKAGE_INSTALL_DIR} )

endif()

# other subdirectories
# only add if not inside add_subdirectory()
option(TYPE_SAFE_BUILD_TEST_EXAMPLE "build test and example" ON)
if(${TYPE_SAFE_BUILD_TEST_EXAMPLE} AND (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
    enable_testing()
    add_subdirectory(example/)
    add_subdirectory(test/)
endif()

if(TYPE_SAFE_IMPORT_STD_MODULE)
    add_subdirectory(test_std_module/)
endif()

option(TYPE_SAFE_BUILD_DOC "generate documentation" OFF)
if(TYPE_SAFE_BUILD_DOC)
    add_subdirectory(doc/)
endif()
