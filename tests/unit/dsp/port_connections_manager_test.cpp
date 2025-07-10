// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/port_connections_manager.h"
#include "utils/gtest_wrapper.h"

namespace zrythm::dsp
{

class PortConnectionsManagerTest : public ::testing::Test
{
protected:
  const PortUuid kTestSrc1{ QUuid::createUuid () };
  const PortUuid kTestSrc2{ QUuid::createUuid () };
  const PortUuid kTestDest1{ QUuid::createUuid () };
  const PortUuid kTestDest2{ QUuid::createUuid () };

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
  EXPECT_FALSE (
    manager_->update_connection (PortUuid{}, PortUuid{}, 1.0f, false, true));
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
    PortUuid{ QUuid::createUuid () }, PortUuid{ QUuid::createUuid () }, 1.0f,
    false, true);
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

} // namespace zrythm::dsp
