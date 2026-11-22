// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
#pragma once

#include "utils/object_registry.h"

namespace zrythm::gui
{

/**
 * @brief Application-scope state.
 *
 * Owns the application registry, where state that outlives projects
 * (the control room's monitor fader and level parameters, the
 * metronome) registers.
 */
class AppScope
{
public:
  AppScope () = default;
  AppScope (const AppScope &) = delete;
  AppScope &operator= (const AppScope &) = delete;

  utils::ObjectRegistry &registry () { return registry_; }

private:
  utils::ObjectRegistry registry_;
};

} // namespace zrythm::gui
