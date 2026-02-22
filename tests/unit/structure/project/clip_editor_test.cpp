// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/midi_region.h"
#include "structure/project/clip_editor.h"

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

using namespace zrythm::structure::project;
using namespace zrythm::structure::arrangement;
using namespace zrythm::structure::tracks;

class ClipEditorTest
    : public ::testing::Test,
      public test_helpers::ScopedQCoreApplication
{
protected:
  void SetUp () override
  {
    // Create registries needed by ClipEditor
    object_registry_ = std::make_unique<ArrangerObjectRegistry> ();
    track_registry_ = std::make_unique<TrackRegistry> ();

    // Create a simple track resolver
    track_resolver_ = [this] (const TrackUuid &uuid) -> TrackPtrVariant {
      return track_registry_->find_by_id_or_throw (uuid);
    };

    // Create ClipEditor
    clip_editor_ = std::make_unique<ClipEditor> (
      *object_registry_, track_resolver_, nullptr);
  }

  void TearDown () override
  {
    clip_editor_.reset ();
    track_registry_.reset ();
    object_registry_.reset ();
  }

  std::unique_ptr<ArrangerObjectRegistry> object_registry_;
  std::unique_ptr<TrackRegistry>          track_registry_;
  TrackResolver                           track_resolver_;
  std::unique_ptr<ClipEditor>             clip_editor_;
};

// ============================================================================
// Construction Tests
// ============================================================================

TEST_F (ClipEditorTest, Construction)
{
  EXPECT_NE (clip_editor_, nullptr);
  EXPECT_FALSE (clip_editor_->has_region ());
}

// ============================================================================
// Region Tests
// ============================================================================

TEST_F (ClipEditorTest, HasRegionInitiallyFalse)
{
  EXPECT_FALSE (clip_editor_->has_region ());
}

TEST_F (ClipEditorTest, RegionInitiallyNull)
{
  auto region = clip_editor_->region ();
  EXPECT_TRUE (region.isNull ());
}

TEST_F (ClipEditorTest, TrackInitiallyNull)
{
  auto track = clip_editor_->track ();
  EXPECT_TRUE (track.isNull ());
}

// ============================================================================
// Editor Components Tests
// ============================================================================

TEST_F (ClipEditorTest, PianoRollNotNull)
{
  EXPECT_NE (clip_editor_->getPianoRoll (), nullptr);
}

TEST_F (ClipEditorTest, ChordEditorNotNull)
{
  EXPECT_NE (clip_editor_->getChordEditor (), nullptr);
}

TEST_F (ClipEditorTest, AudioClipEditorNotNull)
{
  EXPECT_NE (clip_editor_->getAudioClipEditor (), nullptr);
}

TEST_F (ClipEditorTest, AutomationEditorNotNull)
{
  EXPECT_NE (clip_editor_->getAutomationEditor (), nullptr);
}

// ============================================================================
// Init Tests
// ============================================================================

TEST_F (ClipEditorTest, InitDoesNotThrow)
{
  EXPECT_NO_THROW (clip_editor_->init ());
}

TEST_F (ClipEditorTest, InitLoadedDoesNotThrow)
{
  EXPECT_NO_THROW (clip_editor_->init_loaded ());
}

// ============================================================================
// Serialization Tests
// ============================================================================

TEST_F (ClipEditorTest, JsonSerializationNoRegion)
{
  // ClipEditor with no region set
  nlohmann::json j = *clip_editor_;

  // Verify JSON contains expected keys
  EXPECT_TRUE (j.contains ("regionId"));
  EXPECT_TRUE (j.contains ("pianoRoll"));
  EXPECT_TRUE (j.contains ("automationEditor"));
  EXPECT_TRUE (j.contains ("chordEditor"));
  EXPECT_TRUE (j.contains ("audioClipEditor"));
}

TEST_F (ClipEditorTest, JsonSerializationRoundTripNoRegion)
{
  nlohmann::json j = *clip_editor_;

  auto deserialized_editor =
    std::make_unique<ClipEditor> (*object_registry_, track_resolver_, nullptr);
  j.get_to (*deserialized_editor);

  // Should still have no region
  EXPECT_FALSE (deserialized_editor->has_region ());
}

// ============================================================================
// Clone Tests
// ============================================================================

TEST_F (ClipEditorTest, InitFromNoRegion)
{
  auto cloned_editor =
    std::make_unique<ClipEditor> (*object_registry_, track_resolver_, nullptr);

  init_from (*cloned_editor, *clip_editor_, utils::ObjectCloneType::Snapshot);

  // Should have no region like the original
  EXPECT_FALSE (cloned_editor->has_region ());
}

// ============================================================================
// Set/Unset Region Tests
// ============================================================================

TEST_F (ClipEditorTest, UnsetRegionWhenNoRegionDoesNotThrow)
{
  EXPECT_NO_THROW (clip_editor_->unsetRegion ());
  EXPECT_FALSE (clip_editor_->has_region ());
}

TEST_F (ClipEditorTest, UnsetRegionEmitsSignal)
{
  bool signal_emitted = false;

  QObject::connect (
    clip_editor_.get (), &ClipEditor::regionChanged, clip_editor_.get (),
    [&] (QVariant) { signal_emitted = true; });

  clip_editor_->unsetRegion ();

  // Should not emit when there was no region
  EXPECT_FALSE (signal_emitted);
}

// ============================================================================
// Registry Reference Tests
// ============================================================================

TEST_F (ClipEditorTest, TrackResolverThrowsForNonexistentUuid)
{
  // Verify the track resolver throws for non-existent UUIDs
  TrackUuid fake_uuid (QUuid::createUuid ());
  EXPECT_THROW (track_resolver_ (fake_uuid), std::exception);
}
