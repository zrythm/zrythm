// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

// Shim redirecting the sampler fixture's libsamplerate usage to soxr's
// libsamplerate-compatible bindings, so the vendored eg-sampler sources
// stay unmodified.

#include <soxr-lsr.h>
