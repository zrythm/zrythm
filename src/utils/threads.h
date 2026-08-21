// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstddef>
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

/**
 * @brief Returns the stack size to request for a realtime worker thread.
 *
 * Returns @p base_bytes plus any platform-specific padding needed for static
 * TLS and PTHREAD_STACK_MIN accounting (glibc only; @p base_bytes is
 * returned unchanged elsewhere).
 */
size_t
rt_worker_stack_size (size_t base_bytes);

} // namespace zrythm::utils
