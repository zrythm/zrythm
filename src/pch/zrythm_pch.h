// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __PCH_ZRYTHM_PCH_H__
#define __PCH_ZRYTHM_PCH_H__

#ifdef _WIN32
#  include <windows.h>
#endif

#undef ERROR // windows.h defines ERROR unless NOGDI is defined

#include <algorithm>
#include <atomic>
#include <cinttypes>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <ranges>
#include <semaphore>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include <glib/gi18n.h>

#include "libadwaita_wrapper.h"
#include <glibmm.h>

#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wshadow"
#endif
#include <magic_enum_all.hpp>
#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

#include "utils/gtest_wrapper.h"

#include <fmt/format.h>
#include <fmt/printf.h>

/* This also includes all native platform headers. */
/* FIXME compilation fails with internal compiler errors if this is included */
// #include "ext/juce/juce.h"

#endif /* __PCH_ZRYTHM_PCH_H__ */
