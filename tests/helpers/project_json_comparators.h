// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <array>
#include <initializer_list>
#include <map>
#include <string>
#include <string_view>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::test_helpers
{

/**
 * @brief Verifies that all registry sizes and IDs are equal between two
 * serialized projects.
 *
 * Compares port, param, plugin, track, arranger object, and file audio source
 * registries. Catches duplication bugs where deserialization creates extra
 * entries.
 *
 * @param registries_to_skip A list of registries to skip (FIXME:
 * actually fix issues in tests that fail without this and eventually
 * remove this).
 */
inline void
expect_registries_match (
  const nlohmann::json                   &j1,
  const nlohmann::json                   &j2,
  std::initializer_list<std::string_view> registries_to_skip = {})
{
  const auto &regs1 = j1["projectData"]["registries"];
  const auto &regs2 = j2["projectData"]["registries"];

  std::vector<std::string_view> registry_names = {
    "portRegistry",  "paramRegistry",          "pluginRegistry",
    "trackRegistry", "arrangerObjectRegistry", "fileAudioSourceRegistry",
  };
  for (const auto &reg : registries_to_skip)
    {
      std::erase (registry_names, reg);
    }

  for (const auto &name : registry_names)
    {
      const auto &reg1 = regs1[name];
      const auto &reg2 = regs2[name];

      EXPECT_EQ (reg1.size (), reg2.size ())
        << "Registry " << name << " size mismatch";

      // Build map of ID -> object for each registry
      std::map<std::string, nlohmann::json> objs1, objs2;
      for (const auto &obj : reg1)
        objs1[obj["id"].get<std::string> ()] = obj;
      for (const auto &obj : reg2)
        objs2[obj["id"].get<std::string> ()] = obj;

      // Find first mismatch
      for (const auto &[id, obj] : objs1)
        {
          if (!objs2.contains (id))
            {
              FAIL ()
                << "Registry " << name << ": object from j1 missing in j2:\n"
                << obj.dump (2);
            }
        }
      for (const auto &[id, obj] : objs2)
        {
          if (!objs1.contains (id))
            {
              FAIL ()
                << "Registry " << name << ": object from j2 missing in j1:\n"
                << obj.dump (2);
            }
        }
    }
}

} // namespace zrythm::test_helpers
