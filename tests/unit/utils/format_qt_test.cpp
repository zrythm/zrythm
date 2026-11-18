// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/format_qt.h"

#include <QStringList>
#include <QVariant>

#include <fmt/format.h>
#include <gtest/gtest.h>

namespace
{

TEST (FormatQtTest, FormatsStringListVariantJoined)
{
  const auto variant = QVariant::fromValue (QStringList () << "a" << "b");
  EXPECT_EQ (fmt::format ("{}", variant), "a, b");
}

TEST (FormatQtTest, FormatsSingleStringListElementWithoutSeparator)
{
  const auto variant = QVariant::fromValue (QStringList () << "/tmp/x");
  EXPECT_EQ (fmt::format ("{}", variant), "/tmp/x");
}

TEST (FormatQtTest, FormatsStringVariant)
{
  EXPECT_EQ (fmt::format ("{}", QVariant ("x")), "x");
}

} // namespace
