// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/channel_send_commands.h"
#include "structure/tracks/channel.h"
#include "utils/object_registry.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::commands
{

class ChannelSendCommandsTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    channel_ = std::make_unique<structure::tracks::Channel> (
      registry_, dsp::PortType::Audio, dsp::PortType::Audio,
      [] { return u8"Test Channel"; }, false,
      [] (bool solo_status) { return false; });
  }

  utils::ObjectRegistry                       registry_;
  std::unique_ptr<structure::tracks::Channel> channel_;
};

TEST_F (ChannelSendCommandsTest, AddSendAppendsToGroup)
{
  AddChannelSendCommand cmd (*channel_->sends (), true);
  cmd.redo ();

  EXPECT_EQ (channel_->sends ()->rowCount (), 1);
  auto * send = channel_->sends ()->send_at_index (0);
  ASSERT_NE (send, nullptr);
  EXPECT_TRUE (send->is_prefader ());
}

TEST_F (ChannelSendCommandsTest, AddSendUndoRedoKeepsIdentity)
{
  AddChannelSendCommand cmd (*channel_->sends (), false);
  cmd.redo ();
  const auto added_id = channel_->sends ()->send_at_index (0)->get_uuid ();

  cmd.undo ();
  EXPECT_EQ (channel_->sends ()->rowCount (), 0);

  cmd.redo ();
  ASSERT_EQ (channel_->sends ()->rowCount (), 1);
  // The same send is reinserted, not a new one
  EXPECT_EQ (channel_->sends ()->send_at_index (0)->get_uuid (), added_id);
}

TEST_F (ChannelSendCommandsTest, RemoveSendUndoRestoresPosition)
{
  auto first_id = channel_->sends ()->append_send (false).id ();
  auto second_id = channel_->sends ()->append_send (false).id ();
  ASSERT_EQ (channel_->sends ()->rowCount (), 2);

  RemoveChannelSendCommand cmd (*channel_->sends (), first_id);
  cmd.redo ();
  EXPECT_EQ (channel_->sends ()->rowCount (), 1);
  EXPECT_EQ (channel_->sends ()->send_at_index (0)->get_uuid (), second_id);

  cmd.undo ();
  EXPECT_EQ (channel_->sends ()->rowCount (), 2);
  EXPECT_EQ (channel_->sends ()->send_at_index (0)->get_uuid (), first_id);
  EXPECT_EQ (channel_->sends ()->send_at_index (1)->get_uuid (), second_id);
}

TEST_F (ChannelSendCommandsTest, RemoveSendUndoRedoCycles)
{
  auto send_id = channel_->sends ()->append_send (true).id ();

  RemoveChannelSendCommand cmd (*channel_->sends (), send_id);
  for (int i = 0; i < 3; ++i)
    {
      cmd.redo ();
      EXPECT_EQ (channel_->sends ()->rowCount (), 0);
      cmd.undo ();
      ASSERT_EQ (channel_->sends ()->rowCount (), 1);
      EXPECT_EQ (channel_->sends ()->send_at_index (0)->get_uuid (), send_id);
      EXPECT_TRUE (channel_->sends ()->send_at_index (0)->is_prefader ());
    }
}

} // namespace zrythm::commands
