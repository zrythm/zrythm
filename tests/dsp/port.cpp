// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "actions/tracklist_selections.h"
#include "dsp/midi_region.h"
#include "dsp/region.h"
#include "dsp/transport.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "zrythm.h"

#include "tests/helpers/project_helper.h"
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
  g_assert_cmpint (port->dests_.size (), ==, 1);
  g_assert_cmpint (port2->srcs_.size (), ==, 1);
  g_assert_true (port->dests_[0] == port2);

  /*port->dests_[0] = NULL;*/
  port->multipliers[0] = 2.f;
  port->dest_locked[0] = 256;
  port->dest_enabled[0] = 257;

  Port * port3 =
    new Port (
      PortType::Audio, PortFlow::Input, "test-port3");
  port->connect_to (*port3, true);
  g_assert_cmpint (port->dests_.size (), ==, 2);
  g_assert_cmpint (port3->srcs_.size (), ==, 1);
  g_assert_true (port->dests_[0] == port2);
  g_assert_true (port->dests_[1] == port3);
  g_assert_cmpfloat_with_epsilon (
    port->multipliers[0], 2.f, FLT_EPSILON);
  g_assert_cmpint (
    port->dest_locked[0], ==, 256);
  g_assert_cmpint (
    port->dest_enabled[0], ==, 257);
  g_assert_cmpfloat_with_epsilon (
    port->multipliers[1], 1, FLT_EPSILON);
  g_assert_cmpint (
    port->dest_locked[1], ==, 1);
  g_assert_cmpint (
    port->dest_enabled[1], ==, 1);

  port->disconnect_from (port2);
  g_assert_cmpint (port->dests_.size (), ==, 1);
  g_assert_cmpint (port2->srcs_.size (), ==, 0);
  g_assert_true (
    port->dests_[0] == port3);
  g_assert_cmpfloat_with_epsilon (
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
  /*g_warning ("%s", yaml);*/

  Port * deserialized_port =
    (Port *)
    yaml_deserialize (yaml, &port_schema);
  g_assert_cmpfloat_with_epsilon (
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
  /*g_warning ("%s", yaml);*/

  port->disconnect_all ();
  port_free (port);

  port =
    (Port *)
    yaml_deserialize (yaml, &port_schema);
  g_assert_cmpfloat_with_epsilon (
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

static void
test_get_hash (void)
{
  test_helper_zrythm_init ();

  Port port1 = Port (PortType::Audio, PortFlow::Output, "test-port");
  Port port2 = Port (PortType::Audio, PortFlow::Output, "test-port");
  Port port3 = Port (PortType::Audio, PortFlow::Output, "test-port3");

  unsigned int hash1 = port1.get_hash ();
  unsigned int hash2 = port2.get_hash ();
  unsigned int hash3 = port3.get_hash ();
  g_assert_cmpuint (hash1, !=, hash2);
  g_assert_cmpuint (hash1, !=, hash3);

  /* hash again and check equal */
  unsigned int hash11 = port1.get_hash ();
  unsigned int hash22 = port2.get_hash ();
  unsigned int hash33 = port3.get_hash ();
  g_assert_cmpuint (hash1, ==, hash11);
  g_assert_cmpuint (hash2, ==, hash22);
  g_assert_cmpuint (hash3, ==, hash33);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/port/"

  g_test_add_func (TEST_PREFIX "test get hash", (GTestFunc) test_get_hash);
#if 0
  g_test_add_func (
    TEST_PREFIX "test port disconnect",
    (GTestFunc) test_port_disconnect);
  g_test_add_func (
    TEST_PREFIX "test serialization",
    (GTestFunc) test_serialization);
#endif

  return g_test_run ();
}
