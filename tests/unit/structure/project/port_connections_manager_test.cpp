// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdexcept>
#include <vector>

#include "dsp/audio_port.h"
#include "dsp/cv_port.h"
#include "dsp/midi_port.h"
#include "structure/project/port_connections_manager.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"
#include "utils/uuid_reference.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::structure::project
{

class PortConnectionsManagerTest : public ::testing::Test
{
protected:
  const dsp::PortUuid kTestSrc1{ QUuid::createUuid () };
  const dsp::PortUuid kTestSrc2{ QUuid::createUuid () };
  const dsp::PortUuid kTestDest1{ QUuid::createUuid () };
  const dsp::PortUuid kTestDest2{ QUuid::createUuid () };

  void SetUp () override
  {
    manager_ = std::make_unique<PortConnectionsManager> ();

    // Add some test connections
    manager_->add_connection (
      PortConnection (kTestSrc1, kTestDest1, 1.0f, false, true));
    manager_->add_connection (
      PortConnection (kTestSrc2, kTestDest2, 0.5f, true, false));
  }

  std::unique_ptr<PortConnectionsManager> manager_;
};

TEST_F (PortConnectionsManagerTest, AddAndGetConnection)
{
  // Verify initial connections exist
  auto * conn1 = manager_->get_connection (kTestSrc1, kTestDest1);
  ASSERT_NE (conn1, nullptr);
  EXPECT_EQ (conn1->src_id_, kTestSrc1);
  EXPECT_EQ (conn1->dest_id_, kTestDest1);

  auto * conn2 = manager_->get_connection (kTestSrc2, kTestDest2);
  ASSERT_NE (conn2, nullptr);
  EXPECT_EQ (conn2->src_id_, kTestSrc2);
  EXPECT_EQ (conn2->dest_id_, kTestDest2);
}

TEST_F (PortConnectionsManagerTest, RemoveConnection)
{
  // Remove first connection
  EXPECT_TRUE (manager_->remove_connection (kTestSrc1, kTestDest1));

  // Verify removal
  EXPECT_EQ (manager_->get_connection (kTestSrc1, kTestDest1), nullptr);

  // Second connection should still exist
  EXPECT_NE (manager_->get_connection (kTestSrc2, kTestDest2), nullptr);
}

TEST_F (PortConnectionsManagerTest, GetConnectionsForPort)
{
  // Add another connection with same source
  manager_->add_connection (
    PortConnection (kTestSrc1, kTestDest2, 0.8f, false, true));

  auto num_src_conns = manager_->get_num_dests (kTestSrc1);
  EXPECT_EQ (num_src_conns, 2);

  auto num_dest_conns = manager_->get_num_sources (kTestDest2);
  EXPECT_EQ (
    num_dest_conns, 2); // kTestSrc2->kTestDest2 and kTestSrc1->kTestDest2
}

TEST_F (PortConnectionsManagerTest, ConnectionExists)
{
  EXPECT_TRUE (manager_->connection_exists (kTestSrc1, kTestDest1));
  EXPECT_FALSE (manager_->connection_exists (kTestSrc1, kTestDest2));
}

TEST_F (PortConnectionsManagerTest, UpdateConnection)
{
  // Update existing connection
  EXPECT_TRUE (
    manager_->update_connection (kTestSrc1, kTestDest1, 0.75f, true, false));

  auto * conn = manager_->get_connection (kTestSrc1, kTestDest1);
  EXPECT_FLOAT_EQ (conn->multiplier_, 0.75f);
  EXPECT_TRUE (conn->locked_);
  EXPECT_FALSE (conn->enabled_);

  // Try updating non-existent connection
  EXPECT_FALSE (manager_->update_connection (
    dsp::PortUuid{}, dsp::PortUuid{}, 1.0f, false, true));
}

TEST_F (PortConnectionsManagerTest, GetSourceOrDest)
{
  auto * src = manager_->get_source_or_dest (kTestDest1, true);
  ASSERT_NE (src, nullptr);
  EXPECT_EQ (src->src_id_, kTestSrc1);

  auto * dest = manager_->get_source_or_dest (kTestSrc1, false);
  ASSERT_NE (dest, nullptr);
  EXPECT_EQ (dest->dest_id_, kTestDest1);
}

TEST_F (PortConnectionsManagerTest, GetUnlockedSourcesOrDests)
{
  // Add a locked connection
  manager_->add_connection (
    PortConnection (kTestSrc1, kTestDest2, 0.8f, true, true));

  PortConnectionsManager::ConnectionsVector unlocked;
  int                                       count =
    manager_->get_unlocked_sources_or_dests (&unlocked, kTestDest1, true);
  EXPECT_EQ (count, 1);
  EXPECT_FALSE (unlocked[0]->locked_);

  count = manager_->get_unlocked_sources_or_dests (&unlocked, kTestSrc1, false);
  EXPECT_EQ (count, 1);
  EXPECT_FALSE (unlocked[0]->locked_);
}

TEST_F (PortConnectionsManagerTest, AddDefaultConnection)
{
  auto * conn = manager_->add_default_connection (kTestSrc1, kTestDest2, false);
  ASSERT_NE (conn, nullptr);
  EXPECT_FLOAT_EQ (conn->multiplier_, 1.0f);
  EXPECT_FALSE (conn->locked_);
  EXPECT_TRUE (conn->enabled_);
}

TEST_F (PortConnectionsManagerTest, RemoveAllConnections)
{
  manager_->remove_all_connections (kTestSrc1);
  EXPECT_EQ (manager_->get_num_dests (kTestSrc1), 0);
  EXPECT_EQ (manager_->get_connection (kTestSrc1, kTestDest1), nullptr);
}

TEST_F (PortConnectionsManagerTest, ResetConnectionsFromOther)
{
  PortConnectionsManager other;
  other.add_connection (
    PortConnection (kTestSrc1, kTestDest2, 0.8f, true, false));

  manager_->reset_connections_from_other (&other);
  EXPECT_EQ (manager_->get_connection_count (), 1);
  EXPECT_TRUE (manager_->connection_exists (kTestSrc1, kTestDest2));
}

TEST_F (PortConnectionsManagerTest, ContainsConnection)
{
  PortConnection conn (kTestSrc1, kTestDest1, 1.0f, false, true);
  EXPECT_TRUE (manager_->contains_connection (conn));

  PortConnection non_existent (
    dsp::PortUuid{ QUuid::createUuid () },
    dsp::PortUuid{ QUuid::createUuid () }, 1.0f, false, true);
  EXPECT_FALSE (manager_->contains_connection (non_existent));
}

TEST_F (PortConnectionsManagerTest, JsonSerialization)
{
  // Serialize
  nlohmann::json j;
  to_json (j, *manager_);

  // Deserialize into new manager
  PortConnectionsManager deserialized;
  from_json (j, deserialized);

  // Verify connections were preserved
  EXPECT_TRUE (deserialized.connection_exists (kTestSrc1, kTestDest1));
  EXPECT_TRUE (deserialized.connection_exists (kTestSrc2, kTestDest2));
}

TEST_F (PortConnectionsManagerTest, ClearAll)
{
  manager_->clear_all ();
  EXPECT_EQ (manager_->get_connection_count (), 0);
}

class PortConnectionsManagerReasonTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    manager_ = std::make_unique<PortConnectionsManager> ();
  }

  dsp::CVPort * add_cv_port (utils::Utf8String label, dsp::PortFlow flow)
  {
    auto ref =
      utils::create_object<dsp::CVPort> (registry_, std::move (label), flow);
    // the fixture holds the registry reference so the port outlives the
    // test body
    port_refs_.push_back (ref.as_untyped ());
    return ref.get ();
  }

  dsp::MidiPort * add_midi_port (utils::Utf8String label, dsp::PortFlow flow)
  {
    auto ref =
      utils::create_object<dsp::MidiPort> (registry_, std::move (label), flow);
    port_refs_.push_back (ref.as_untyped ());
    return ref.get ();
  }

  utils::ObjectRegistry                   registry_;
  std::unique_ptr<PortConnectionsManager> manager_;
  std::vector<utils::UuidReference>       port_refs_;
};

TEST_F (PortConnectionsManagerReasonTest, ConnectAcceptsValidPairing)
{
  auto * src = add_cv_port (u8"CV out", dsp::PortFlow::Output);
  auto * dest = add_cv_port (u8"CV in", dsp::PortFlow::Input);

  const PortConnectionsManager::ConnectivityQuery always_valid =
    [] (const dsp::Port &, const dsp::Port &) {
      return std::optional<utils::RefusalReason>{};
    };

  EXPECT_FALSE (
    manager_->connect_refusal_reason (*src, *dest, always_valid).has_value ());
}

TEST_F (PortConnectionsManagerReasonTest, ConnectStatesEachRefusal)
{
  const PortConnectionsManager::ConnectivityQuery always_valid =
    [] (const dsp::Port &, const dsp::Port &) {
      return std::optional<utils::RefusalReason>{};
    };

  auto * src = add_cv_port (u8"CV out", dsp::PortFlow::Output);
  auto * dest = add_cv_port (u8"CV in", dsp::PortFlow::Input);

  const auto self = manager_->connect_refusal_reason (*src, *src, always_valid);
  ASSERT_TRUE (self.has_value ());
  EXPECT_EQ (self->kind, utils::RefusalKind::InvalidParameter);
  EXPECT_NE (self->text.view ().find ("itself"), std::string_view::npos);

  auto *     second_output = add_cv_port (u8"CV out 2", dsp::PortFlow::Output);
  const auto direction =
    manager_->connect_refusal_reason (*src, *second_output, always_valid);
  ASSERT_TRUE (direction.has_value ());
  EXPECT_EQ (direction->kind, utils::RefusalKind::InvalidParameter);
  EXPECT_NE (
    direction->text.view ().find ("input port"), std::string_view::npos);

  auto *     midi_in = add_midi_port (u8"MIDI in", dsp::PortFlow::Input);
  const auto type =
    manager_->connect_refusal_reason (*src, *midi_in, always_valid);
  ASSERT_TRUE (type.has_value ());
  EXPECT_EQ (type->kind, utils::RefusalKind::TypeMismatch);
  EXPECT_NE (type->text.view ().find ("takes"), std::string_view::npos);

  manager_->add_default_connection (src->get_uuid (), dest->get_uuid (), false);
  const auto duplicate =
    manager_->connect_refusal_reason (*src, *dest, always_valid);
  ASSERT_TRUE (duplicate.has_value ());
  EXPECT_EQ (duplicate->kind, utils::RefusalKind::AlreadyExists);
  EXPECT_NE (
    duplicate->text.view ().find ("already connected"), std::string_view::npos);
}

TEST_F (PortConnectionsManagerReasonTest, ConnectValidatesDetachedDestination)
{
  auto * src = add_cv_port (u8"CV out", dsp::PortFlow::Output);
  auto * dest = add_cv_port (u8"CV in", dsp::PortFlow::Input);
  dest->set_detached (true);

  bool       query_saw_attached_destination = false;
  const auto loop_refusal = manager_->connect_refusal_reason (
    *src, *dest, [&] (const dsp::Port &, const dsp::Port &queried_dest) {
      query_saw_attached_destination = !queried_dest.detached ();
      return std::optional<utils::RefusalReason>{
        utils::RefusalReason{
                             utils::RefusalKind::CyclicRoute,
                             utils::Utf8String::from_utf8_encoded_string (
            "the wiring would close a signal loop") }
      };
    });
  ASSERT_TRUE (loop_refusal.has_value ());
  EXPECT_TRUE (query_saw_attached_destination);
  // the lasting attachment state is untouched by validation
  EXPECT_TRUE (dest->detached ());
  EXPECT_EQ (loop_refusal->kind, utils::RefusalKind::CyclicRoute);
  EXPECT_NE (
    loop_refusal->text.view ().find ("signal loop"), std::string_view::npos);

  const auto valid = manager_->connect_refusal_reason (
    *src, *dest, [] (const dsp::Port &, const dsp::Port &) {
      return std::optional<utils::RefusalReason>{};
    });
  EXPECT_FALSE (valid.has_value ());
  EXPECT_TRUE (dest->detached ());

  const auto failed = manager_->connect_refusal_reason (
    *src, *dest,
    [] (const dsp::Port &, const dsp::Port &)
      -> std::optional<utils::RefusalReason> {
      throw std::runtime_error ("graph build failed");
    });
  ASSERT_TRUE (failed.has_value ());
  EXPECT_EQ (failed->kind, utils::RefusalKind::Unavailable);
  EXPECT_NE (
    failed->text.view ().find ("could not be validated"),
    std::string_view::npos);
  EXPECT_TRUE (dest->detached ());
}

TEST_F (PortConnectionsManagerReasonTest, EditStatesMissingAndLockedReasons)
{
  auto * src = add_cv_port (u8"CV out", dsp::PortFlow::Output);
  auto * dest = add_cv_port (u8"CV in", dsp::PortFlow::Input);
  auto * locked_dest = add_cv_port (u8"Locked in", dsp::PortFlow::Input);

  manager_->add_default_connection (src->get_uuid (), dest->get_uuid (), false);
  manager_->add_default_connection (
    src->get_uuid (), locked_dest->get_uuid (), true);

  const auto missing = manager_->edit_refusal_reason (
    *dest, *src); // no connection in this direction
  ASSERT_TRUE (missing.has_value ());
  EXPECT_EQ (missing->kind, utils::RefusalKind::NotFound);
  EXPECT_NE (
    missing->text.view ().find ("no connection"), std::string_view::npos);

  const auto locked = manager_->edit_refusal_reason (*src, *locked_dest);
  ASSERT_TRUE (locked.has_value ());
  EXPECT_EQ (locked->kind, utils::RefusalKind::Unsupported);
  EXPECT_NE (locked->text.view ().find ("locked"), std::string_view::npos);

  EXPECT_FALSE (manager_->edit_refusal_reason (*src, *dest).has_value ());
}

TEST_F (PortConnectionsManagerReasonTest, MultiplierStatesTypeAndRangeReasons)
{
  auto * src = add_cv_port (u8"CV out", dsp::PortFlow::Output);
  auto * cv_dest = add_cv_port (u8"CV in", dsp::PortFlow::Input);
  auto   audio_dest_ref = utils::create_object<dsp::AudioPort> (
    registry_, u8"Audio in", dsp::PortFlow::Input,
    dsp::SpeakerArrangement::stereo ());
  port_refs_.push_back (audio_dest_ref.as_untyped ());
  auto * audio_dest = audio_dest_ref.get ();
  auto * midi_dest = add_midi_port (u8"MIDI in", dsp::PortFlow::Input);

  manager_->add_default_connection (
    src->get_uuid (), cv_dest->get_uuid (), false);
  manager_->add_default_connection (
    src->get_uuid (), audio_dest->get_uuid (), false);
  manager_->add_default_connection (
    src->get_uuid (), midi_dest->get_uuid (), false);

  EXPECT_FALSE (
    manager_->multiplier_refusal_reason (*src, *cv_dest, 1.0).has_value ());
  EXPECT_FALSE (
    manager_->multiplier_refusal_reason (*src, *audio_dest, 2.0).has_value ());

  const auto cv_range =
    manager_->multiplier_refusal_reason (*src, *cv_dest, 1.5);
  ASSERT_TRUE (cv_range.has_value ());
  EXPECT_EQ (cv_range->kind, utils::RefusalKind::OutOfRange);
  EXPECT_NE (
    cv_range->text.view ().find ("between -1 and 1"), std::string_view::npos);

  const auto audio_range =
    manager_->multiplier_refusal_reason (*src, *audio_dest, 2.5);
  ASSERT_TRUE (audio_range.has_value ());
  EXPECT_EQ (audio_range->kind, utils::RefusalKind::OutOfRange);
  EXPECT_NE (
    audio_range->text.view ().find ("between -2 and 2"), std::string_view::npos);

  const auto midi = manager_->multiplier_refusal_reason (*src, *midi_dest, 0.5);
  ASSERT_TRUE (midi.has_value ());
  EXPECT_EQ (midi->kind, utils::RefusalKind::Unsupported);
  EXPECT_NE (midi->text.view ().find ("MIDI"), std::string_view::npos);

  const auto missing = manager_->multiplier_refusal_reason (*cv_dest, *src, 0.5);
  ASSERT_TRUE (missing.has_value ());
  EXPECT_EQ (missing->kind, utils::RefusalKind::NotFound);
  EXPECT_NE (
    missing->text.view ().find ("no connection"), std::string_view::npos);
}

} // namespace zrythm::structure::project
