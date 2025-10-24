# SPDX-FileCopyrightText: Nuno Fachada
# SPDX-License-Identifier: CC0-1.0
#
# Taken from https://github.com/nunofachada/cmake-git-semver
#
# This cmake module sets the project version and partial version
# variables by analysing the git tag and commit history. It expects git
# tags defined with semantic versioning 2.0.0 (http://semver.org/).
#
# The module expects the PROJECT_NAME variable to be set, and recognizes
# the GIT_FOUND, GIT_EXECUTABLE and VERSION_UPDATE_FROM_GIT variables.
# If Git is found and VERSION_UPDATE_FROM_GIT is set to boolean TRUE,
# the project version will be updated using information fetched from the
# most recent git tag and commit. Otherwise, the module will try to read
# a VERSION file containing the full and partial versions. The module
# will update this file each time the project version is updated.
#
# Once done, this module will define the following variables:
#
# ${PROJECT_NAME}_VERSION_STRING - Version string without metadata
# such as "v2.0.0" or "v1.2.41-beta.1". This should correspond to the
# most recent git tag.
# ${PROJECT_NAME}_VERSION_STRING_FULL - Version string with metadata
# such as "v2.0.0+3.a23fbc" or "v1.3.1-alpha.2+4.9c4fd1"
# ${PROJECT_NAME}_VERSION - Same as ${PROJECT_NAME}_VERSION_STRING,
# without the preceding 'v', e.g. "2.0.0" or "1.2.41-beta.1"
# ${PROJECT_NAME}_VERSION_MAJOR - Major version integer (e.g. 2 in v2.3.1-RC.2+21.ef12c8)
# ${PROJECT_NAME}_VERSION_MINOR - Minor version integer (e.g. 3 in v2.3.1-RC.2+21.ef12c8)
# ${PROJECT_NAME}_VERSION_PATCH - Patch version integer (e.g. 1 in v2.3.1-RC.2+21.ef12c8)
# ${PROJECT_NAME}_VERSION_TWEAK - Tweak version string (e.g. "RC.2" in v2.3.1-RC.2+21.ef12c8)
# ${PROJECT_NAME}_VERSION_AHEAD - How many commits ahead of last tag (e.g. 21 in v2.3.1-RC.2+21.ef12c8)
# ${PROJECT_NAME}_VERSION_GIT_SHA - The git sha1 of the most recent commit (e.g. the "ef12c8" in v2.3.1-RC.2+21.ef12c8)
#
# This module is public domain, use it as it fits you best.
#
# Author: Nuno Fachada

set(should_use_git GIT_FOUND AND VERSION_UPDATE_FROM_GIT)

if(should_use_git)

  # Get last tag from git
  execute_process(COMMAND ${GIT_EXECUTABLE} describe --abbrev=0 --tags
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE ${PROJECT_NAME}_VERSION_STRING
    ERROR_VARIABLE GIT_DESCRIBE_ERROR
    RESULT_VARIABLE GIT_DESCRIBE_RESULT
    OUTPUT_STRIP_TRAILING_WHITESPACE)

endif()

# if we can look at git tags
if (should_use_git AND GIT_DESCRIBE_RESULT EQUAL 0)

  # Get the current branch name (will return HEAD if not on a branch, which also works for the next command)
  execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE CURRENT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  # How many commits since last tag
  execute_process(COMMAND ${GIT_EXECUTABLE} rev-list ${CURRENT_BRANCH} ${${PROJECT_NAME}_VERSION_STRING}^..HEAD --count
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE ${PROJECT_NAME}_VERSION_AHEAD
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  # Get current commit SHA from git
  execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short=12 HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE ${PROJECT_NAME}_VERSION_GIT_SHA
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  # Get partial versions into a list
  string(REGEX MATCHALL "-.*$|[0-9]+" ${PROJECT_NAME}_PARTIAL_VERSION_LIST
    ${${PROJECT_NAME}_VERSION_STRING})

  # Set the version numbers
  list(GET ${PROJECT_NAME}_PARTIAL_VERSION_LIST
    0 ${PROJECT_NAME}_VERSION_MAJOR)
  list(GET ${PROJECT_NAME}_PARTIAL_VERSION_LIST
    1 ${PROJECT_NAME}_VERSION_MINOR)
  list(GET ${PROJECT_NAME}_PARTIAL_VERSION_LIST
    2 ${PROJECT_NAME}_VERSION_PATCH)

  # The tweak part is optional, so check if the list contains it
  list(LENGTH ${PROJECT_NAME}_PARTIAL_VERSION_LIST
    ${PROJECT_NAME}_PARTIAL_VERSION_LIST_LEN)
  if (${PROJECT_NAME}_PARTIAL_VERSION_LIST_LEN GREATER 3)
    list(GET ${PROJECT_NAME}_PARTIAL_VERSION_LIST 3 ${PROJECT_NAME}_VERSION_TWEAK)
    string(SUBSTRING ${${PROJECT_NAME}_VERSION_TWEAK} 1 -1 ${PROJECT_NAME}_VERSION_TWEAK)
  endif()

  # Unset the list
  unset(${PROJECT_NAME}_PARTIAL_VERSION_LIST)

  # Set full project version string
  set(${PROJECT_NAME}_VERSION_STRING_FULL
    ${${PROJECT_NAME}_VERSION_STRING}+${${PROJECT_NAME}_VERSION_AHEAD}.${${PROJECT_NAME}_VERSION_GIT_SHA})

else()

  # Git not available, get version from VERSION.txt file
  file(STRINGS ${CMAKE_SOURCE_DIR}/VERSION.txt VERSION_CONTENT)

  # Parse the version string
  string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)(-([a-zA-Z0-9.]+))?" VERSION_MATCH ${VERSION_CONTENT})

  set(${PROJECT_NAME}_VERSION_STRING ${VERSION_CONTENT})
  set(${PROJECT_NAME}_VERSION_MAJOR ${CMAKE_MATCH_1})
  set(${PROJECT_NAME}_VERSION_MINOR ${CMAKE_MATCH_2})
  set(${PROJECT_NAME}_VERSION_PATCH ${CMAKE_MATCH_3})
  set(${PROJECT_NAME}_VERSION_TWEAK ${CMAKE_MATCH_5})

  # Set full version string (without commit info)
  set(${PROJECT_NAME}_VERSION_STRING_FULL ${VERSION_CONTENT})

  # Set placeholder values for git-specific information
  set(${PROJECT_NAME}_VERSION_AHEAD "0")
  set(${PROJECT_NAME}_VERSION_GIT_SHA "unknown")

endif()

# Set project version (without the preceding 'v')
set(${PROJECT_NAME}_VERSION ${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.${${PROJECT_NAME}_VERSION_PATCH})
if (${PROJECT_NAME}_VERSION_TWEAK)
  set(${PROJECT_NAME}_VERSION ${${PROJECT_NAME}_VERSION}-${${PROJECT_NAME}_VERSION_TWEAK})
endif()
