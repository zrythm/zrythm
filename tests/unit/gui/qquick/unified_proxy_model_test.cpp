// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/unified_proxy_model.h"

#include <QStringListModel>

#include <gtest/gtest.h>

using namespace Qt::StringLiterals;

namespace zrythm::gui
{

TEST (UnifiedProxyModelTest, MapFromSourceAutoRegistersModel)
{
  UnifiedProxyModel unified;
  QStringListModel  source ({ u"a"_s, u"b"_s });

  // No explicit addSourceModel() call - the model must be registered on
  // first mapping.
  const auto mapped = unified.mapFromSource (source.index (1, 0));
  ASSERT_TRUE (mapped.isValid ());
  EXPECT_EQ (mapped.row (), 1);
}

TEST (UnifiedProxyModelTest, MapFromSourceAppendsAfterExistingSources)
{
  UnifiedProxyModel unified;
  QStringListModel  source1 ({ u"a"_s });
  QStringListModel  source2 ({ u"b"_s, u"c"_s });

  unified.addSourceModel (&source1);

  const auto mapped = unified.mapFromSource (source2.index (1, 0));
  ASSERT_TRUE (mapped.isValid ());
  EXPECT_EQ (mapped.row (), 2);
}

TEST (UnifiedProxyModelTest, MapFromSourceInvalidReturnsInvalid)
{
  UnifiedProxyModel unified;
  EXPECT_FALSE (unified.mapFromSource ({}).isValid ());
}

TEST (UnifiedProxyModelTest, MapFromSourceRoundTrip)
{
  UnifiedProxyModel unified;
  QStringListModel  source ({ u"a"_s });

  const auto mapped = unified.mapFromSource (source.index (0, 0));
  ASSERT_TRUE (mapped.isValid ());

  const auto source_index = unified.mapToSource (mapped);
  ASSERT_TRUE (source_index.isValid ());
  EXPECT_EQ (source_index.model (), &source);
  EXPECT_EQ (source_index.row (), 0);
}

} // namespace zrythm::gui
