// SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/gtk_widgets/track.h"
#include "gui/dsp/track.h"

#include "tests/helpers/fishbowl.h"
#include "tests/helpers/fishbowl_window.h"
#include "tests/helpers/zrythm_helper.h"

#include "utils/flags.h"

typedef struct
{
  Region * midi_region;
} RegionFixture;

GtkWidget *
fishbowl_creation_func ()
{
  Track * track = track_new (Track::Type::INSTRUMENT, "track-label", 1);
  z_return_val_if_fail (track, nullptr);
  TrackWidget * itw = track_widget_new (track);
  gtk_widget_set_size_request (GTK_WIDGET (itw), 300, 120);
  track_widget_force_redraw (Z_TRACK_WIDGET (itw));

  return GTK_WIDGET (itw);
}

static void
test_instrument_track_fishbowl ()
{
  guint count =
    fishbowl_window_widget_run (fishbowl_creation_func, DEFAULT_FISHBOWL_TIME);
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, nullptr);

  test_helper_zrythm_init ();
  test_helper_zrythm_gui_init (argc, argv);

#define TEST_PREFIX "/gui/widgets/track/"

  g_test_add_func (
    TEST_PREFIX "test instrument track fishbowl",
    (GTestFunc) test_instrument_track_fishbowl);

  return g_test_run ();
}
