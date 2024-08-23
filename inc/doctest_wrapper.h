// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __DOCTEST_WRAPPER_H__
#define __DOCTEST_WRAPPER_H__

#include <doctest/doctest.h>

#ifndef REQUIRE_NONNULL
#  define REQUIRE_NONNULL(ptr) REQUIRE_NE (ptr, nullptr)
#endif

#ifndef REQUIRE_NULL
#  define REQUIRE_NULL(ptr) REQUIRE_EQ (ptr, nullptr)
#endif

#ifndef REQUIRE_EMPTY
#  define REQUIRE_EMPTY(ptr) REQUIRE ((ptr).empty ())
#endif

#ifndef REQUIRE_NONEMPTY
#  define REQUIRE_NONEMPTY(ptr) REQUIRE (!(ptr).empty ())
#endif

#ifndef REQUIRE_SIZE_EQ
#  define REQUIRE_SIZE_EQ(ptr, _size) REQUIRE_EQ ((ptr).size (), _size)
#endif

#ifndef REQUIRE_POSITION_EQ
#  define REQUIRE_POSITION_EQ(a, b) \
    REQUIRE ((a).ticks_ == doctest::Approx ((b).ticks_).epsilon (0.0001)); \
    REQUIRE ((a).frames_ == (b).frames_)
#endif

#ifndef REQUIRE_FLOAT_EQ
#  define REQUIRE_FLOAT_EQ(a, b) REQUIRE (a == doctest::Approx (b))
#endif

#ifndef REQUIRE_FLOAT_NEAR
#  define REQUIRE_FLOAT_NEAR(a, b, _epsilon) \
    REQUIRE (a == doctest::Approx (b).epsilon (_epsilon))
#endif

#ifndef REQUIRE_OPTIONAL_HAS_VALUE
#  define REQUIRE_OPTIONAL_HAS_VALUE(opt) REQUIRE (opt.has_value ())
#endif

#ifndef REQUIRE_UNREACHABLE
#  define REQUIRE_UNREACHABLE() REQUIRE (false)
#endif

#endif // __DOCTEST_WRAPPER_H__