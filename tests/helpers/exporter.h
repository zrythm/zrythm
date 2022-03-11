/*
 * Copyright (C) 2022 Alexandros Theodotou <alex at zrythm dot org>
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
  g_assert_cmpint (
    TRANSPORT->playhead_pos.frames, ==, 0);
  char * filename = g_strdup ("test_export.wav");
  ExportSettings settings;
  memset (&settings, 0, sizeof (settings));
  settings.progress_info.has_error = false;
  settings.progress_info.cancelled = false;
  settings.format = AUDIO_FORMAT_WAV;
  settings.artist = g_strdup ("Test Artist");
  settings.title = g_strdup ("Test Title");
  settings.genre = g_strdup ("Test Genre");
  settings.depth = BIT_DEPTH_16;
  settings.time_range = time_range;
  if (mode == EXPORT_MODE_FULL)
    {
      settings.mode = EXPORT_MODE_FULL;
      tracklist_mark_all_tracks_for_bounce (
        TRACKLIST, F_NO_BOUNCE);
      settings.bounce_with_parents = false;
    }
  else
    {
      settings.mode = EXPORT_MODE_TRACKS;
      tracklist_mark_all_tracks_for_bounce (
        TRACKLIST, F_BOUNCE);
      settings.bounce_with_parents = true;
    }
  char * exports_dir =
    project_get_path (
      PROJECT, PROJECT_PATH_EXPORTS, false);
  settings.file_uri =
    g_build_filename (
      exports_dir, filename, NULL);
  int ret = exporter_export (&settings);
  g_assert_false (AUDIO_ENGINE->exporting);
  g_assert_cmpint (ret, ==, 0);
  g_free (filename);

  char * file = g_strdup (settings.file_uri);
  export_settings_free_members (&settings);

  return file;
}

#endif /* __TEST_HELPERS_EXPORTER_H__ */
