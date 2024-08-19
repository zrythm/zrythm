// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "dsp/graph.h"
#include "dsp/graph_export.h"
#include "project.h"
#include "zrythm.h"

#include "tests/helpers/zrythm_helper.h"

TEST_SUITE_BEGIN ("dsp/graph export");

TEST_CASE_FIXTURE (ZrythmFixture, "export graph to SVG")
{
#ifdef HAVE_CGRAPH
  char * tmp_dir = g_dir_make_tmp ("zrythm_graph_export_XXXXXX", nullptr);
  char * filepath = g_build_filename (tmp_dir, "test.svg", nullptr);

  graph_export_as_simple (GRAPH_EXPORT_SVG, filepath);

  io_remove (filepath);
  io_rmdir (tmp_dir, false);
  g_free (filepath);
  g_free (tmp_dir);
#endif
}

TEST_SUITE_END;