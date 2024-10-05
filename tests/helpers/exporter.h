// SPDX-FileCopyrightText: © 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Exporter helper.
 */

#ifndef __TEST_HELPERS_EXPORTER_H__
#define __TEST_HELPERS_EXPORTER_H__

#include "zrythm-test-config.h"

#include "common/dsp/exporter.h"

/**
 * Convenient quick export.
 */
std::string
test_exporter_export_audio (Exporter::TimeRange time_range, Exporter::Mode mode);

#endif /* __TEST_HELPERS_EXPORTER_H__ */
