// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Memory utils.
 */

#pragma once

#include <cstddef>

void *
z_realloc (void * ptr, size_t size);

void
z_free_strv (char ** strv);
