// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <filesystem>
#include <span>

namespace zrythm::frontends::gui
{

/**
 * @brief Returns whether a portal file chooser backend is installed
 * under the given XDG data directories.
 *
 * @param xdg_data_dirs Directories searched for portal backend
 * descriptor files in xdg-desktop-portal/portals.
 * @return True when at least one backend descriptor lists the file
 * chooser interface.
 */
[[nodiscard]] bool
file_chooser_backend_installed (
  std::span<const std::filesystem::path> xdg_data_dirs);

/**
 * @brief Returns whether a portal file chooser backend is installed
 * in this session.
 *
 * Callable before the application is constructed: the platform theme
 * is selected before that.
 *
 * @return True when a backend descriptor under the XDG data directories
 * lists the file chooser interface. Whether the portal daemon selects
 * that backend depends on its own configuration.
 */
[[nodiscard]] bool
portal_file_dialogs_available ();

}
