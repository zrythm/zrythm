// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/timebase.h"

#include <QSignalSpy>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{

class TimebaseProviderTest
    : public ::testing::Test,
      private test_helpers::ScopedQCoreApplication
{
};

TEST_F (TimebaseProviderTest, DefaultsToMusical)
{
  TimebaseProvider provider;
  EXPECT_FALSE (provider.hasOverride ());
  EXPECT_EQ (provider.effectiveTimebase (), Timebase::Musical);
}

TEST_F (TimebaseProviderTest, OverrideForcesValue)
{
  TimebaseProvider provider;
  provider.setOverride (Timebase::Absolute);
  EXPECT_EQ (provider.effectiveTimebase (), Timebase::Absolute);
}

TEST_F (TimebaseProviderTest, ClearOverrideReturnsToDefault)
{
  TimebaseProvider provider;
  provider.setOverride (Timebase::Absolute);
  provider.clearOverride ();
  EXPECT_EQ (provider.effectiveTimebase (), Timebase::Musical);
  EXPECT_FALSE (provider.hasOverride ());
}

TEST_F (TimebaseProviderTest, InheritsFromSource)
{
  TimebaseProvider root;
  root.setOverride (Timebase::Absolute);
  TimebaseProvider leaf;
  leaf.setSource (&root);
  EXPECT_EQ (leaf.effectiveTimebase (), Timebase::Absolute);
}

TEST_F (TimebaseProviderTest, LeafOverrideBeatsSource)
{
  TimebaseProvider root;
  root.setOverride (Timebase::Absolute);
  TimebaseProvider leaf;
  leaf.setSource (&root);
  leaf.setOverride (Timebase::Musical);
  EXPECT_EQ (leaf.effectiveTimebase (), Timebase::Musical);
}

TEST_F (TimebaseProviderTest, ClearLeafOverrideReturnsToSource)
{
  TimebaseProvider root;
  root.setOverride (Timebase::Absolute);
  TimebaseProvider leaf;
  leaf.setSource (&root);
  leaf.setOverride (Timebase::Musical);
  leaf.clearOverride ();
  EXPECT_EQ (leaf.effectiveTimebase (), Timebase::Absolute);
}

TEST_F (TimebaseProviderTest, ReactsToSourceOverrideChange)
{
  TimebaseProvider root;
  root.setOverride (Timebase::Musical);
  TimebaseProvider leaf;
  leaf.setSource (&root);
  QSignalSpy spy (&leaf, &TimebaseProvider::effectiveTimebaseChanged);
  root.setOverride (Timebase::Absolute);
  EXPECT_EQ (spy.count (), 1);
  EXPECT_EQ (leaf.effectiveTimebase (), Timebase::Absolute);
}

TEST_F (TimebaseProviderTest, NoEmitWhenLeafOverrideActive)
{
  TimebaseProvider root;
  root.setOverride (Timebase::Musical);
  TimebaseProvider leaf;
  leaf.setSource (&root);
  leaf.setOverride (Timebase::Absolute);
  QSignalSpy spy (&leaf, &TimebaseProvider::effectiveTimebaseChanged);
  root.setOverride (Timebase::Absolute);
  // Override is Absolute, source now also Absolute -> no observable change.
  EXPECT_EQ (spy.count (), 0);
}

TEST_F (TimebaseProviderTest, SourceDeletionFallsBackToDefault)
{
  auto root = std::make_unique<TimebaseProvider> ();
  root->setOverride (Timebase::Absolute);
  TimebaseProvider leaf;
  leaf.setSource (root.get ());
  EXPECT_EQ (leaf.effectiveTimebase (), Timebase::Absolute);
  root.reset ();
  // QPointer clears; provider returns Musical (default) when source is gone.
  EXPECT_EQ (leaf.effectiveTimebase (), Timebase::Musical);
}

TEST_F (TimebaseProviderTest, MultiLevelChain)
{
  TimebaseProvider root;
  root.setOverride (Timebase::Absolute);
  TimebaseProvider middle;
  middle.setSource (&root);
  TimebaseProvider leaf;
  leaf.setSource (&middle);
  EXPECT_EQ (leaf.effectiveTimebase (), Timebase::Absolute);
  middle.setOverride (Timebase::Musical);
  EXPECT_EQ (leaf.effectiveTimebase (), Timebase::Musical);
  middle.clearOverride ();
  EXPECT_EQ (leaf.effectiveTimebase (), Timebase::Absolute);
}

} // namespace zrythm::dsp
