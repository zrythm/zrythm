// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/change_arranger_object_color_command.h"
#include "dsp/tempo_map.h"
#include "structure/project/project_registry.h"
#include "utils/registry_utils.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

class ChangeArrangerObjectColorCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper_ = std::make_unique<dsp::TempoMapWrapper> (*tempo_map_);
    clip_ref_ = utils::create_object<structure::arrangement::AudioClip> (
      registry_, *tempo_map_wrapper_, registry_);
    color_obj_ = clip_ref_.get ()->color ();
    ASSERT_NE (color_obj_, nullptr);
    color_obj_->setColor (QColor ("#FF0000"));
  }

  std::unique_ptr<dsp::TempoMap>                      tempo_map_;
  std::unique_ptr<dsp::TempoMapWrapper>               tempo_map_wrapper_;
  structure::project::ProjectRegistry                 registry_;
  structure::arrangement::ArrangerObjectUuidReference clip_ref_{ registry_ };
  structure::arrangement::ArrangerObjectColor *       color_obj_;
};

TEST_F (ChangeArrangerObjectColorCommandTest, SetAndUndo)
{
  ChangeArrangerObjectColorCommand command (
    *clip_ref_.get (), registry_, QColor ("#00FF00"));

  command.redo ();
  EXPECT_TRUE (color_obj_->useColor ());
  EXPECT_EQ (color_obj_->color (), QColor ("#00FF00"));

  command.undo ();
  EXPECT_TRUE (color_obj_->useColor ());
  EXPECT_EQ (color_obj_->color (), QColor ("#FF0000"));
}

// Resetting, then undoing, restores the color
TEST_F (ChangeArrangerObjectColorCommandTest, ResetAndUndo)
{
  ChangeArrangerObjectColorCommand command (
    *clip_ref_.get (), registry_, std::nullopt);

  command.redo ();
  EXPECT_FALSE (color_obj_->useColor ());

  command.undo ();
  EXPECT_TRUE (color_obj_->useColor ());
  EXPECT_EQ (color_obj_->color (), QColor ("#FF0000"));
}

// Setting a color after a reset: undo restores the unset state
TEST_F (ChangeArrangerObjectColorCommandTest, UndoAfterResetRestoresUnsetState)
{
  ChangeArrangerObjectColorCommand reset_command (
    *clip_ref_.get (), registry_, std::nullopt);
  reset_command.redo ();

  ChangeArrangerObjectColorCommand set_command (
    *clip_ref_.get (), registry_, QColor ("#00FF00"));
  set_command.redo ();
  set_command.undo ();
  EXPECT_FALSE (color_obj_->useColor ());
}

// Both flavors are labeled for the undo stack
TEST_F (ChangeArrangerObjectColorCommandTest, CommandTexts)
{
  const ChangeArrangerObjectColorCommand set_command (
    *clip_ref_.get (), registry_, QColor ("#00FF00"));
  EXPECT_EQ (set_command.text (), QString ("Change Object Color"));

  const ChangeArrangerObjectColorCommand reset_command (
    *clip_ref_.get (), registry_, std::nullopt);
  EXPECT_EQ (reset_command.text (), QString ("Reset Object Color"));
}

} // namespace zrythm::commands
