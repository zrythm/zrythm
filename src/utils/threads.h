// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <string>

namespace zrythm::utils
{

/**
 * @brief Returns the name of the current thread as set by the OS or
 * threading library.
 *
 * Falls back to the numeric thread ID if no name is available.
 */
std::string
get_current_thread_name ();

} // namespace zrythm::utils
