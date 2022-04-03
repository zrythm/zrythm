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

#include <stdlib.h>

#include "plugins/lv2/lv2_state.h"
#include "plugins/lv2_plugin.h"
#include "utils/string.h"

#include <glib.h>

#include "tests/helpers/plugin_manager.h"

#include <lv2/presets/presets.h>

/* handled by lilv */
#if 0
static void
test_path_features (void)
{
#  ifdef HAVE_LSP_MULTISAMPLER_24_DO
  test_helper_zrythm_init ();

  test_plugin_manager_create_tracks_from_plugin (
    LSP_MULTISAMPLER_24_DO_BUNDLE,
    LSP_MULTISAMPLER_24_DO_URI, true, false, 1);

  Track * track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  Plugin * pl = track->channel->instrument;
  g_assert_true (IS_PLUGIN_AND_NONNULL (pl));

  char * absolute_path =
    lv2_state_make_path_save (pl->lv2, "abc/def.wav");

  char * abstract_path =
    lv2_state_get_abstract_path (
      pl->lv2, absolute_path);
  g_assert_true (
    string_is_equal (
      abstract_path, "abc/def.wav"));

  char * new_absolute_path =
    lv2_state_get_absolute_path (
      pl->lv2, abstract_path);
  g_assert_true (
    string_is_equal (
      new_absolute_path, absolute_path));

  test_helper_zrythm_cleanup ();
#  endif
}
#endif

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/plugins/lv2_plugin/"

#if 0
  g_test_add_func (
    TEST_PREFIX "test path features",
    (GTestFunc) test_path_features);
#endif

  return g_test_run ();
}
