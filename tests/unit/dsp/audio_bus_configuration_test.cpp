// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_bus_configuration.h"
#include "dsp/port_all.h"
#include "dsp/processor_base.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::dsp
{

class AudioBusConfigurationTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    registry_ = std::make_unique<utils::ObjectRegistry> ();
    processor_ = std::make_unique<ProcessorBase> (*registry_, u8"TestProcessor");
  }

  auto add_audio_input (
    utils::Utf8String       label,
    SpeakerArrangement      arrangement,
    AudioPort::Purpose      purpose = AudioPort::Purpose::Main,
    std::optional<uint32_t> external_id = std::nullopt)
  {
    auto port_ref = utils::create_object<AudioPort> (
      *registry_, std::move (label), PortFlow::Input, arrangement, purpose);
    port_ref.get ()->set_external_port_id (external_id);
    processor_->add_input_port (port_ref);
    return port_ref;
  }

  auto add_audio_output (
    utils::Utf8String       label,
    SpeakerArrangement      arrangement,
    AudioPort::Purpose      purpose = AudioPort::Purpose::Main,
    std::optional<uint32_t> external_id = std::nullopt)
  {
    auto port_ref = utils::create_object<AudioPort> (
      *registry_, std::move (label), PortFlow::Output, arrangement, purpose);
    port_ref.get ()->set_external_port_id (external_id);
    processor_->add_output_port (port_ref);
    return port_ref;
  }

  static auto audio_port_at (const auto &ports, size_t index)
  {
    return ports[index].template get_object_as<AudioPort> ();
  }

  std::unique_ptr<utils::ObjectRegistry> registry_;
  std::unique_ptr<ProcessorBase>         processor_;
};

TEST_F (AudioBusConfigurationTest, MatchingConfigurationChangesNothing)
{
  add_audio_input (u8"Main In", SpeakerArrangement::stereo ());
  add_audio_output (u8"Main Out", SpeakerArrangement::stereo ());

  const std::array desired_in{
    AudioBusConfig{
                   u8"Main In", SpeakerArrangement::stereo (), AudioPort::Purpose::Main, true }
  };
  const std::array desired_out{
    AudioBusConfig{
                   u8"Main Out", SpeakerArrangement::stereo (), AudioPort::Purpose::Main,
                   true }
  };

  const auto initial_in = reconcile_audio_bus_configuration (
    *registry_, *processor_, PortFlow::Input, desired_in);
  EXPECT_FALSE (initial_in.graph_changed);
  EXPECT_FALSE (initial_in.metadata_changed);
  const auto initial_out = reconcile_audio_bus_configuration (
    *registry_, *processor_, PortFlow::Output, desired_out);
  EXPECT_FALSE (initial_out.graph_changed);
  EXPECT_FALSE (initial_out.metadata_changed);

  EXPECT_EQ (processor_->get_all_input_ports ().size (), 1);
  EXPECT_EQ (processor_->get_all_output_ports ().size (), 1);
  EXPECT_FALSE (
    audio_port_at (processor_->get_all_input_ports (), 0)->detached ());
}

TEST_F (AudioBusConfigurationTest, RemovedBusesDetachTrailingPorts)
{
  add_audio_input (u8"Main In", SpeakerArrangement::stereo ());
  add_audio_input (
    u8"Sidechain In", SpeakerArrangement::stereo (),
    AudioPort::Purpose::Sidechain);

  const std::array desired{
    AudioBusConfig{
                   u8"Main In", SpeakerArrangement::stereo (), AudioPort::Purpose::Main, true }
  };

  EXPECT_TRUE (
    reconcile_audio_bus_configuration (
      *registry_, *processor_, PortFlow::Input, desired)
      .graph_changed);

  // Both ports are kept; only the second is detached
  ASSERT_EQ (processor_->get_all_input_ports ().size (), 2);
  EXPECT_FALSE (
    audio_port_at (processor_->get_all_input_ports (), 0)->detached ());
  EXPECT_TRUE (
    audio_port_at (processor_->get_all_input_ports (), 1)->detached ());

  // Reconciling again with the same configuration is a no-op
  const auto again = reconcile_audio_bus_configuration (
    *registry_, *processor_, PortFlow::Input, desired);
  EXPECT_FALSE (again.graph_changed);
  EXPECT_FALSE (again.metadata_changed);
}

TEST_F (AudioBusConfigurationTest, ReaddedBusReattachesAndSyncsSamePort)
{
  auto port_ref = add_audio_input (u8"Main In", SpeakerArrangement::stereo ());
  const auto original_uuid = port_ref.id ();
  auto *     port = port_ref.get ();
  port->set_detached (true);

  const std::array desired{
    AudioBusConfig{
                   u8"Renamed In", SpeakerArrangement::mono (), AudioPort::Purpose::Main,
                   true }
  };

  EXPECT_TRUE (
    reconcile_audio_bus_configuration (
      *registry_, *processor_, PortFlow::Input, desired)
      .graph_changed);

  // Same port object and UUID; arrangement and label synced in place
  ASSERT_EQ (processor_->get_all_input_ports ().size (), 1);
  EXPECT_EQ (processor_->get_all_input_ports ()[0].id (), original_uuid);
  EXPECT_FALSE (port->detached ());
  EXPECT_EQ (port->arrangement (), SpeakerArrangement::mono ());
  EXPECT_EQ (port->get_label (), u8"Renamed In");
}

TEST_F (AudioBusConfigurationTest, InactiveBusAtMatchingIndexDetaches)
{
  add_audio_input (u8"Main In", SpeakerArrangement::stereo ());

  const std::array desired{
    AudioBusConfig{
                   u8"Main In", SpeakerArrangement::mono (), AudioPort::Purpose::Main, false }
  };

  EXPECT_TRUE (
    reconcile_audio_bus_configuration (
      *registry_, *processor_, PortFlow::Input, desired)
      .graph_changed);

  auto * port = audio_port_at (processor_->get_all_input_ports (), 0);
  EXPECT_TRUE (port->detached ());
  // Arrangement stays in sync with the reported configuration
  EXPECT_EQ (port->arrangement (), SpeakerArrangement::mono ());
}

TEST_F (AudioBusConfigurationTest, AddedBusesCreateNewPorts)
{
  auto first_ref = add_audio_input (u8"Main In", SpeakerArrangement::stereo ());

  const std::array desired{
    AudioBusConfig{
                   u8"Main In",      SpeakerArrangement::stereo (), AudioPort::Purpose::Main,
                   true                                                                            },
    AudioBusConfig{
                   u8"Sidechain In", SpeakerArrangement::mono (),
                   AudioPort::Purpose::Sidechain,                                             true }
  };

  EXPECT_TRUE (
    reconcile_audio_bus_configuration (
      *registry_, *processor_, PortFlow::Input, desired)
      .graph_changed);

  ASSERT_EQ (processor_->get_all_input_ports ().size (), 2);

  // The existing port is untouched
  EXPECT_EQ (processor_->get_all_input_ports ()[0].id (), first_ref.id ());

  // The new port matches the desired bus and is registered
  const auto &new_ref = processor_->get_all_input_ports ()[1];
  EXPECT_NE (new_ref.id (), first_ref.id ());
  auto * new_port = new_ref.get_object_as<AudioPort> ();
  ASSERT_NE (new_port, nullptr);
  EXPECT_EQ (new_port->get_label (), u8"Sidechain In");
  EXPECT_EQ (new_port->arrangement (), SpeakerArrangement::mono ());
  EXPECT_EQ (new_port->purpose (), AudioPort::Purpose::Sidechain);
  EXPECT_TRUE (new_port->is_input ());
  EXPECT_FALSE (new_port->detached ());
}

TEST_F (AudioBusConfigurationTest, AddedInactiveBusCreatesDetachedPort)
{
  const std::array desired{
    AudioBusConfig{
                   u8"Main Out",  SpeakerArrangement::stereo (), AudioPort::Purpose::Main,
                   true                                                                          },
    AudioBusConfig{
                   u8"Extra Out", SpeakerArrangement::stereo (),
                   AudioPort::Purpose::Sidechain,                                          false }
  };

  EXPECT_TRUE (
    reconcile_audio_bus_configuration (
      *registry_, *processor_, PortFlow::Output, desired)
      .graph_changed);

  ASSERT_EQ (processor_->get_all_output_ports ().size (), 2);
  EXPECT_FALSE (
    audio_port_at (processor_->get_all_output_ports (), 0)->detached ());
  EXPECT_TRUE (
    audio_port_at (processor_->get_all_output_ports (), 1)->detached ());
}

TEST_F (AudioBusConfigurationTest, MidiAndCvPortsAreNotCountedAsBuses)
{
  add_audio_input (u8"Audio 1", SpeakerArrangement::stereo ());
  auto midi_ref =
    utils::create_object<MidiPort> (*registry_, u8"MIDI In", PortFlow::Input);
  processor_->add_input_port (midi_ref);
  add_audio_input (u8"Audio 2", SpeakerArrangement::stereo ());

  const std::array desired{
    AudioBusConfig{
                   u8"Audio 1", SpeakerArrangement::stereo (), AudioPort::Purpose::Main, true }
  };

  EXPECT_TRUE (
    reconcile_audio_bus_configuration (
      *registry_, *processor_, PortFlow::Input, desired)
      .graph_changed);

  // Port order is preserved; the second *audio* port is detached while the
  // MIDI port in between is untouched
  ASSERT_EQ (processor_->get_all_input_ports ().size (), 3);
  EXPECT_FALSE (
    audio_port_at (processor_->get_all_input_ports (), 0)->detached ());
  EXPECT_FALSE (midi_ref.get ()->detached ());
  EXPECT_TRUE (
    audio_port_at (processor_->get_all_input_ports (), 2)->detached ());
}

TEST_F (AudioBusConfigurationTest, PurposeSyncsOnMatchingPorts)
{
  // the hosting SDK may change a bus's purpose at runtime (e.g. CLAP's
  // IS_MAIN flag via a flags rescan)
  add_audio_input (
    u8"Main In", SpeakerArrangement::stereo (), AudioPort::Purpose::Main, 1);

  const std::array desired{
    AudioBusConfig{
                   u8"Main In", SpeakerArrangement::stereo (), AudioPort::Purpose::Sidechain,
                   true, 1 }
  };

  EXPECT_TRUE (
    reconcile_audio_bus_configuration (
      *registry_, *processor_, PortFlow::Input, desired)
      .graph_changed);
  EXPECT_EQ (
    audio_port_at (processor_->get_all_input_ports (), 0)->purpose (),
    AudioPort::Purpose::Sidechain);
}

TEST_F (AudioBusConfigurationTest, StableIdsMatchAcrossIndexShift)
{
  auto main_ref = add_audio_input (
    u8"Main In", SpeakerArrangement::stereo (), AudioPort::Purpose::Main, 1);
  auto sidechain_ref = add_audio_input (
    u8"Sidechain In", SpeakerArrangement::mono (),
    AudioPort::Purpose::Sidechain, 2);
  auto extra_ref = add_audio_input (
    u8"Extra In", SpeakerArrangement::stereo (), AudioPort::Purpose::Main, 3);

  // the plugin removed the middle bus: enumeration indices shift, ids stay
  // stable
  const std::array desired{
    AudioBusConfig{
                   u8"Main In",  SpeakerArrangement::stereo (), AudioPort::Purpose::Main,
                   true, 1 },
    AudioBusConfig{
                   u8"Extra In", SpeakerArrangement::stereo (), AudioPort::Purpose::Main,
                   true, 3 },
  };

  EXPECT_TRUE (
    reconcile_audio_bus_configuration (
      *registry_, *processor_, PortFlow::Input, desired)
      .graph_changed);

  // all three ports are kept; the removed bus's port is detached with its
  // identity intact, and the third port was matched by id rather than the
  // second port being mutated into it
  ASSERT_EQ (processor_->get_all_input_ports ().size (), 3);
  EXPECT_EQ (processor_->get_all_input_ports ()[0].id (), main_ref.id ());
  EXPECT_EQ (processor_->get_all_input_ports ()[1].id (), sidechain_ref.id ());
  EXPECT_EQ (processor_->get_all_input_ports ()[2].id (), extra_ref.id ());

  auto * sidechain = audio_port_at (processor_->get_all_input_ports (), 1);
  EXPECT_TRUE (sidechain->detached ());
  EXPECT_EQ (sidechain->get_label (), u8"Sidechain In");
  EXPECT_EQ (sidechain->arrangement (), SpeakerArrangement::mono ());

  auto * extra = audio_port_at (processor_->get_all_input_ports (), 2);
  EXPECT_FALSE (extra->detached ());
  EXPECT_EQ (extra->get_label (), u8"Extra In");
}

TEST_F (AudioBusConfigurationTest, IdlessPortsAdoptStableIdsPositionally)
{
  // ports saved before stable ids were recorded carry none
  auto main_ref = add_audio_input (u8"Main In", SpeakerArrangement::stereo ());
  auto extra_ref = add_audio_input (
    u8"Extra In", SpeakerArrangement::mono (), AudioPort::Purpose::Sidechain);

  const std::array desired{
    AudioBusConfig{
                   u8"Main In",  SpeakerArrangement::stereo (), AudioPort::Purpose::Main,
                   true, 1 },
    AudioBusConfig{
                   u8"Extra In", SpeakerArrangement::mono (),   AudioPort::Purpose::Sidechain,
                   true, 3 },
  };

  // adopting an id is metadata-only, not a topology change
  const auto adopted = reconcile_audio_bus_configuration (
    *registry_, *processor_, PortFlow::Input, desired);
  EXPECT_FALSE (adopted.graph_changed);
  EXPECT_TRUE (adopted.metadata_changed);

  // matched positionally, and the ports adopt the reported ids
  ASSERT_EQ (processor_->get_all_input_ports ().size (), 2);
  EXPECT_EQ (main_ref.get ()->external_port_id (), 1);
  EXPECT_EQ (extra_ref.get ()->external_port_id (), 3);

  // a later rescan that drops the first bus matches the survivor by id
  const std::array after_removal{
    AudioBusConfig{
                   u8"Extra In", SpeakerArrangement::mono (), AudioPort::Purpose::Sidechain,
                   true, 3 }
  };
  EXPECT_TRUE (
    reconcile_audio_bus_configuration (
      *registry_, *processor_, PortFlow::Input, after_removal)
      .graph_changed);

  ASSERT_EQ (processor_->get_all_input_ports ().size (), 2);
  EXPECT_TRUE (
    audio_port_at (processor_->get_all_input_ports (), 0)->detached ());
  EXPECT_EQ (processor_->get_all_input_ports ()[1].id (), extra_ref.id ());
  EXPECT_FALSE (
    audio_port_at (processor_->get_all_input_ports (), 1)->detached ());
}

TEST_F (AudioBusConfigurationTest, BusWithUnknownStableIdCreatesPort)
{
  add_audio_input (
    u8"Main In", SpeakerArrangement::stereo (), AudioPort::Purpose::Main, 1);

  const std::array desired{
    AudioBusConfig{
                   u8"Main In", SpeakerArrangement::stereo (), AudioPort::Purpose::Main,
                   true, 1 },
    AudioBusConfig{
                   u8"New Bus", SpeakerArrangement::stereo (), AudioPort::Purpose::Sidechain,
                   true, 9 },
  };

  EXPECT_TRUE (
    reconcile_audio_bus_configuration (
      *registry_, *processor_, PortFlow::Input, desired)
      .graph_changed);

  ASSERT_EQ (processor_->get_all_input_ports ().size (), 2);
  auto * new_port = audio_port_at (processor_->get_all_input_ports (), 1);
  EXPECT_EQ (new_port->external_port_id (), 9);
  EXPECT_EQ (new_port->get_label (), u8"New Bus");
  EXPECT_EQ (new_port->purpose (), AudioPort::Purpose::Sidechain);
  EXPECT_FALSE (new_port->detached ());
}

TEST_F (AudioBusConfigurationTest, ReaddedBusReattachesPortByStableId)
{
  auto main_ref = add_audio_input (
    u8"Main In", SpeakerArrangement::stereo (), AudioPort::Purpose::Main, 1);
  auto sidechain_ref = add_audio_input (
    u8"Sidechain In", SpeakerArrangement::mono (),
    AudioPort::Purpose::Sidechain, 2);

  // bus 2 removed
  const std::array without_sidechain{
    AudioBusConfig{
                   u8"Main In", SpeakerArrangement::stereo (), AudioPort::Purpose::Main,
                   true, 1 }
  };
  EXPECT_TRUE (
    reconcile_audio_bus_configuration (
      *registry_, *processor_, PortFlow::Input, without_sidechain)
      .graph_changed);
  ASSERT_TRUE (sidechain_ref.get ()->detached ());

  // bus 2 comes back
  const std::array with_sidechain{
    AudioBusConfig{
                   u8"Main In",      SpeakerArrangement::stereo (), AudioPort::Purpose::Main,
                   true,                                                                            1 },
    AudioBusConfig{
                   u8"Sidechain In", SpeakerArrangement::mono (),
                   AudioPort::Purpose::Sidechain,                                             true, 2 },
  };
  EXPECT_TRUE (
    reconcile_audio_bus_configuration (
      *registry_, *processor_, PortFlow::Input, with_sidechain)
      .graph_changed);

  // the same port object is re-attached; no port was created
  ASSERT_EQ (processor_->get_all_input_ports ().size (), 2);
  EXPECT_EQ (processor_->get_all_input_ports ()[1].id (), sidechain_ref.id ());
  EXPECT_FALSE (sidechain_ref.get ()->detached ());
}

TEST_F (AudioBusConfigurationTest, AddedBusInheritsOwnerDesignationPrefix)
{
  const std::array desired{
    AudioBusConfig{
                   u8"Sidechain In", SpeakerArrangement::mono (),
                   AudioPort::Purpose::Sidechain, true }
  };

  EXPECT_TRUE (
    reconcile_audio_bus_configuration (
      *registry_, *processor_, PortFlow::Input, desired)
      .graph_changed);

  const auto * new_port = audio_port_at (processor_->get_all_input_ports (), 0);
  EXPECT_EQ (new_port->get_full_designation (), u8"TestProcessor/Sidechain In");
}

TEST_F (AudioBusConfigurationTest, DetachedFlagSerializationRoundtrip)
{
  AudioPort port (u8"Port", PortFlow::Input, SpeakerArrangement::stereo ());
  port.set_detached (true);

  const nlohmann::json j = port;
  ASSERT_TRUE (j.contains ("detached"));
  EXPECT_TRUE (j.at ("detached").get<bool> ());

  AudioPort deserialized (
    u8"Other", PortFlow::Output, SpeakerArrangement::mono ());
  from_json (j, deserialized);
  EXPECT_TRUE (deserialized.detached ());

  // Attached ports omit the key, and absent "detached" reads as attached
  const AudioPort attached_port (
    u8"Attached", PortFlow::Input, SpeakerArrangement::stereo ());
  nlohmann::json without_flag = attached_port;
  EXPECT_FALSE (without_flag.contains ("detached"));
  AudioPort attached (u8"Other", PortFlow::Output, SpeakerArrangement::mono ());
  from_json (without_flag, attached);
  EXPECT_FALSE (attached.detached ());
}

} // namespace zrythm::dsp
