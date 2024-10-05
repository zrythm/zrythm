/*
 * SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include <cstdlib>
#include <format>

#include <glib.h>

#include "common/utils/env.h"
#include "juce/juce.h"

/**
 * Returns a newly allocated string.
 *
 * @param def Default value to return if not found.
 */
char *
env_get_string (const char * key, const char * def)
{
  const char * val = g_getenv (key);
  if (!val)
    return g_strdup (def);
  return g_strdup (val);
}

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
