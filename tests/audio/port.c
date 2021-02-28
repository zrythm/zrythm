/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "zrythm-test-config.h"

#include "actions/tracklist_selections.h"
#include "audio/midi_region.h"
#include "audio/region.h"
#include "audio/transport.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "zrythm.h"

#include "tests/helpers/project.h"
#include "tests/helpers/zrythm.h"

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

  yaml = port_serialize (port);
  /*g_warning ("%s", yaml);*/

  Port * deserialized_port =
    port_deserialize (yaml);
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

  yaml = port_serialize (deserialized_port);
  /*g_warning ("%s", yaml);*/

  port = port_deserialize (yaml);
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

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/port/"

  g_test_add_func (
    TEST_PREFIX "test port disconnect",
    (GTestFunc) test_port_disconnect);
  g_test_add_func (
    TEST_PREFIX "test serialization",
    (GTestFunc) test_serialization);

  return g_test_run ();
}
