// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Exporter helper.
 */

#ifndef __TEST_HELPERS_EXPORTER_H__
#define __TEST_HELPERS_EXPORTER_H__

#include "zrythm-test-config.h"

char *
test_exporter_export_audio (
  ExportTimeRange time_range,
  ExportMode      mode);

/**
 * Convenient quick export.
 */
char *
test_exporter_export_audio (
  ExportTimeRange time_range,
  ExportMode      mode)
{
  g_assert_false (TRANSPORT_IS_ROLLING);
  g_assert_cmpint (TRANSPORT->playhead_pos.frames, ==, 0);
  char *           filename = g_strdup ("test_export.wav");
  ExportSettings * settings = export_settings_new ();
  settings->format = EXPORT_FORMAT_WAV;
  settings->artist = g_strdup ("Test Artist");
  settings->title = g_strdup ("Test Title");
  settings->genre = g_strdup ("Test Genre");
  settings->depth = BIT_DEPTH_16;
  settings->time_range = time_range;
  if (mode == EXPORT_MODE_FULL)
    {
      settings->mode = EXPORT_MODE_FULL;
      tracklist_mark_all_tracks_for_bounce (
        TRACKLIST, F_NO_BOUNCE);
      settings->bounce_with_parents = false;
    }
  else
    {
      settings->mode = EXPORT_MODE_TRACKS;
      tracklist_mark_all_tracks_for_bounce (
        TRACKLIST, F_BOUNCE);
      settings->bounce_with_parents = true;
    }
  char * exports_dir =
    project_get_path (PROJECT, PROJECT_PATH_EXPORTS, false);
  settings->file_uri =
    g_build_filename (exports_dir, filename, NULL);

  EngineState state;
  GPtrArray * conns =
    exporter_prepare_tracks_for_export (settings, &state);

  /* start exporting in a new thread */
  GThread * thread = g_thread_new (
    "bounce_thread",
    (GThreadFunc) exporter_generic_export_thread, settings);

  g_thread_join (thread);

  exporter_post_export (settings, conns, &state);

  g_assert_false (AUDIO_ENGINE->exporting);
  g_free (filename);

  char * file = g_strdup (settings->file_uri);
  export_settings_free (settings);

  return file;
}

#endif /* __TEST_HELPERS_EXPORTER_H__ */
