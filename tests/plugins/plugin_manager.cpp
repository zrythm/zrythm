// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/zrythm_helper.h"

#include "common/plugins/plugin_manager.h"

TEST_F (ZrythmFixture, FindPlugins)
{
#ifdef HAVE_MDA_AMBIENCE
  {
    auto setting = test_plugin_manager_get_plugin_setting (
      MDA_AMBIENCE_BUNDLE, MDA_AMBIENCE_URI, false);
  }
#endif
#ifdef HAVE_AMS_LFO
  {
    auto setting = test_plugin_manager_get_plugin_setting (
      AMS_LFO_BUNDLE, AMS_LFO_URI, false);
  }
#endif
#ifdef HAVE_HELM
  {
    auto setting =
      test_plugin_manager_get_plugin_setting (HELM_BUNDLE, HELM_URI, false);
  }
#endif
}