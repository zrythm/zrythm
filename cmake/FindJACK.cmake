# SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

find_package(PkgConfig REQUIRED)

add_library(zrythm_jack_lib INTERFACE)

if (WIN32 OR ${ZRYTHM_IS_INSTALLER_VER})
  set(USE_WEAK_JACK ON)
else()
  set(USE_WEAK_JACK OFF)
endif()

# prefer jack1
pkg_check_modules(JACK IMPORTED_TARGET jack<1.0)
if(NOT JACK_FOUND)
  pkg_check_modules(JACK REQUIRED IMPORTED_TARGET jack>=1.0)
  set(HAVE_JACK2 ON)
endif()
set(HAVE_JACK ON)

if(USE_WEAK_JACK)
  target_include_directories(zrythm_jack_lib INTERFACE ${JACK_INCLUDE_DIRS})
  target_link_libraries(zrythm_jack_lib INTERFACE weakjack::weakjack)
else()
  target_link_libraries(zrythm_jack_lib INTERFACE PkgConfig::JACK)
endif()

set(CMAKE_REQUIRED_INCLUDES ${JACK_INCLUDE_DIRS})
set(CMAKE_REQUIRED_LIBRARIES ${JACK_LIBRARIES})
check_library_exists("${JACK_LIBRARIES}" jack_set_property "" HAVE_JACK_SET_PROPERTY)
check_library_exists("${JACK_LIBRARIES}" jack_port_type_get_buffer_size "" HAVE_JACK_PORT_TYPE_GET_BUFFER_SIZE)

target_compile_definitions(zrythm_jack_lib
  INTERFACE
  HAVE_JACK=$<BOOL:${HAVE_JACK}>
  HAVE_JACK2=$<BOOL:${HAVE_JACK2}>
  HAVE_JACK_SET_PROPERTY=$<BOOL:${HAVE_JACK_SET_PROPERTY}>
  HAVE_JACK_PORT_TYPE_GET_BUFFER_SIZE=$<BOOL:${HAVE_JACK_PORT_TYPE_GET_BUFFER_SIZE}>
)

add_library(zrythm::jack ALIAS zrythm_jack_lib)