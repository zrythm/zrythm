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

#include "tests/helpers/project.h"
#include "tests/helpers/zrythm.h"

#if 0
static void
test_port_disconnect (void)
{
  test_helper_zrythm_init ();

  Port * port =
    port_new_with_type (
      TYPE_AUDIO, FLOW_OUTPUT, "test-port");
  Port * port2 =
    port_new_with_type (
      TYPE_AUDIO, FLOW_INPUT, "test-port2");
  port_connect (port, port2, true);
  g_assert_cmpint (port->num_dests, ==, 1);
  g_assert_cmpint (port2->num_srcs, ==, 1);
  g_assert_true (port->dests[0] == port2);

  /*port->dests[0] = NULL;*/
  port->multipliers[0] = 2.f;
  port->dest_locked[0] = 256;
  port->dest_enabled[0] = 257;

  Port * port3 =
    port_new_with_type (
      TYPE_AUDIO, FLOW_INPUT, "test-port3");
  port_connect (port, port3, true);
  g_assert_cmpint (port->num_dests, ==, 2);
  g_assert_cmpint (port3->num_srcs, ==, 1);
  g_assert_true (port->dests[0] == port2);
  g_assert_true (port->dests[1] == port3);
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

  port_disconnect (port, port2);
  g_assert_cmpint (port->num_dests, ==, 1);
  g_assert_cmpint (port2->num_srcs, ==, 0);
  g_assert_true (
    port->dests[0] == port3);
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
    port_new_with_type (
      TYPE_AUDIO, FLOW_OUTPUT, "test-port");
  Port * port2 =
    port_new_with_type (
      TYPE_AUDIO, FLOW_INPUT, "test-port2");
  port_connect (port, port2, true);

  port->multipliers[0] = 2.f;
  port->dest_locked[0] = 256;
  port->dest_enabled[0] = 257;

  Port * port3 =
    port_new_with_type (
      TYPE_AUDIO, FLOW_INPUT, "test-port3");
  port_connect (port, port3, true);

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

  port_disconnect_all (port);
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

  port_disconnect_all (port);
  port_disconnect_all (port2);
  port_disconnect_all (port3);
  port_disconnect_all (deserialized_port);
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

  Port * port1 =
    port_new_with_type (TYPE_AUDIO, FLOW_OUTPUT, "test-port");
  Port * port2 =
    port_new_with_type (TYPE_AUDIO, FLOW_OUTPUT, "test-port");
  Port * port3 = port_new_with_type (
    TYPE_AUDIO, FLOW_OUTPUT, "test-port3");

  unsigned int hash1 = port_get_hash (port1);
  unsigned int hash2 = port_get_hash (port2);
  unsigned int hash3 = port_get_hash (port3);
  g_assert_cmpuint (hash1, !=, hash2);
  g_assert_cmpuint (hash1, !=, hash3);

  /* hash again and check equal */
  unsigned int hash11 = port_get_hash (port1);
  unsigned int hash22 = port_get_hash (port2);
  unsigned int hash33 = port_get_hash (port3);
  g_assert_cmpuint (hash1, ==, hash11);
  g_assert_cmpuint (hash2, ==, hash22);
  g_assert_cmpuint (hash3, ==, hash33);

  port_free (port1);
  port_free (port2);
  port_free (port3);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/port/"

  g_test_add_func (
    TEST_PREFIX "test get hash", (GTestFunc) test_get_hash);
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
