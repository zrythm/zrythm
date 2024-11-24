/*
 * SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include <cstdlib>
#include <format>

#include "juce_wrapper.h"
#include "utils/env.h"
#include <fmt/format.h>

/**
 * Returns an int for the given environment variable
 * if it exists and is valid, otherwise returns the
 * default int.
 *
 * @param def Default value to return if not found.
 */
int
env_get_int (const char * key, int def)
{
  auto str =
    juce::SystemStats::getEnvironmentVariable (key, fmt::format ("{}", def));
  return str.getIntValue ();
}
