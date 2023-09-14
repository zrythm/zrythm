// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <sys/stat.h>

#include "dsp/audio_region.h"
#include "dsp/automation_point.h"
#include "dsp/automation_track.h"
#include "dsp/channel.h"
#include "dsp/chord_track.h"
#include "dsp/engine.h"
#include "dsp/marker_track.h"
#include "dsp/master_track.h"
#include "dsp/midi_note.h"
#include "dsp/modulator_track.h"
#include "dsp/port_connections_manager.h"
#include "dsp/router.h"
#include "dsp/tempo_track.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "dsp/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/tracklist_selections.h"
#include "gui/widgets/audio_arranger.h"
#include "gui/widgets/audio_editor_space.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/dialogs/project_progress_dialog.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/region.h"
#include "gui/widgets/splash.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "plugins/carla_native_plugin.h"
#include "plugins/lv2/lv2_state.h"
#include "plugins/lv2_plugin.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/datetime.h"
#include "utils/debug.h"
#include "utils/error.h"
#include "utils/file.h"
#include "utils/flags.h"
#include "utils/general.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/progress_info.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "schemas/project.h"
#include <time.h>
#include <zstd.h>

typedef enum
{
  Z_PROJECT_ERROR_FAILED,
} ZProjectError;

#define Z_PROJECT_ERROR z_project_error_quark ()
GQuark
z_project_error_quark (void);
G_DEFINE_QUARK (z - project - error - quark, z_project_error)

bool
project_make_project_dirs (Project * self, bool is_backup, GError ** error)
{
#define MK_PROJECT_DIR(_path) \
  { \
    char * tmp = project_get_path (self, PROJECT_PATH_##_path, is_backup); \
    if (!tmp) \
      { \
        g_set_error_literal ( \
          error, Z_PROJECT_ERROR, Z_PROJECT_ERROR_FAILED, \
          "Failed to get path for " #_path); \
        return false; \
      } \
    GError * err = NULL; \
    bool     success = io_mkdir (tmp, &err); \
    if (!success) \
      { \
        PROPAGATE_PREFIXED_ERROR ( \
          error, err, "Failed to create directory %s", tmp); \
        return false; \
      } \
    g_free_and_null (tmp); \
  }

  MK_PROJECT_DIR (EXPORTS);
  MK_PROJECT_DIR (EXPORTS_STEMS);
  MK_PROJECT_DIR (POOL);
  MK_PROJECT_DIR (PLUGIN_STATES);
  MK_PROJECT_DIR (PLUGIN_EXT_COPIES);
  MK_PROJECT_DIR (PLUGIN_EXT_LINKS);

  return true;
}

/**
 * Compresses/decompress a project from a file/data
 * to a file/data.
 *
 * @param compress True to compress, false to
 *   decompress.
 * @param[out] _dest Pointer to a location to allocate
 *   memory.
 * @param[out] _dest_size Pointer to a location to
 *   store the size of the allocated memory.
 * @param _src Input buffer or filepath.
 * @param _src_size Input buffer size, if not
 *   filepath.
 *
 * @return Whether successful.
 */
bool
_project_compress (
  bool                   compress,
  char **                _dest,
  size_t *               _dest_size,
  ProjectCompressionFlag dest_type,
  const char *           _src,
  const size_t           _src_size,
  ProjectCompressionFlag src_type,
  GError **              error)
{
  g_message (
    "using zstd v%d.%d.%d", ZSTD_VERSION_MAJOR, ZSTD_VERSION_MINOR,
    ZSTD_VERSION_RELEASE);

  char * src = NULL;
  size_t src_size = 0;
  switch (src_type)
    {
    case PROJECT_COMPRESS_DATA:
      src = (char *) _src;
      src_size = _src_size;
      break;
    case PROJECT_COMPRESS_FILE:
      {
        bool ret = g_file_get_contents (_src, &src, &src_size, error);
        if (!ret)
          {
            return false;
          }
      }
      break;
    }

  char * dest = NULL;
  size_t dest_size = 0;
  if (compress)
    {
      g_message ("compressing project...");
      size_t compress_bound = ZSTD_compressBound (src_size);
      dest = malloc (compress_bound);
      dest_size = ZSTD_compress (dest, compress_bound, src, src_size, 1);
      if (ZSTD_isError (dest_size))
        {
          free (dest);

          g_set_error (
            error, Z_PROJECT_ERROR, Z_PROJECT_ERROR_FAILED,
            "Failed to compress project file: %s",
            ZSTD_getErrorName (dest_size));
          return false;
        }
    }
  else /* decompress */
    {
#if (ZSTD_VERSION_MAJOR == 1 && ZSTD_VERSION_MINOR < 3)
      unsigned long long const frame_content_size =
        ZSTD_getDecompressedSize (src, src_size);
      if (frame_content_size == 0)
#else
      unsigned long long const frame_content_size =
        ZSTD_getFrameContentSize (src, src_size);
      if (frame_content_size == ZSTD_CONTENTSIZE_ERROR)
#endif
        {
          g_set_error_literal (
            error, Z_PROJECT_ERROR, Z_PROJECT_ERROR_FAILED,
            "Project not compressed by zstd");
          return false;
        }
      dest = malloc ((size_t) frame_content_size);
      dest_size = ZSTD_decompress (dest, frame_content_size, src, src_size);
      if (ZSTD_isError (dest_size))
        {
          free (dest);

          g_set_error (
            error, Z_PROJECT_ERROR, Z_PROJECT_ERROR_FAILED,
            "Failed to decompress project file: %s",
            ZSTD_getErrorName (dest_size));
          return false;
        }
      if (dest_size != frame_content_size)
        {
          free (dest);

          /* impossible because zstd will check
           * this condition */
          g_set_error_literal (
            error, Z_PROJECT_ERROR, Z_PROJECT_ERROR_FAILED,
            "uncompressed_size != "
            "frame_content_size");
          return false;
        }
    }

  g_message (
    "%s : %u bytes -> %u bytes", compress ? "Compression" : "Decompression",
    (unsigned) src_size, (unsigned) dest_size);

  switch (dest_type)
    {
    case PROJECT_COMPRESS_DATA:
      *_dest = dest;
      *_dest_size = dest_size;
      break;
    case PROJECT_COMPRESS_FILE:
      {
        bool ret = g_file_set_contents (*_dest, dest, (gssize) dest_size, error);
        if (!ret)
          return false;
      }
      break;
    }

  return true;
}

static void
set_datetime_str (Project * self)
{
  if (self->datetime_str)
    g_free (self->datetime_str);
  self->datetime_str = datetime_get_current_as_string ();
}

/**
 * Sets the next available backup dir to use for
 * saving a backup during this call.
 *
 * @return Whether successful.
 */
static bool
set_and_create_next_available_backup_dir (Project * self, GError ** error)
{
  if (self->backup_dir)
    g_free (self->backup_dir);

  char * backups_dir = project_get_path (self, PROJECT_PATH_BACKUPS, false);

  int i = 0;
  do
    {
      if (i > 0)
        {
          g_free (self->backup_dir);
          char * bak_title = g_strdup_printf ("%s.bak%d", self->title, i);
          self->backup_dir = g_build_filename (backups_dir, bak_title, NULL);
          g_free (bak_title);
        }
      else
        {
          char * bak_title = g_strdup_printf ("%s.bak", self->title);
          self->backup_dir = g_build_filename (backups_dir, bak_title, NULL);
          g_free (bak_title);
        }
      i++;
    }
  while (file_exists (self->backup_dir));
  g_free (backups_dir);

  GError * err = NULL;
  bool     success = io_mkdir (self->backup_dir, &err);
  if (!success)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, _ ("Failed to create backup directory %s"),
        self->backup_dir);
      return false;
    }

  return true;
}

/**
 * Checks that everything is okay with the project.
 */
void
project_validate (Project * self)
{
  g_message ("%s: validating...", __func__);

#if 0
  size_t max_size = 20;
  Port ** ports =
    object_new_n (max_size, Port *);
  int num_ports = 0;
  port_get_all (
    &ports, &max_size, true, &num_ports);
  for (int i = 0; i < num_ports; i++)
    {
      Port * port = ports[i];
      port_update_identifier (
        port, &port->id, port->track,
        F_UPDATE_AUTOMATION_TRACK);
    }
  free (ports);
#endif

  tracklist_validate (self->tracklist);

  region_link_group_manager_validate (self->region_link_group_manager);

  /* TODO add arranger_object_get_all and check
   * positions (arranger_object_validate) */

  g_message ("%s: done", __func__);
}

/**
 * @return Whether positions were adjusted.
 */
bool
project_fix_audio_regions (Project * self)
{
  g_message ("fixing audio region positions...");

  int         num_fixed = 0;
  Tracklist * tl = self->tracklist;
  for (int i = 0; i < tl->num_tracks; i++)
    {
      Track * track = tl->tracks[i];
      if (track->type != TRACK_TYPE_AUDIO)
        continue;

      for (int j = 0; j < track->num_lanes; j++)
        {
          TrackLane * lane = track->lanes[j];

          for (int k = 0; k < lane->num_regions; k++)
            {
              ZRegion * r = lane->regions[k];
              if (audio_region_fix_positions (r, 0))
                num_fixed++;
            }
        }
    }

  g_message ("done fixing %d audio region positions", num_fixed);

  return num_fixed > 0;
}

#if 0
ArrangerWidget *
project_get_arranger_for_last_selection (
  Project * self)
{
  ZRegion * r =
    clip_editor_get_region (CLIP_EDITOR);
  switch (self->last_selection)
    {
    case SELECTION_TYPE_TIMELINE:
      return TL_SELECTIONS;
      break;
    case SELECTION_TYPE_EDITOR:
      if (r)
        {
          switch (r->id.type)
            {
            case REGION_TYPE_AUDIO:
              return AUDIO_SELECTIONS;
            case REGION_TYPE_AUTOMATION:
              return AUTOMATION_SELECTIONS;
            case REGION_TYPE_MIDI:
              return MA_SELECTIONS;
            case REGION_TYPE_CHORD:
              return CHORD_SELECTIONS;
            }
        }
      break;
    default:
      return NULL;
    }

  return NULL;
}
#endif

ArrangerSelections *
project_get_arranger_selections_for_last_selection (Project * self)
{
  ZRegion * r = clip_editor_get_region (CLIP_EDITOR);
  switch (self->last_selection)
    {
    case SELECTION_TYPE_TIMELINE:
      return (ArrangerSelections *) TL_SELECTIONS;
      break;
    case SELECTION_TYPE_EDITOR:
      if (r)
        {
          switch (r->id.type)
            {
            case REGION_TYPE_AUDIO:
              return (ArrangerSelections *) AUDIO_SELECTIONS;
            case REGION_TYPE_AUTOMATION:
              return (ArrangerSelections *) AUTOMATION_SELECTIONS;
            case REGION_TYPE_MIDI:
              return (ArrangerSelections *) MA_SELECTIONS;
            case REGION_TYPE_CHORD:
              return (ArrangerSelections *) CHORD_SELECTIONS;
            }
        }
      break;
    default:
      return NULL;
    }

  return NULL;
}

/**
 * Initializes the selections in the project.
 *
 * @note
 * Not meant to be used anywhere besides tests and project.c
 */
void
project_init_selections (Project * self)
{
  self->automation_selections = (AutomationSelections *)
    arranger_selections_new (ARRANGER_SELECTIONS_TYPE_AUTOMATION);
  self->audio_selections = (AudioSelections *) arranger_selections_new (
    ARRANGER_SELECTIONS_TYPE_AUDIO);
  self->chord_selections = (ChordSelections *) arranger_selections_new (
    ARRANGER_SELECTIONS_TYPE_CHORD);
  self->timeline_selections = (TimelineSelections *) arranger_selections_new (
    ARRANGER_SELECTIONS_TYPE_TIMELINE);
  self->midi_arranger_selections = (MidiArrangerSelections *)
    arranger_selections_new (ARRANGER_SELECTIONS_TYPE_MIDI);
  self->mixer_selections = mixer_selections_new ();
  mixer_selections_init (self->mixer_selections);
}

/**
 * Returns the YAML representation of the saved
 * project file.
 *
 * To be free'd with free().
 *
 * @param backup Whether to use the project file
 *   from the most recent backup.
 */
char *
project_get_existing_yaml (Project * self, bool backup, GError ** error)
{
  /* get file contents */
  char * project_file_path =
    project_get_path (self, PROJECT_PATH_PROJECT_FILE, backup);
  g_return_val_if_fail (project_file_path, NULL);
  g_message (
    "%s: getting YAML for project file %s", __func__, project_file_path);

  char *   compressed_pj;
  gsize    compressed_pj_size;
  GError * err = NULL;
  g_file_get_contents (
    project_file_path, &compressed_pj, &compressed_pj_size, &err);
  if (err != NULL)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, _ ("Unable to read file at %s"), project_file_path);
      return NULL;
    }

  /* decompress */
  g_message ("%s: decompressing project...", __func__);
  char * yaml = NULL;
  size_t yaml_size;
  err = NULL;
  bool ret = project_decompress (
    &yaml, &yaml_size, PROJECT_DECOMPRESS_DATA, compressed_pj,
    compressed_pj_size, PROJECT_DECOMPRESS_DATA, &err);
  g_free (compressed_pj);
  if (!ret)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, _ ("Failed to decompress project file at %s"),
        project_file_path);
      return NULL;
    }

  /* make string null-terminated */
  yaml = g_realloc (yaml, yaml_size + sizeof (char));
  yaml[yaml_size] = '\0';

  return yaml;
}

/**
 * Autosave callback.
 *
 * This will keep getting called at regular short intervals,
 * and if enough time has passed and it's okay to save it will
 * autosave, otherwise it will wait until the next interval and
 * check again.
 */
int
project_autosave_cb (void * data)
{
  if (
    !PROJECT || !PROJECT->loaded || !PROJECT->dir || !PROJECT->datetime_str
    || !MAIN_WINDOW || !MAIN_WINDOW->setup)
    return G_SOURCE_CONTINUE;

  unsigned int autosave_interval_mins =
    g_settings_get_uint (S_P_PROJECTS_GENERAL, "autosave-interval");

  /* return if autosave disabled */
  if (autosave_interval_mins <= 0)
    return G_SOURCE_CONTINUE;

  StereoPorts * out_ports;
  gint64        cur_time = g_get_monotonic_time ();
  gint64        microsec_to_autosave =
    (gint64) autosave_interval_mins * 60 * 1000000 -
    /* subtract 4 seconds because the time
       * this gets called is not exact */
    4 * 1000000;

  /* skip if semaphore busy */
  if (zix_sem_try_wait (&PROJECT->save_sem) != ZIX_STATUS_SUCCESS)
    {
      g_message (
        "can't acquire project lock - skipping "
        "autosave");
      return G_SOURCE_CONTINUE;
    }

  GError *         err = NULL;
  bool             success;
  UndoableAction * last_action =
    undo_manager_get_last_action (PROJECT->undo_manager);

  /* skip if bad time to save or rolling */
  if (
    cur_time - PROJECT->last_successful_autosave_time
      < microsec_to_autosave
    || TRANSPORT_IS_ROLLING
    ||
    (TRANSPORT->play_state == PLAYSTATE_ROLL_REQUESTED
     && (TRANSPORT->preroll_frames_remaining > 0
         || TRANSPORT->countin_frames_remaining > 0)))
    {
      goto post_save_sem_and_continue;
    }

  /* skip if sound is playing */
  out_ports = P_MASTER_TRACK->channel->stereo_out;
  if (out_ports->l->peak >= 0.0001f || out_ports->r->peak >= 0.0001f)
    {
      g_debug ("sound is playing, skipping autosave");
      goto post_save_sem_and_continue;
    }

  /* skip if currently performing action */
  if (arranger_widget_any_doing_action ())
    {
      g_debug (
        "in the middle of an action, skipping "
        "autosave");
      goto post_save_sem_and_continue;
    }

  if (PROJECT->last_action_in_last_successful_autosave == last_action)
    {
      g_debug (
        "last action is same as previous backup - skipping "
        "autosave");
      goto post_save_sem_and_continue;
    }

  /* skip if any modal window is open */
  if (ZRYTHM_HAVE_UI)
    {
      GListModel * toplevels = gtk_window_get_toplevels ();
      guint        num_toplevels = g_list_model_get_n_items (toplevels);
      for (guint i = 0; i < num_toplevels; i++)
        {

          GtkWindow * window = GTK_WINDOW (g_list_model_get_item (toplevels, i));
          if (
            gtk_widget_get_visible (GTK_WIDGET (window))
            && (gtk_window_get_modal (window) || gtk_window_get_transient_for (window) == GTK_WINDOW (MAIN_WINDOW)))
            {
              g_debug ("modal/transient windows exist - skipping autosave");
              z_gtk_widget_print_hierarchy (GTK_WIDGET (window));
              goto post_save_sem_and_continue;
            }
        }
    }

  /* ok to save */
  success = project_save (
    PROJECT, PROJECT->dir, F_BACKUP, Z_F_SHOW_NOTIFICATION, F_ASYNC, &err);
  if (success)
    {
      PROJECT->last_successful_autosave_time = cur_time;
    }
  else
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to save project"));
    }

post_save_sem_and_continue:
  zix_sem_post (&PROJECT->save_sem);

  return G_SOURCE_CONTINUE;
}

/**
 * Returns the requested project path as a newly
 * allocated string.
 *
 * @param backup Whether to get the path for the
 *   current backup instead of the main project.
 */
char *
project_get_path (Project * self, ProjectPath path, bool backup)
{
  g_return_val_if_fail (self->dir, NULL);
  char * dir = backup ? self->backup_dir : self->dir;
  switch (path)
    {
    case PROJECT_PATH_BACKUPS:
      return g_build_filename (dir, PROJECT_BACKUPS_DIR, NULL);
    case PROJECT_PATH_EXPORTS:
      return g_build_filename (dir, PROJECT_EXPORTS_DIR, NULL);
    case PROJECT_PATH_EXPORTS_STEMS:
      return g_build_filename (
        dir, PROJECT_EXPORTS_DIR, PROJECT_STEMS_DIR, NULL);
    case PROJECT_PATH_PLUGINS:
      return g_build_filename (dir, PROJECT_PLUGINS_DIR, NULL);
    case PROJECT_PATH_PLUGIN_STATES:
      {
        char * plugins_dir =
          project_get_path (self, PROJECT_PATH_PLUGINS, backup);
        char * ret =
          g_build_filename (plugins_dir, PROJECT_PLUGIN_STATES_DIR, NULL);
        g_free (plugins_dir);
        return ret;
      }
      break;
    case PROJECT_PATH_PLUGIN_EXT_COPIES:
      {
        char * plugins_dir =
          project_get_path (self, PROJECT_PATH_PLUGINS, backup);
        char * ret =
          g_build_filename (plugins_dir, PROJECT_PLUGIN_EXT_COPIES_DIR, NULL);
        g_free (plugins_dir);
        return ret;
      }
      break;
    case PROJECT_PATH_PLUGIN_EXT_LINKS:
      {
        char * plugins_dir =
          project_get_path (self, PROJECT_PATH_PLUGINS, backup);
        char * ret =
          g_build_filename (plugins_dir, PROJECT_PLUGIN_EXT_LINKS_DIR, NULL);
        g_free (plugins_dir);
        return ret;
      }
      break;
    case PROJECT_PATH_POOL:
      return g_build_filename (dir, PROJECT_POOL_DIR, NULL);
    case PROJECT_PATH_PROJECT_FILE:
      return g_build_filename (dir, PROJECT_FILE, NULL);
    case PROJECT_PATH_FINISHED_FILE:
      return g_build_filename (dir, PROJECT_FINISHED_FILE, NULL);
    default:
      g_return_val_if_reached (NULL);
    }
  g_return_val_if_reached (NULL);
}

void
project_init_common (Project * self)
{
  zix_sem_init (&self->save_sem, 1);
}

/**
 * Creates an empty project object.
 */
Project *
project_new (Zrythm * _zrythm)
{
  g_message ("%s: Creating...", __func__);

  Project * self = object_new (Project);
  self->schema_version = PROJECT_SCHEMA_VERSION;

  if (_zrythm)
    {
      _zrythm->project = self;
    }

  self->version = zrythm_get_version (0);
  self->clip_editor = clip_editor_new ();
  self->timeline = timeline_new ();
  self->snap_grid_timeline = snap_grid_new ();
  self->snap_grid_editor = snap_grid_new ();
  self->quantize_opts_timeline = quantize_options_new ();
  self->quantize_opts_editor = quantize_options_new ();
  self->tracklist_selections = tracklist_selections_new (true);
  self->region_link_group_manager = region_link_group_manager_new ();
  self->port_connections_manager = port_connections_manager_new ();
  self->midi_mappings = midi_mappings_new ();

  project_init_common (self);

  g_message ("%s: done", __func__);

  return self;
}

ProjectSaveData *
project_save_data_new (void)
{
  ProjectSaveData * self = object_new_unresizable (ProjectSaveData);
  self->progress_info = progress_info_new ();

  return self;
}

void
project_save_data_free (ProjectSaveData * self)
{
  g_free_and_null (self->project_file_path);
  object_free_w_func_and_null (project_free, self->project);
  object_free_w_func_and_null (progress_info_free, self->progress_info);

  object_zero_and_free_unresizable (ProjectSaveData, self);
}

/**
 * Thread that does the serialization and saving.
 */
static void *
serialize_project_thread (ProjectSaveData * data)
{
  char * compressed_yaml;
  size_t compressed_size;
  bool   ret;

  /* generate yaml */
  g_message ("serializing project to yaml...");
  GError * err = NULL;
  gint64   time_before = g_get_monotonic_time ();
  char *   yaml = yaml_serialize (data->project, &project_schema, &err);
  gint64   time_after = g_get_monotonic_time ();
  g_message (
    "time to serialize: %ldms", (long) (time_after - time_before) / 1000);
  if (!yaml)
    {
      HANDLE_ERROR_LITERAL (err, _ ("Failed to serialize project"));
      data->has_error = true;
      goto serialize_end;
    }

  /* compress */
  err = NULL;
  ret = project_compress (
    &compressed_yaml, &compressed_size, PROJECT_COMPRESS_DATA, yaml,
    strlen (yaml) * sizeof (char), PROJECT_COMPRESS_DATA, &err);
  g_free (yaml);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to compress project file"));
      data->has_error = true;
      goto serialize_end;
    }

  /* set file contents */
  g_message (
    "%s: saving project file at %s...", __func__, data->project_file_path);
  g_file_set_contents (
    data->project_file_path, compressed_yaml, (gssize) compressed_size, &err);
  free (compressed_yaml);
  if (err != NULL)
    {
      g_critical (
        "%s: Unable to write project file: %s", __func__, err->message);
      g_error_free (err);
      data->has_error = true;
    }

  g_message ("%s: successfully saved project", __func__);

serialize_end:
  zix_sem_post (&UNDO_MANAGER->action_sem);
  data->finished = true;
  return NULL;
}

/**
 * Idle func to check if the project has finished
 * saving and show a notification.
 */
static int
project_idle_saved_cb (ProjectSaveData * data)
{
  if (!data->finished)
    {
      return G_SOURCE_CONTINUE;
    }

  if (data->is_backup)
    {
      g_message (_ ("Backup saved."));
      if (ZRYTHM_HAVE_UI)
        {
          ui_show_notification (_ ("Backup saved."));
        }
    }
  else
    {
      if (!ZRYTHM_TESTING)
        {
          zrythm_add_to_recent_projects (ZRYTHM, data->project_file_path);
        }
      if (data->show_notification)
        ui_show_notification (_ ("Project saved."));
    }

  if (ZRYTHM_HAVE_UI && PROJECT->loaded && MAIN_WINDOW)
    {
      EVENTS_PUSH (ET_PROJECT_SAVED, PROJECT);
    }

  progress_info_mark_completed (
    data->progress_info, PROGRESS_COMPLETED_SUCCESS, NULL);

  return G_SOURCE_REMOVE;
}

/**
 * Cleans up unnecessary plugin state dirs from the
 * main project.
 */
static void
cleanup_plugin_state_dirs (ProjectSaveData * data)
{
  g_debug (
    "cleaning plugin state dirs%s...", data->is_backup ? " for backup" : "");

  /* if saving backup, the temporary state dirs
     * created during clone() are not needed by
     * the main project so we just check what is
     * needed from the main project and delete the
     * rest.
     * if saving the main project, the newly copied
     * state dirs are used instead of the old ones,
     * so we check the cloned project and delete
     * the rest
     *
     * that said, there is still an issue
     * (see https://todo.sr.ht/~alextee/zrythm-bug/1047)
     * so just don't delete any plugins in either the clone
     * or the current/main project
     * */
  GPtrArray * arr = g_ptr_array_new ();
  plugin_get_all (PROJECT, arr, true);
  plugin_get_all (data->project, arr, true);
  for (size_t i = 0; i < arr->len; i++)
    {
      Plugin * pl = (Plugin *) g_ptr_array_index (arr, i);
      g_debug ("plugin %zu: %s", i, pl->state_dir);
    }

  char * plugin_states_path =
    project_get_path (PROJECT, PROJECT_PATH_PLUGIN_STATES, F_NOT_BACKUP);

  /* get existing plugin state dirs */
  GDir *   dir;
  GError * error = NULL;
  dir = g_dir_open (plugin_states_path, 0, &error);
  g_return_if_fail (error == NULL);
  const char * filename;
  while ((filename = g_dir_read_name (dir)))
    {
      char * full_path = g_build_filename (plugin_states_path, filename, NULL);

      /* skip non-dirs */
      if (!g_file_test (full_path, G_FILE_TEST_IS_DIR))
        {
          g_free (full_path);
          continue;
        }

      /*g_debug ("filename to check: %s", filename);*/

      /* if not found in the current plugins,
       * remove the dir */
      bool found = false;
      for (size_t i = 0; i < arr->len; i++)
        {
          Plugin * pl = (Plugin *) g_ptr_array_index (arr, i);
          if (string_is_equal (filename, pl->state_dir))
            {
              found = true;
              /*g_debug ("found");*/
              break;
            }
        }

      if (!found)
        {
          g_message ("removing unused plugin state in %s", full_path);
          io_rmdir (full_path, Z_F_FORCE);
        }

      g_free (full_path);
    }
  g_dir_close (dir);

  g_ptr_array_unref (arr);

  g_free (plugin_states_path);

  g_debug ("cleaned plugin state dirs");
}

/**
 * Saves the project to a project file in the
 * given dir.
 *
 * @param is_backup 1 if this is a backup. Backups
 *   will be saved as <original filename>.bak<num>.
 * @param show_notification Show a notification
 *   in the UI that the project was saved.
 * @param async Save asynchronously in another
 *   thread.
 *
 * @return Whether successful.
 */
bool
project_save (
  Project *    self,
  const char * _dir,
  const bool   is_backup,
  const bool   show_notification,
  const bool   async,
  GError **    error)
{
  /* pause engine */
  EngineState state;
  bool        engine_paused = false;
  if (AUDIO_ENGINE->activated)
    {
      engine_wait_for_pause (AUDIO_ENGINE, &state, Z_F_NO_FORCE, true);
      engine_paused = true;
    }

  if (async)
    {
      zix_sem_wait (&UNDO_MANAGER->action_sem);
    }

  project_validate (self);

  char * dir = g_strdup (_dir);

  /* set the dir and create it if it doesn't
   * exist */
  g_free_and_null (self->dir);
  self->dir = g_strdup (dir);
  GError * err = NULL;
  bool     success = io_mkdir (PROJECT->dir, &err);
  if (!success)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "Failed to create project directory %s", PROJECT->dir);
      return false;
    }

  /* set the title */
  char * basename = g_path_get_basename (dir);
  g_free_and_null (self->title);
  self->title = g_strdup (basename);
  g_free (basename);
  g_free (dir);

  /* save current datetime */
  set_datetime_str (self);

  /* set the project version */
  g_free_and_null (self->version);
  self->version = zrythm_get_version (false);

  /* if backup, get next available backup dir */
  if (is_backup)
    {
      success = set_and_create_next_available_backup_dir (self, &err);
      if (!success)
        {
          PROPAGATE_PREFIXED_ERROR_LITERAL (
            error, err, "Failed to create backup directory");
          return false;
        }
    }

  success = project_make_project_dirs (self, is_backup, &err);
  if (!success)
    {
      PROPAGATE_PREFIXED_ERROR_LITERAL (
        error, err, "Failed to create project directories");
      return false;
    }

  /* write the pool */
  audio_pool_remove_unused (AUDIO_POOL, is_backup);
  success = audio_pool_write_to_disk (AUDIO_POOL, is_backup, &err);
  if (!success)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "%s", _ ("Failed to write audio pool to disk"));
      return false;
    }

  ProjectSaveData * data = project_save_data_new ();
  data->project_file_path =
    project_get_path (self, PROJECT_PATH_PROJECT_FILE, is_backup);
  data->show_notification = show_notification;
  data->is_backup = is_backup;
  data->project = project_clone (PROJECT, is_backup, &err);
  if (!data->project)
    {
      PROPAGATE_PREFIXED_ERROR (error, err, "%s", _ ("Failed to clone project"));
      return false;
    }
  g_return_val_if_fail (data->project->tracklist_selections, false);
  data->project->tracklist_selections->free_tracks = true;

#if 0
  /* write plugin states */
  GPtrArray * plugins =
    g_ptr_array_new_full (100, NULL);
  tracklist_get_plugins (self->tracklist, plugins);
  for (size_t i = 0; i < plugins->len; i++)
    {
      Plugin * pl = g_ptr_array_index (plugins, i);

      if (!pl->instantiated)
        {
          g_debug (
            "skipping uninstantiated plugin %s...",
            pl->setting->descr->name);
          continue;
        }

      /* save state */
#  ifdef HAVE_CARLA
      if (pl->setting->open_with_carla)
        {
          carla_native_plugin_save_state (
            pl->carla, is_backup, NULL);
        }
      else
        {
#  endif
          switch (pl->setting->descr->protocol)
            {
            case PROT_LV2:
              {
                LilvState * tmp_state =
                  lv2_state_save_to_file (
                    pl->lv2, is_backup);
                lilv_state_free (tmp_state);
              }
              break;
            default:
              g_warn_if_reached ();
              break;
            }
#  ifdef HAVE_CARLA
        }
#  endif
    }
#endif

  if (is_backup)
    {
      /* copy plugin states */
      char * prj_pl_states_dir =
        project_get_path (PROJECT, PROJECT_PATH_PLUGINS, F_NOT_BACKUP);
      char * prj_backup_pl_states_dir =
        project_get_path (PROJECT, PROJECT_PATH_PLUGINS, F_BACKUP);
      success = io_copy_dir (
        prj_backup_pl_states_dir, prj_pl_states_dir, F_NO_FOLLOW_SYMLINKS,
        F_RECURSIVE, &err);
      if (!success)
        {
          PROPAGATE_PREFIXED_ERROR_LITERAL (
            error, err, "Failed to copy plugin states");
          return false;
        }
    }

  if (!is_backup)
    {
      /* cleanup unused plugin states (or do it when executing
       * mixer/tracklist selection actions) */
      cleanup_plugin_state_dirs (data);
    }

  /* TODO verify all plugin states exist */

  if (async)
    {
      g_thread_new (
        "serialize_project_thread", (GThreadFunc) serialize_project_thread,
        data);

      /* don't show progress dialog */
      if (ZRYTHM_HAVE_UI && false)
        {
          g_idle_add ((GSourceFunc) project_idle_saved_cb, data);

#if 0
          /* show progress while saving */
          ProjectProgressDialogWidget * dialog =
            project_progress_dialog_widget_new (data);
          g_return_val_if_fail (
            Z_IS_MAIN_WINDOW_WIDGET (MAIN_WINDOW), -1);
          gtk_window_set_transient_for (
            GTK_WINDOW (dialog), GTK_WINDOW (MAIN_WINDOW));
          gtk_window_set_modal (GTK_WINDOW (dialog), true);
          z_gtk_dialog_run (GTK_DIALOG (dialog), true);
#endif
        }
      else
        {
          while (!data->finished)
            {
              g_usleep (1000);
            }
          project_idle_saved_cb (data);
        }
    }
  else /* else if no async */
    {
      /* call synchronously */
      serialize_project_thread (data);
      project_idle_saved_cb (data);
    }

  object_free_w_func_and_null (project_save_data_free, data);

  /* write FINISHED file */
  {
    char * finished_file_path =
      project_get_path (self, PROJECT_PATH_FINISHED_FILE, is_backup);
    io_touch_file (finished_file_path);
    g_free (finished_file_path);
  }

  if (ZRYTHM_TESTING)
    tracklist_validate (self->tracklist);

  UndoableAction * last_action =
    undo_manager_get_last_action (self->undo_manager);
  if (is_backup)
    {
      self->last_action_in_last_successful_autosave = last_action;
    }
  else
    {
      self->last_saved_action = last_action;
    }

  if (engine_paused)
    {
      engine_resume (AUDIO_ENGINE, &state);
    }

  return true;
}

bool
project_has_unsaved_changes (const Project * self)
{
  /* simply check if the last performed action matches the
   * last action when the project was last saved/loaded */
  UndoableAction * last_performed_action =
    undo_manager_get_last_action (self->undo_manager);
  return last_performed_action != self->last_saved_action;
}

/**
 * Deep-clones the given project.
 *
 * To be used during save on the main thread.
 *
 * @param for_backup Whether the resulting project
 *   is for a backup.
 */
Project *
project_clone (const Project * src, bool for_backup, GError ** error)
{
  g_return_val_if_fail (ZRYTHM_APP_IS_GTK_THREAD, NULL);
  g_message ("cloning project...");

  Project * self = object_new (Project);
  self->schema_version = PROJECT_SCHEMA_VERSION;

  self->title = g_strdup (src->title);
  self->datetime_str = g_strdup (src->datetime_str);
  self->version = g_strdup (src->version);
  self->tracklist = tracklist_clone (src->tracklist);
  self->clip_editor = clip_editor_clone (src->clip_editor);
  self->timeline = timeline_clone (src->timeline);
  self->snap_grid_timeline = snap_grid_clone (src->snap_grid_timeline);
  self->snap_grid_editor = snap_grid_clone (src->snap_grid_editor);
  self->quantize_opts_timeline =
    quantize_options_clone (src->quantize_opts_timeline);
  self->quantize_opts_editor =
    quantize_options_clone (src->quantize_opts_editor);
  self->audio_engine = engine_clone (src->audio_engine);
  self->mixer_selections =
    mixer_selections_clone (src->mixer_selections, F_PROJECT);
  self->timeline_selections = (TimelineSelections *) arranger_selections_clone (
    (ArrangerSelections *) src->timeline_selections);
  self->midi_arranger_selections =
    (MidiArrangerSelections *) arranger_selections_clone (
      (ArrangerSelections *) src->midi_arranger_selections);
  self->chord_selections = (ChordSelections *) arranger_selections_clone (
    (ArrangerSelections *) src->chord_selections);
  self->automation_selections = (AutomationSelections *)
    arranger_selections_clone ((ArrangerSelections *) src->automation_selections);
  self->audio_selections = (AudioSelections *) arranger_selections_clone (
    (ArrangerSelections *) src->audio_selections);
  GError * err = NULL;
  self->tracklist_selections =
    tracklist_selections_clone (src->tracklist_selections, &err);
  if (!self->tracklist_selections)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "%s", "Failed to clone track selections");
      return NULL;
    }
  self->tracklist_selections->is_project = true;
  self->region_link_group_manager =
    region_link_group_manager_clone (src->region_link_group_manager);
  self->port_connections_manager =
    port_connections_manager_clone (src->port_connections_manager);
  self->midi_mappings = midi_mappings_clone (src->midi_mappings);

  /* no undo history in backups */
  if (!for_backup)
    {
      self->undo_manager = undo_manager_clone (src->undo_manager);
    }

  g_message ("finished cloning project");

  return self;
}

/**
 * Frees the selections in the project.
 */
static void
free_arranger_selections (Project * self)
{
  object_free_w_func_and_null_cast (
    arranger_selections_free, ArrangerSelections *, self->automation_selections);
  object_free_w_func_and_null_cast (
    arranger_selections_free, ArrangerSelections *, self->audio_selections);
  object_free_w_func_and_null_cast (
    arranger_selections_free, ArrangerSelections *, self->chord_selections);
  object_free_w_func_and_null_cast (
    arranger_selections_free, ArrangerSelections *, self->timeline_selections);
  object_free_w_func_and_null_cast (
    arranger_selections_free, ArrangerSelections *,
    self->midi_arranger_selections);
}

/**
 * Tears down the project.
 */
void
project_free (Project * self)
{
  g_message ("%s: tearing down...", __func__);

  self->loaded = false;

  g_free_and_null (self->title);

  if (self->audio_engine && self->audio_engine->activated)
    {
      engine_activate (self->audio_engine, false);
    }
  /* remove region from clip editor to avoid
   * lookups for it when removing regions from
   * track */
  self->clip_editor->has_region = false;

  object_free_w_func_and_null (undo_manager_free, self->undo_manager);

  /* must be free'd before tracklist selections,
   * mixer selections, engine, and port connection
   * manager */
  object_free_w_func_and_null (tracklist_free, self->tracklist);

  object_free_w_func_and_null (midi_mappings_free, self->midi_mappings);
  object_free_w_func_and_null (clip_editor_free, self->clip_editor);
  object_free_w_func_and_null (timeline_free, self->timeline);
  object_free_w_func_and_null (snap_grid_free, self->snap_grid_timeline);
  object_free_w_func_and_null (snap_grid_free, self->snap_grid_editor);
  object_free_w_func_and_null (
    quantize_options_free, self->quantize_opts_timeline);
  object_free_w_func_and_null (
    quantize_options_free, self->quantize_opts_editor);
  object_free_w_func_and_null (
    region_link_group_manager_free, self->region_link_group_manager);

  object_free_w_func_and_null (
    tracklist_selections_free, self->tracklist_selections);

  free_arranger_selections (self);

  object_free_w_func_and_null (engine_free, self->audio_engine);

  /* must be free'd after engine */
  object_free_w_func_and_null (
    port_connections_manager_free, self->port_connections_manager);

  /* must be free'd after port connections
   * manager */
  object_free_w_func_and_null (mixer_selections_free, self->mixer_selections);

  zix_sem_destroy (&self->save_sem);

  object_zero_and_free (self);

  g_message ("%s: free'd project", __func__);
}
