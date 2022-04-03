/*
 * Copyright (C) 2020-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "plugins/plugin_manager.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/zrythm.h"

static void
test_find_plugins (void)
{
  PluginSetting * setting;
  (void) setting;
#ifdef HAVE_MDA_AMBIENCE
  setting = test_plugin_manager_get_plugin_setting (
    MDA_AMBIENCE_BUNDLE, MDA_AMBIENCE_URI, false);
  g_assert_nonnull (setting);
#endif
#ifdef HAVE_AMS_LFO
  setting = test_plugin_manager_get_plugin_setting (
    AMS_LFO_BUNDLE, AMS_LFO_URI, false);
  g_assert_nonnull (setting);
#endif
#ifdef HAVE_HELM
  setting = test_plugin_manager_get_plugin_setting (
    HELM_BUNDLE, HELM_URI, false);
  g_assert_nonnull (setting);
#endif
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/plugins/plugin_manager/"

  /* test that finding the plugins works */
  g_test_add_func (
    TEST_PREFIX "test find plugins",
    (GTestFunc) test_find_plugins);

  return g_test_run ();
}
