// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/route_channel_send_command.h"
#include "dsp/audio_port.h"
#include "structure/tracks/channel_send.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

class RouteChannelSendCommandTest : public ::testing::Test
{
protected:
  structure::tracks::ChannelSendUuidReference create_audio_send ()
  {
    return utils::create_object<structure::tracks::ChannelSend> (
      registry_, registry_, dsp::PortType::Audio, false);
  }

  dsp::PortUuidReference create_audio_input_port ()
  {
    return utils::create_object<dsp::AudioPort> (
      registry_, u8"Destination Input", dsp::PortFlow::Input,
      dsp::SpeakerArrangement::stereo ());
  }

  dsp::PortUuidReference create_midi_input_port ()
  {
    return utils::create_object<dsp::MidiPort> (
      registry_, u8"Destination MIDI Input", dsp::PortFlow::Input);
  }

  utils::ObjectRegistry registry_;
};

TEST_F (RouteChannelSendCommandTest, RedoSetsDestinationAndUndoClears)
{
  auto   send_ref = create_audio_send ();
  auto * send = send_ref.get ();
  auto   dest_port = create_audio_input_port ();

  RouteChannelSendCommand cmd (send_ref, dest_port.id ());
  cmd.redo ();
  ASSERT_TRUE (send->has_destination ());
  EXPECT_EQ (send->destination_port ()->id (), dest_port.id ());

  cmd.undo ();
  EXPECT_FALSE (send->has_destination ());
}

TEST_F (RouteChannelSendCommandTest, UndoRestoresReplacedDestination)
{
  auto   send_ref = create_audio_send ();
  auto * send = send_ref.get ();
  auto   first_port = create_audio_input_port ();
  auto   second_port = create_audio_input_port ();

  RouteChannelSendCommand first_cmd (send_ref, first_port.id ());
  first_cmd.redo ();

  RouteChannelSendCommand second_cmd (send_ref, second_port.id ());
  second_cmd.redo ();
  EXPECT_EQ (send->destination_port ()->id (), second_port.id ());

  second_cmd.undo ();
  EXPECT_EQ (send->destination_port ()->id (), first_port.id ());

  first_cmd.undo ();
  EXPECT_FALSE (send->has_destination ());
}

TEST_F (RouteChannelSendCommandTest, MultipleUndoRedoCycles)
{
  auto   send_ref = create_audio_send ();
  auto * send = send_ref.get ();
  auto   dest_port = create_audio_input_port ();

  RouteChannelSendCommand cmd (send_ref, dest_port.id ());
  for (int i = 0; i < 3; ++i)
    {
      cmd.redo ();
      EXPECT_EQ (send->destination_port ()->id (), dest_port.id ());
      cmd.undo ();
      EXPECT_FALSE (send->has_destination ());
    }
}

TEST_F (RouteChannelSendCommandTest, NullDestinationClears)
{
  auto   send_ref = create_audio_send ();
  auto * send = send_ref.get ();
  auto   dest_port = create_audio_input_port ();

  send->set_destination_port (dest_port);
  ASSERT_TRUE (send->has_destination ());

  RouteChannelSendCommand cmd (send_ref, std::nullopt);
  cmd.redo ();
  EXPECT_FALSE (send->has_destination ());

  cmd.undo ();
  EXPECT_EQ (send->destination_port ()->id (), dest_port.id ());
}

TEST_F (RouteChannelSendCommandTest, MismatchedPortTypeIsRejectedBySend)
{
  auto send_ref = create_audio_send ();
  auto midi_port = create_midi_input_port ();

  EXPECT_THROW (
    send_ref.get ()->set_destination_port (midi_port), std::invalid_argument);
}

} // namespace zrythm::commands
