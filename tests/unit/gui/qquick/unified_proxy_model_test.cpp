// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/unified_proxy_model.h"

#include <QItemSelectionModel>
#include <QStringListModel>

#include <gtest/gtest.h>

using namespace Qt::StringLiterals;

namespace zrythm::gui
{

// Registration is explicit: an index from a model that was never
// registered maps to an invalid index and does not register the model.
TEST (UnifiedProxyModelTest, MapFromSourceUnregisteredModelReturnsInvalid)
{
  UnifiedProxyModel unified;
  QStringListModel  source ({ u"a"_s, u"b"_s });

  EXPECT_FALSE (unified.mapFromSource (source.index (1, 0)).isValid ());
  EXPECT_EQ (unified.rowCount (), 0);
}

TEST (UnifiedProxyModelTest, MapFromSourceOffsetsAcrossSources)
{
  UnifiedProxyModel unified;
  QStringListModel  source1 ({ u"a"_s });
  QStringListModel  source2 ({ u"b"_s, u"c"_s });

  unified.addSourceModel (&source1);
  unified.addSourceModel (&source2);

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

  unified.addSourceModel (&source);

  const auto mapped = unified.mapFromSource (source.index (0, 0));
  ASSERT_TRUE (mapped.isValid ());

  const auto source_index = unified.mapToSource (mapped);
  ASSERT_TRUE (source_index.isValid ());
  EXPECT_EQ (source_index.model (), &source);
  EXPECT_EQ (source_index.row (), 0);
}

// A source model registered after another observer connected to it (the
// order QML views produce when the model enters the unified model lazily)
// leaves a window while the source is mid-removal: the unified model's
// cached row count still includes the removed rows while the source's own
// row count already shrank. Selection ranges are left pointing at the old
// rows during that window, so reading selection state looks up rows no
// source model provides anymore. Those rows must resolve to an invalid
// index (like any flat model returns for out-of-range rows) instead of
// crashing, and selection state must stay readable throughout the removal.
TEST (UnifiedProxyModelTest, SelectionReadableWhileLateSourceModelIsMidRemoval)
{
  UnifiedProxyModel unified;
  QStringListModel  source1 ({ u"a"_s, u"b"_s });
  unified.addSourceModel (&source1);

  QItemSelectionModel selection (&unified);

  QStringListModel source2 ({ u"c"_s, u"d"_s, u"e"_s });

  // Observer connected to source2's removal signal BEFORE the unified
  // model registers source2 (a QML view over source2 connects when the
  // view is created, which precedes any lazy registration)
  bool selection_during_removal = false;
  QObject::connect (
    &source2, &QAbstractItemModel::rowsRemoved, &selection,
    [&selection, &selection_during_removal] () {
      selection_during_removal = selection.hasSelection ();
    });

  unified.addSourceModel (&source2);

  // Select source2's last row "e" (unified row 4); removing source2's
  // first row shifts "e" down, so during the removal window the selected
  // range still references unified row 4 while source2 only provides two
  // rows (unified rows 2 and 3) — row 4 is the gap
  selection.select (unified.index (4, 0), QItemSelectionModel::Select);
  ASSERT_TRUE (selection.hasSelection ());

  source2.removeRows (0, 1);

  // During the removal the selected row is in the gap, so it resolves to
  // no selected item; once the removal settles, "e" remains selected at
  // its new unified row
  EXPECT_FALSE (selection_during_removal);
  EXPECT_TRUE (selection.hasSelection ());
  EXPECT_TRUE (selection.isSelected (unified.index (3, 0)));
}

// Repeated registrations of the same source model are counted: the model
// becomes a source once and stays a source until the last registration is
// removed.
TEST (UnifiedProxyModelTest, RepeatedRegistrationsAreCounted)
{
  UnifiedProxyModel unified;
  QStringListModel  source ({ u"a"_s, u"b"_s });

  unified.addSourceModel (&source);
  unified.addSourceModel (&source);
  EXPECT_EQ (unified.rowCount (), 2);
  EXPECT_EQ (unified.sourceModels ().size (), 1);

  unified.removeSourceModel (&source);
  EXPECT_EQ (unified.rowCount (), 2);
  EXPECT_TRUE (unified.sourceModels ().contains (&source));

  unified.removeSourceModel (&source);
  EXPECT_EQ (unified.rowCount (), 0);
  EXPECT_FALSE (unified.sourceModels ().contains (&source));
}

// Removing a model that was never registered is refused without changing
// the unified model.
TEST (UnifiedProxyModelTest, RemoveUnregisteredModelIsRefused)
{
  UnifiedProxyModel unified;
  QStringListModel  registered_source ({ u"a"_s });
  unified.addSourceModel (&registered_source);

  QStringListModel unregistered_source ({ u"b"_s });
  unified.removeSourceModel (&unregistered_source);
  EXPECT_EQ (unified.rowCount (), 1);
  EXPECT_TRUE (unified.sourceModels ().contains (&registered_source));
}

// A model registered by several sibling views stays registered while any
// of them holds its registration, so selections on its rows survive one
// sibling dropping out.
TEST (UnifiedProxyModelTest, ModelStaysRegisteredWhileAnyRegistrationRemains)
{
  UnifiedProxyModel   unified;
  QItemSelectionModel selection (&unified);
  QStringListModel    source ({ u"a"_s, u"b"_s });

  unified.addSourceModel (&source);
  unified.addSourceModel (&source);

  selection.select (unified.index (0, 0), QItemSelectionModel::Select);
  selection.select (unified.index (1, 0), QItemSelectionModel::Select);
  ASSERT_TRUE (selection.hasSelection ());

  unified.removeSourceModel (&source);
  EXPECT_EQ (unified.rowCount (), 2);
  EXPECT_TRUE (selection.hasSelection ());
  EXPECT_TRUE (selection.isSelected (unified.index (1, 0)));
}

} // namespace zrythm::gui
