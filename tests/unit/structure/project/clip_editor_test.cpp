// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/midi_clip.h"
#include "structure/project/clip_editor.h"
#include "utils/object_registry.h"

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm
{
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
    registry_ = std::make_unique<utils::ObjectRegistry> ();

    clip_editor_ = std::make_unique<ClipEditor> (*registry_, nullptr);
  }

  void TearDown () override
  {
    clip_editor_.reset ();
    registry_.reset ();
  }

  std::unique_ptr<utils::ObjectRegistry> registry_;
  std::unique_ptr<ClipEditor>            clip_editor_;
};

// ============================================================================
// Construction Tests
// ============================================================================

TEST_F (ClipEditorTest, Construction)
{
  EXPECT_NE (clip_editor_, nullptr);
  EXPECT_FALSE (clip_editor_->has_clip ());
}

// ============================================================================
// Clip Tests
// ============================================================================

TEST_F (ClipEditorTest, HasRegionInitiallyFalse)
{
  EXPECT_FALSE (clip_editor_->has_clip ());
}

TEST_F (ClipEditorTest, RegionInitiallyNull)
{
  auto * clip = clip_editor_->clip ();
  EXPECT_TRUE (clip == nullptr);
}

TEST_F (ClipEditorTest, TrackInitiallyNull)
{
  auto track = clip_editor_->track ();
  EXPECT_TRUE (track.isNull ());
}

// ============================================================================
// Editor Components Tests
// ============================================================================

TEST_F (ClipEditorTest, MidiEditorNotNull)
{
  EXPECT_NE (clip_editor_->midiEditor (), nullptr);
}

TEST_F (ClipEditorTest, ChordEditorNotNull)
{
  EXPECT_NE (clip_editor_->chordEditor (), nullptr);
}

TEST_F (ClipEditorTest, AudioClipEditorNotNull)
{
  EXPECT_NE (clip_editor_->audioClipEditor (), nullptr);
}

TEST_F (ClipEditorTest, AutomationEditorNotNull)
{
  EXPECT_NE (clip_editor_->automationEditor (), nullptr);
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

TEST_F (ClipEditorTest, JsonSerializationNoClip)
{
  // ClipEditor with no clip set
  nlohmann::json j = *clip_editor_;

  // Verify JSON contains expected keys
  EXPECT_TRUE (j.contains ("clipId"));
  EXPECT_TRUE (j.contains ("midiEditor"));
  EXPECT_TRUE (j.contains ("automationEditor"));
  EXPECT_TRUE (j.contains ("chordEditor"));
  EXPECT_TRUE (j.contains ("audioClipEditor"));
}

TEST_F (ClipEditorTest, JsonSerializationRoundTripNoClip)
{
  nlohmann::json j = *clip_editor_;

  auto deserialized_editor = std::make_unique<ClipEditor> (*registry_, nullptr);
  j.get_to (*deserialized_editor);

  // Should still have no clip
  EXPECT_FALSE (deserialized_editor->has_clip ());
}

// ============================================================================
// Clone Tests
// ============================================================================

TEST_F (ClipEditorTest, InitFromNoClip)
{
  auto cloned_editor = std::make_unique<ClipEditor> (*registry_, nullptr);

  init_from (*cloned_editor, *clip_editor_, utils::ObjectCloneType::Snapshot);

  // Should have no clip like the original
  EXPECT_FALSE (cloned_editor->has_clip ());
}

// ============================================================================
// Set/Unset Clip Tests
// ============================================================================

TEST_F (ClipEditorTest, UnsetClipWhenNoClipDoesNotThrow)
{
  EXPECT_NO_THROW (clip_editor_->unsetClip ());
  EXPECT_FALSE (clip_editor_->has_clip ());
}

TEST_F (ClipEditorTest, UnsetClipEmitsSignal)
{
  bool signal_emitted = false;

  QObject::connect (
    clip_editor_.get (), &ClipEditor::clipObjectChanged, clip_editor_.get (),
    [&] () { signal_emitted = true; });

  clip_editor_->unsetClip ();

  // Should not emit when there was no clip
  EXPECT_FALSE (signal_emitted);
}
}
