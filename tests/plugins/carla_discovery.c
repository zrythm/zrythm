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

#include "audio/fader.h"
#include "audio/router.h"
#include "plugins/carla/carla_discovery.h"
#include "utils/math.h"

#include <glib.h>

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/zrythm.h"

static void
test_mock_au_plugin_scan (void)
{
  test_helper_zrythm_init ();

#ifdef HAVE_CARLA
  g_message ("Scanning AU plugins...");
  unsigned int au_count = 45;
  GError *     err = NULL;
  char *       contents = NULL;
  char *       filename = g_build_filename (
          TESTS_SRCDIR, "au_plugins.txt", NULL);
  bool success = g_file_get_contents (
    filename, &contents, NULL, &err);
  g_assert_true (success);
  char * all_plugins = contents;
  g_message ("all plugins %s", all_plugins);
  g_message ("%u plugins found", au_count);
  for (unsigned int i = 0; i < au_count; i++)
    {
      PluginDescriptor * descriptor =
        z_carla_discovery_create_au_descriptor_from_string (
          all_plugins, (int) i);

      if (descriptor)
        {
          g_assert_cmpuint (
            strlen (descriptor->category_str), >,
            1);
          g_assert_cmpuint (
            strlen (descriptor->category_str), <,
            40);

          g_message (
            "Scanned AU plugin %s",
            descriptor->name);
        }
      else
        {
          g_message ("Skipped AU plugin at %u", i);
        }

      plugin_descriptor_free (descriptor);
    }
#endif

  test_helper_zrythm_cleanup ();
}

static void
test_parse_plugin_info (void)
{
  test_helper_zrythm_init ();

#ifdef HAVE_CARLA
  const char * str =
    "\
carla-discovery::init::-----------\n\
\n\
carla-discovery::build::2\n\
\n\
carla-discovery::hints::8\n\
\n\
carla-discovery::category::none\n\
\n\
carla-discovery::name::SurgeEffectsBank\n\
\n\
carla-discovery::maker::Surge Synth Team\n\
\n\
carla-discovery::label::AudioUnit:Effects/aufx,SFXB,VmbA\n\
\n\
carla-discovery::audio.ins::4\n\
\n\
carla-discovery::audio.outs::2\n\
\n\
carla-discovery::cv.ins::0\n\
\n\
carla-discovery::cv.outs::0\n\
\n\
carla-discovery::midi.ins::0\n\
\n\
carla-discovery::midi.outs::0\n\
\n\
carla-discovery::parameters.ins::0\n\
\n\
carla-discovery::parameters.outs::0\n\
\n\
carla-discovery::end::------------\n\
\n\
carla-discovery::init::-----------\n\
\n\
carla-discovery::build::2\n\
\n\
carla-discovery::hints::8\n\
\n\
carla-discovery::category::none\n\
\n\
carla-discovery::name::AUBandpass\n\
\n\
carla-discovery::maker::Apple\n\
\n\
carla-discovery::label::AudioUnit:Effects/aufx,bpas,appl\n\
\n\
carla-discovery::audio.ins::2\n\
\n\
carla-discovery::audio.outs::2\n\
\n\
carla-discovery::cv.ins::0\n\
\n\
carla-discovery::cv.outs::0\n\
\n\
carla-discovery::midi.ins::0\n\
\n\
carla-discovery::midi.outs::0\n\
\n\
carla-discovery::parameters.ins::0\n\
\n\
carla-discovery::parameters.outs::0\n\
\n\
carla-discovery::end::------------\n\
\n\
carla-discovery::init::-----------\n\
\n\
carla-discovery::build::2\n\
\n\
carla-discovery::hints::8\n\
\n\
carla-discovery::category::none\n\
\n\
carla-discovery::name::AUDynamicsProcessor\n\
\n\
carla-discovery::maker::Apple\n\
\n\
carla-discovery::label::AudioUnit:Effects/aufx,dcmp,appl\n\
\n\
carla-discovery::audio.ins::2\n\
\n\
carla-discovery::audio.outs::2\n\
\n\
carla-discovery::cv.ins::0\n\
\n\
carla-discovery::cv.outs::0\n\
\n\
carla-discovery::midi.ins::0\n\
\n\
carla-discovery::midi.outs::0\n\
\n\
carla-discovery::parameters.ins::0\n\
\n\
carla-discovery::parameters.outs::0\n\
\n\
carla-discovery::end::------------";

  PluginDescriptor * descr = NULL;
  descr =
    z_carla_discovery_create_au_descriptor_from_string (
      str, 0);
  g_assert_cmpstr (
    descr->name, ==, "SurgeEffectsBank");
  g_assert_true (descr->has_custom_ui);
  plugin_descriptor_free (descr);
  descr =
    z_carla_discovery_create_au_descriptor_from_string (
      str, 1);
  g_assert_cmpstr (descr->name, ==, "AUBandpass");
  g_assert_true (descr->has_custom_ui);
  plugin_descriptor_free (descr);
#endif

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/plugins/carla_discovery/"

  g_test_add_func (
    TEST_PREFIX "test mock AU plugin scan",
    (GTestFunc) test_mock_au_plugin_scan);
  g_test_add_func (
    TEST_PREFIX "test parse plugin info",
    (GTestFunc) test_parse_plugin_info);

  return g_test_run ();
}
