// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "plugins/plugin_format_utils.h"

namespace zrythm::plugins
{

juce::Time
effective_modification_time (const juce::File &plugin_file)
{
  if (!plugin_file.isDirectory ())
    return plugin_file.getLastModificationTime ();

  juce::Time newest;
  for (
    const auto &entry : juce::RangedDirectoryIterator (
      plugin_file, true, "*", juce::File::findFiles))
    {
      newest = std::max (newest, entry.getFile ().getLastModificationTime ());
    }
  return newest;
}

} // namespace zrythm::plugins
