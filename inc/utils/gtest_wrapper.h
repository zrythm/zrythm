// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __ZRYTHM_UTILS_GTEST_WRAPPER_H__
#define __ZRYTHM_UTILS_GTEST_WRAPPER_H__

#include <gtest/gtest.h>

extern bool g_running_in_test;

bool
get_running_in_test ();

void
set_running_in_test (bool value);

#define ZRYTHM_TESTING (get_running_in_test ())

#ifndef REQUIRE
#  define REQUIRE(condition) ASSERT_TRUE (condition)
#endif

#ifndef EXPECT_NONNULL
#  define EXPECT_NONNULL(ptr) EXPECT_NE (ptr, nullptr)
#endif

#ifndef ASSERT_NONNULL
#  define ASSERT_NONNULL(ptr) ASSERT_NE (ptr, nullptr)
#endif

#ifndef ASSERT_NULL
#  define ASSERT_NULL(ptr) ASSERT_EQ (ptr, nullptr)
#endif

#ifndef ASSERT_EMPTY
#  define ASSERT_EMPTY(ptr) ASSERT_TRUE ((ptr).empty ())
#endif

#ifndef ASSERT_NONEMPTY
#  define ASSERT_NONEMPTY(ptr) ASSERT_TRUE (!(ptr).empty ())
#endif

#ifndef EXPECT_NONEMPTY
#  define EXPECT_NONEMPTY(ptr) EXPECT_TRUE (!(ptr).empty ())
#endif

#ifndef ASSERT_SIZE_EQ
#  define ASSERT_SIZE_EQ(ptr, _size) ASSERT_EQ ((ptr).size (), _size)
#endif

constexpr double POSITION_EQ_EPSILON = 0.0001;

#ifndef ASSERT_POSITION_EQ
#  define ASSERT_POSITION_EQ(a, b) \
    ASSERT_NEAR ((a).ticks_, (b).ticks_, 0.0001); \
    ASSERT_TRUE ((a).frames_ == (b).frames_)
#endif

#ifndef ASSERT_FLOAT_EQ
#  define ASSERT_FLOAT_EQ(a, b) ASSERT_NEAR (a, b, FLT_EPSILON)
#endif

#ifndef ASSERT_HAS_VALUE
#  define ASSERT_HAS_VALUE(opt) ASSERT_TRUE ((opt).has_value ())
#endif

#ifndef EXPECT_HAS_VALUE
#  define EXPECT_HAS_VALUE(opt) EXPECT_TRUE ((opt).has_value ())
#endif

#ifndef ASSERT_UNREACHABLE
#  define ASSERT_UNREACHABLE() ASSERT_TRUE (false)
#endif

#ifndef EXPECT_UNREACHABLE
#  define EXPECT_UNREACHABLE() EXPECT_TRUE (false)
#endif

#endif // __ZRYTHM_UTILS_GTEST_WRAPPER_H__