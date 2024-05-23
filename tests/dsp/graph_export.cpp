// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "dsp/graph.h"
#include "dsp/graph_export.h"
#include "project.h"
#include "zrythm.h"

#include <glib.h>

#include "helpers/plugin_manager.h"
#include "helpers/zrythm.h"

static void
test_svg_export (void)
{
#ifdef HAVE_CGRAPH
  test_helper_zrythm_init ();

  char * tmp_dir = g_dir_make_tmp ("zrythm_graph_export_XXXXXX", NULL);
  char * filepath = g_build_filename (tmp_dir, "test.svg", NULL);

  graph_export_as_simple (GRAPH_EXPORT_SVG, filepath);

  io_remove (filepath);
  io_rmdir (tmp_dir, false);
  g_free (filepath);
  g_free (tmp_dir);

  test_helper_zrythm_cleanup ();
#endif
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/graph_export/"

  g_test_add_func (TEST_PREFIX "test svg export", (GTestFunc) test_svg_export);

  return g_test_run ();
}
