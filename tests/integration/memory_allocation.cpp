// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "tests/helpers/zrythm_helper.h"

TEST_SUITE_BEGIN ("integration/memory allocation");

/**
 * Verify that memory allocated by the fixture is free'd by cleanup().
 *
 * This is intended to be ran with a memory leak detector.
 */
TEST_CASE_FIXTURE (ZrythmFixture, "memory allocation") { }

TEST_SUITE_END;
