// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "tests/helpers/zrythm_helper.h"

#if 0
static void
test_port_disconnect (void)
{
  test_helper_zrythm_init ();

  Port * port =
    new Port (
      PortType::Audio, PortFlow::Output, "test-port");
  Port * port2 =
    new Port (
      PortType::Audio, PortFlow::Input, "test-port2");
  port->connect_to (*port2, true);
  ASSERT_EQ (port->dests_.size (), 1);
  ASSERT_EQ (port2->srcs_.size (), 1);
  ASSERT_TRUE (port->dests_[0] == port2);

  /*port->dests_[0] = NULL;*/
  port->multipliers[0] = 2.f;
  port->dest_locked[0] = 256;
  port->dest_enabled[0] = 257;

  Port * port3 =
    new Port (
      PortType::Audio, PortFlow::Input, "test-port3");
  port->connect_to (*port3, true);
  ASSERT_EQ (port->dests_.size (), 2);
  ASSERT_EQ (port3->srcs_.size (), 1);
  ASSERT_TRUE (port->dests_[0] == port2);
  ASSERT_TRUE (port->dests_[1] == port3);
  ASSERT_NEAR (
    port->multipliers[0], 2.f, FLT_EPSILON);
  g_assert_cmpint (
    port->dest_locked[0], ==, 256);
  g_assert_cmpint (
    port->dest_enabled[0], ==, 257);
  ASSERT_NEAR (
    port->multipliers[1], 1, FLT_EPSILON);
  g_assert_cmpint (
    port->dest_locked[1], ==, 1);
  g_assert_cmpint (
    port->dest_enabled[1], ==, 1);

  port->disconnect_from (port2);
  ASSERT_EQ (port->dests_.size (), 1);
  ASSERT_EQ (port2->srcs_.size (), 0);
  ASSERT_TRUE (
    port->dests_[0] == port3);
  ASSERT_NEAR (
    port->multipliers[0], 1, FLT_EPSILON);
  g_assert_cmpint (
    port->dest_locked[0], ==, 1);
  g_assert_cmpint (
    port->dest_enabled[0], ==, 1);

  test_helper_zrythm_cleanup ();
}

static void
test_serialization (void)
{
  test_helper_zrythm_init ();

  char * yaml = NULL;
  yaml_set_log_level (CYAML_LOG_DEBUG);

  Port * port =
    new Port (
      PortType::Audio, PortFlow::Output, "test-port");
  Port * port2 =
    new Port (
      PortType::Audio, PortFlow::Input, "test-port2");
  port->connect_to (*port2, true);

  port->multipliers[0] = 2.f;
  port->dest_locked[0] = 256;
  port->dest_enabled[0] = 257;

  Port * port3 =
    new Port (
      PortType::Audio, PortFlow::Input, "test-port3");
  port->connect_to (*port3, true);

  yaml = yaml_serialize (port, &port_schema);
  /*z_warning("%s", yaml);*/

  Port * deserialized_port =
    (Port *)
    yaml_deserialize (yaml, &port_schema);
  ASSERT_NEAR (
    port->multipliers[0],
    deserialized_port->multipliers[0],
    FLT_EPSILON);
  g_assert_cmpint (
    port->dest_locked[0], ==,
    deserialized_port->dest_locked[0]);
  g_assert_cmpint (
    port->dest_enabled[0], ==,
    deserialized_port->dest_enabled[0]);
  deserialized_port->magic = PORT_MAGIC;

  yaml =
    yaml_serialize (deserialized_port, &port_schema);
  /*z_warning("%s", yaml);*/

  port->disconnect_all ();
  port_free (port);

  port =
    (Port *)
    yaml_deserialize (yaml, &port_schema);
  ASSERT_NEAR (
    port->multipliers[0],
    deserialized_port->multipliers[0],
    FLT_EPSILON);
  g_assert_cmpint (
    port->dest_locked[0], ==,
    deserialized_port->dest_locked[0]);
  g_assert_cmpint (
    port->dest_enabled[0], ==,
    deserialized_port->dest_enabled[0]);
  port->magic = PORT_MAGIC;

  port->disconnect_all ();
  port2->disconnect_all ();
  port3->disconnect_all ();
  deserialized_port->disconnect_all ();
  port_free (port);
  port_free (port2);
  port_free (port3);
  port_free (deserialized_port);

  test_helper_zrythm_cleanup ();
}
#endif

TEST_F (ZrythmFixture, GetPortHash)
{
  AudioPort port1 ("test-port", PortFlow::Output);
  AudioPort port2 ("test-port", PortFlow::Output);
  AudioPort port3 ("test-port3", PortFlow::Output);

  const auto hash1 = port1.get_hash ();
  const auto hash2 = port2.get_hash ();
  const auto hash3 = port3.get_hash ();
  ASSERT_NE (hash1, hash2);
  ASSERT_NE (hash1, hash3);

  /* hash again and check equal */
  const auto hash11 = port1.get_hash ();
  const auto hash22 = port2.get_hash ();
  const auto hash33 = port3.get_hash ();
  ASSERT_EQ (hash1, hash11);
  ASSERT_EQ (hash2, hash22);
  ASSERT_EQ (hash3, hash33);
}
