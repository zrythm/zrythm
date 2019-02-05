/*
 * gui/widgets/file_browser.c - The file file_browser on
 *   the right
 *
 * Copyright (C) 2019 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <math.h>

#include "zrythm.h"
#include "audio/audio_region.h"
#include "audio/audio_track.h"
#include "audio/engine.h"
#include "audio/mixer.h"
#include "audio/region.h"
#include "ext/audio_decoder/ad.h"
#include "gui/backend/file_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/file_browser.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/right_dock_edge.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/tracklist.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/io.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

#include <sndfile.h>
#include <samplerate.h>

G_DEFINE_TYPE (FileBrowserWidget,
               file_browser_widget,
               GTK_TYPE_PANED)

enum
{
  COLUMN_ICON,
  COLUMN_NAME,
  COLUMN_DESCR,
  NUM_COLUMNS
};

/**
 * Visible function for file tree model.
 *
 * Used for filtering based on selected category.
 */
static gboolean
visible_func (GtkTreeModel *model,
              GtkTreeIter  *iter,
              gpointer      data)
{
  /*FileBrowserWidget * self = Z_FILE_BROWSER_WIDGET (data);*/

  return TRUE;
}

static int
update_file_info_label (FileBrowserWidget * self,
                          gpointer user_data)
{
  char * label = (char *) user_data;

  gtk_label_set_text (self->file_info, label);

  return G_SOURCE_REMOVE;
}

static void
on_selection_changed (GtkTreeSelection * ts,
                      gpointer         user_data)
{
  FileBrowserWidget * self =
    Z_FILE_BROWSER_WIDGET (user_data);
  GtkTreeView * tv =
    gtk_tree_selection_get_tree_view (ts);
  GtkTreeModel * model =
    gtk_tree_view_get_model (tv);
  GList * selected_rows =
    gtk_tree_selection_get_selected_rows (ts,
                                          NULL);
  if (selected_rows)
    {
      GtkTreePath * tp =
        (GtkTreePath *) g_list_first (selected_rows)->data;
      GtkTreeIter iter;
      gtk_tree_model_get_iter (model,
                               &iter,
                               tp);
      GValue value = G_VALUE_INIT;

      if (model == self->type_tree_model)
        {
          gtk_tree_model_get_value (model,
                                    &iter,
                                    0,
                                    &value);
          self->selected_type =
            g_value_get_pointer (&value);
          gtk_tree_model_filter_refilter (
            self->files_tree_model);
        }
      else if (model ==
               GTK_TREE_MODEL (self->files_tree_model))
        {
          gtk_tree_model_get_value (model,
                                    &iter,
                                    COLUMN_DESCR,
                                    &value);
          FileDescriptor * descr =
            g_value_get_pointer (&value);
          FileType * ft =
            file_manager_get_file_type_from_enum (
              descr->type);


          char * label;
          if (descr->type == FILE_TYPE_MP3 ||
              descr->type == FILE_TYPE_FLAC ||
              descr->type == FILE_TYPE_OGG ||
              descr->type == FILE_TYPE_WAV)
            {
              /* open with sndfile */
              struct adinfo nfo;
              ad_finfo (descr->absolute_path,
                        &nfo);
              ad_dump_nfo (3, &nfo);
              /*SF_INFO sfinfo;*/
              /*SNDFILE * sndfile =*/
                /*sf_open (descr->absolute_path,*/
                         /*SFM_READ,*/
                         /*&sfinfo);*/
              label =
                g_strdup_printf (
                "%s\nFormat: TODO\nSample rate: %d\n"
                "Channels:%d Bitrate: %d\nBit depth: %d",
                descr->label,
                nfo.sample_rate,
                nfo.channels,
                nfo.bit_rate,
                nfo.bit_depth);
              /*sf_close (sndfile);*/
            }
          else
            label =
              g_strdup_printf (
              "%s\nType: %s",
              descr->label,
              ft->label);
          update_file_info_label (self,
                                  label);
        }
    }
}

static void
on_drag_data_get (GtkWidget        *widget,
               GdkDragContext   *context,
               GtkSelectionData *data,
               guint             info,
               guint             time,
               gpointer          user_data)
{
  GtkTreeSelection * ts = (GtkTreeSelection *) user_data;
  GList * selected_rows =
    gtk_tree_selection_get_selected_rows (ts,
                                          NULL);
  GtkTreePath * tp =
    (GtkTreePath *) g_list_first (selected_rows)->data;
  GtkTreeIter iter;
  gtk_tree_model_get_iter (
    GTK_TREE_MODEL (MW_FILE_BROWSER->files_tree_model),
    &iter,
    tp);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (
    GTK_TREE_MODEL (MW_FILE_BROWSER->files_tree_model),
    &iter,
    1,
    &value);
  FileDescriptor * descr = g_value_get_pointer (&value);

  gtk_selection_data_set (
    data,
    gdk_atom_intern_static_string ("FILE_DESCR"),
    32,
    (const guchar *)&descr,
    sizeof (FileDescriptor));
}

static GtkTreeModel *
create_model_for_types ()
{
  GtkListStore *list_store;
  /*GtkTreePath *path;*/
  GtkTreeIter iter;
  gint i;

  /* file name, index */
  list_store = gtk_list_store_new (2,
                                   G_TYPE_STRING,
                                   G_TYPE_POINTER);

  for (i = 0; i < NUM_FILE_TYPES; i++)
    {
      FileType * ft =
        FILE_MANAGER->file_types[i];

      // Add a new row to the model
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter,
        0, ft->label,
        1, ft,
        -1);
    }

  return GTK_TREE_MODEL (list_store);
}

static GtkTreeModel *
create_model_for_locations ()
{
  GtkListStore *list_store;
  /*GtkTreePath *path;*/
  GtkTreeIter iter;
  gint i;

  /* file name, index */
  list_store = gtk_list_store_new (2,
                                   G_TYPE_STRING,
                                   G_TYPE_POINTER);

  for (i = 0; i < FILE_MANAGER->num_locations; i++)
    {
      FileBrowserLocation * loc =
        FILE_MANAGER->locations[i];

      // Add a new row to the model
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter,
        0, loc->label,
        1, loc,
        -1);
    }

  return GTK_TREE_MODEL (list_store);
}

static GtkTreeModel *
create_model_for_files (FileBrowserWidget * self)
{
  GtkListStore *list_store;
  /*GtkTreePath *path;*/
  GtkTreeIter iter;
  gint i;

  /* file name, index */
  list_store =
    gtk_list_store_new (NUM_COLUMNS,
                        G_TYPE_STRING,
                        G_TYPE_STRING,
                        G_TYPE_POINTER);

  for (i = 0; i < FILE_MANAGER->num_files; i++)
    {
      FileDescriptor * descr =
        FILE_MANAGER->files[i];

      gchar * icon_name;
      switch (descr->type)
        {
        case FILE_TYPE_MIDI:
          icon_name = "audio-midi";
          break;
        case FILE_TYPE_MP3:
          icon_name = "audio-mp3";
          break;
        case FILE_TYPE_FLAC:
          icon_name = "audio-flac";
          break;
        case FILE_TYPE_OGG:
          icon_name = "application-ogg";
          break;
        case FILE_TYPE_WAV:
          icon_name = "audio-x-wav";
          break;
        case FILE_TYPE_DIR:
          icon_name = "inode-directory";
          break;
        case FILE_TYPE_PARENT_DIR:
          icon_name = "inode-directory";
          break;
        case FILE_TYPE_OTHER:
          icon_name = "none";
          break;
        case NUM_FILE_TYPES:
          break;
        }

      // Add a new row to the model
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter,
        COLUMN_ICON, icon_name,
        COLUMN_NAME, descr->label,
        COLUMN_DESCR, descr,
        -1);
    }

  GtkTreeModel * model =
    gtk_tree_model_filter_new (
      GTK_TREE_MODEL (list_store),
      NULL);
  gtk_tree_model_filter_set_visible_func (
    GTK_TREE_MODEL_FILTER (model),
    visible_func,
    self,
    NULL);

  return model;
}

static void
on_row_activated (GtkTreeView       *tree_view,
               GtkTreePath       *tp,
               GtkTreeViewColumn *column,
               gpointer           user_data)
{
  FileBrowserWidget * self =
    MW_FILE_BROWSER;
  GtkTreeModel * model =
    GTK_TREE_MODEL (self->files_tree_model);
  GtkTreeIter iter;
  gtk_tree_model_get_iter (
    model,
    &iter,
    tp);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (
    model,
    &iter,
    COLUMN_DESCR,
    &value);
  FileDescriptor * descr =
    g_value_get_pointer (&value);

  g_message ("activated file type %d, abs path %s",
             descr->type,
             descr->absolute_path);
  if (descr->type == FILE_TYPE_DIR ||
      descr->type == FILE_TYPE_PARENT_DIR)
    {
      /* FIXME free unnecessary stuff */
      FileBrowserLocation * loc =
        malloc (sizeof (FileBrowserLocation));
      loc->path = descr->absolute_path;
      loc->label = g_path_get_basename (loc->path);
      file_manager_set_selection (
        loc,
        FB_SELECTION_TYPE_LOCATIONS,
        1);
      self->files_tree_model =
        GTK_TREE_MODEL_FILTER (
          create_model_for_files (self));
      gtk_tree_view_set_model (
        self->files_tree_view,
        GTK_TREE_MODEL (self->files_tree_model));
    }
  else if (descr->type == FILE_TYPE_WAV ||
           descr->type == FILE_TYPE_OGG ||
           descr->type == FILE_TYPE_FLAC ||
           descr->type == FILE_TYPE_MP3)
    {
      /* open with sndfile */
      /*SF_INFO sfinfo;*/
      /*SNDFILE * sndfile =*/
        /*sf_open (descr->absolute_path,*/
                 /*SFM_READ,*/
                 /*&sfinfo);*/
      /*float in_buff[sfinfo.frames * sfinfo.channels];*/
      /*sf_readf_float (sndfile,*/
                      /*in_buff,*/
                      /*sfinfo.frames * sfinfo.channels);*/

      /* open with ad */
      struct adinfo nfo;
      void * handle =
        ad_open (descr->absolute_path,
                 &nfo);
      long in_buff_size = nfo.frames * nfo.channels;
      /* add some extra buffer for some reason */
      float * in_buff =
        malloc (in_buff_size * sizeof (float));
      long samples_read =
        ad_read (handle,
                 in_buff,
                 in_buff_size);
      g_message ("in buff size: %ld, %ld samples read",
                 in_buff_size,
                 samples_read);

      /* resample with libsamplerate */
      double src_ratio =
        (1.0 * AUDIO_ENGINE->sample_rate) /
        nfo.sample_rate ;
      if (fabs (src_ratio - 1.0) < 1e-20)
        {
          g_message ("Target samplerate and input "
                     "samplerate are the same.");
        }
      if (src_is_valid_ratio (src_ratio) == 0)
        {
          g_warning ("Sample rate change out of valid "
                     "range.");
        }
      long out_buff_size =
        in_buff_size * src_ratio;
      /* add some extra buffer for some reason */
      float * out_buff =
        malloc (out_buff_size * sizeof (float));
      SRC_DATA src_data;
      g_message ("out_buff_size %ld, sizeof float %ld, "
                 "sizeof long %ld, src ratio %f",
                 out_buff_size,
                 sizeof (float),
                 sizeof (long),
                 src_ratio);
      src_data.data_in = &in_buff[0];
      src_data.data_out = &out_buff[0];
      src_data.input_frames = nfo.frames;
      src_data.output_frames = nfo.frames * src_ratio;
      src_data.src_ratio = src_ratio;

      int err =
        src_simple (&src_data,
                    SRC_SINC_BEST_QUALITY,
                    nfo.channels);
      g_message ("output frames gen %ld, out buff size %ld, "
                 "input frames used %ld, err %d",
                 src_data.output_frames_gen,
                 out_buff_size,
                 src_data.input_frames_used,
                 err);

      /* create a channel/track */
      Channel * chan =
        channel_create (CT_AUDIO, "Audio Track");
      mixer_add_channel (chan);
      tracklist_append_track (chan->track);

      /* create an audio region & add to track */
      Position start_pos, end_pos;
      position_set_to_pos (&start_pos,
                           &PLAYHEAD);
      position_set_to_pos (&end_pos,
                           &PLAYHEAD);
      position_add_frames (&end_pos,
                           src_data.output_frames_gen /
                           nfo.channels);
      AudioRegion * ar =
        audio_region_new (chan->track,
                          &start_pos,
                          &end_pos);
      audio_track_add_region ((AudioTrack *) chan->track,
                              ar);

      /* create an audio clip and add to region */
      AudioClip * ac =
        audio_clip_new (ar,
                        out_buff,
                        src_data.output_frames_gen,
                        nfo.channels,
                        descr->absolute_path);
      audio_region_add_audio_clip (ar,
                                   ac);

      /* refresh tracklist */
      mixer_widget_refresh (MW_MIXER);
      tracklist_widget_hard_refresh (MW_TRACKLIST);
      arranger_widget_refresh (
        Z_ARRANGER_WIDGET (MW_TIMELINE));

      /* cleanup */
      ad_close (handle);
      free (in_buff);
    }
}


static void
expander_callback (GObject    *object,
                   GParamSpec *param_spec,
                   gpointer    user_data)
{
  GtkExpander *expander;

  GtkWidget * scrolled_window = GTK_WIDGET (user_data);
  expander = GTK_EXPANDER (object);

  if (gtk_expander_get_expanded (expander))
    {
      gtk_widget_set_vexpand (GTK_WIDGET (scrolled_window),
                             TRUE);
    }
  else
    {
      gtk_widget_set_vexpand (GTK_WIDGET (scrolled_window),
                             FALSE);
    }
}

static GtkWidget *
tree_view_create (FileBrowserWidget * self,
                  GtkTreeModel * model,
                  int          allow_multi,
                  int          dnd)
{
  /* instantiate tree view using model */
  GtkWidget * tree_view = gtk_tree_view_new_with_model (
      GTK_TREE_MODEL (model));

  /* init tree view */
  GtkCellRenderer * renderer;
  GtkTreeViewColumn * column;
  if (GTK_TREE_MODEL (self->files_tree_model) == model)
    {
      /* column for icon */
      renderer =
        gtk_cell_renderer_pixbuf_new ();
      column =
        gtk_tree_view_column_new_with_attributes (
          "icon", renderer,
          "icon-name", COLUMN_ICON,
          NULL);
      gtk_tree_view_append_column (
        GTK_TREE_VIEW (tree_view),
        column);

      /* column for name */
      renderer =
        gtk_cell_renderer_text_new ();
      column =
        gtk_tree_view_column_new_with_attributes (
          "name", renderer,
          "text", COLUMN_NAME,
          NULL);
      gtk_tree_view_append_column (
        GTK_TREE_VIEW (tree_view),
        column);
    }
  else
    {
      /* column for name */
      renderer =
        gtk_cell_renderer_text_new ();
      column =
        gtk_tree_view_column_new_with_attributes (
          "name", renderer,
          "text", 0,
          NULL);
      gtk_tree_view_append_column (
        GTK_TREE_VIEW (tree_view),
        column);
    }

  /* hide headers and allow multi-selection */
  gtk_tree_view_set_headers_visible (
            GTK_TREE_VIEW (tree_view),
            FALSE);

  if (allow_multi)
    gtk_tree_selection_set_mode (
        gtk_tree_view_get_selection (
          GTK_TREE_VIEW (tree_view)),
        GTK_SELECTION_MULTIPLE);

  if (dnd)
    {
      GtkTargetEntry entries[1];
      entries[0].target = TARGET_ENTRY_PLUGIN_DESCR;
      entries[0].flags = GTK_TARGET_SAME_APP;
      entries[0].info = TARGET_ENTRY_ID_PLUGIN_DESCR;
      gtk_tree_view_enable_model_drag_source (
        GTK_TREE_VIEW (tree_view),
        GDK_BUTTON1_MASK,
        entries,
        1,
        GDK_ACTION_COPY);
      g_signal_connect (
        GTK_WIDGET (tree_view),
        "drag-data-get",
        G_CALLBACK (on_drag_data_get),
        gtk_tree_view_get_selection (
          GTK_TREE_VIEW (tree_view)));
    }

  g_signal_connect (
    G_OBJECT (gtk_tree_view_get_selection (
                GTK_TREE_VIEW (tree_view))),
    "changed",
     G_CALLBACK (on_selection_changed),
     self);

  return tree_view;
}

static GtkWidget *
add_scroll_window (GtkTreeView * tree_view)
{
  /* add treeview to scroll window */
  GtkWidget * scrolled_window =
    gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_window),
                     GTK_WIDGET (tree_view));
  return scrolled_window;
}


/**
 * Creates a GtkTreeView using the given model,
 * and puts it in the given expander in a GtkScrolledWindow
 *
 * @return the scroll window
 * TODO FIXME
 */
static GtkScrolledWindow *
create_tree_view_add_to_expander (FileBrowserWidget * self,
                                  GtkTreeModel * model,
                  GtkExpander  * expander)
{
  GtkWidget * scrolled_window = add_scroll_window
                  (GTK_TREE_VIEW (tree_view_create (self,
                                                    model,
                                                    1,0)));

  /* add scroll window to expander */
  gtk_container_add (
      GTK_CONTAINER (expander),
      scrolled_window);

  /* connect signal to expand/hide */
  g_signal_connect (expander, "notify::expanded",
                  G_CALLBACK (expander_callback),
                  scrolled_window);

  return GTK_SCROLLED_WINDOW (scrolled_window);
}

FileBrowserWidget *
file_browser_widget_new ()
{
  FileBrowserWidget * self = g_object_new (FILE_BROWSER_WIDGET_TYPE, NULL);

  g_message ("Instantiating file_browser widget...");

  gtk_label_set_xalign (self->file_info, 0);

  /* set divider position */
  int divider_pos =
    g_settings_get_int (SETTINGS->root,
                      "browser-divider-position");
  gtk_paned_set_position (GTK_PANED (self),
                          divider_pos);

  /* create each tree */
  create_tree_view_add_to_expander (
    self,
    create_model_for_types (),
    GTK_EXPANDER (self->collections_exp));
  create_tree_view_add_to_expander (
    self,
    create_model_for_types (),
    GTK_EXPANDER (self->types_exp));
  self->locations_tree_model =
    create_model_for_locations ();
  GtkScrolledWindow * scrolled_window =
  create_tree_view_add_to_expander (
    self,
    self->locations_tree_model,
    GTK_EXPANDER (self->locations_exp));

  /* expand locations by default */
  gtk_expander_set_expanded (
    GTK_EXPANDER (self->locations_exp),
    TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (scrolled_window),
                          TRUE);

  /* populate files */
  self->files_tree_model =
    GTK_TREE_MODEL_FILTER (create_model_for_files (self));
  self->files_tree_view =
    GTK_TREE_VIEW (tree_view_create (
      self,
      GTK_TREE_MODEL (
        self->files_tree_model),
      0,
      1));
  g_signal_connect (G_OBJECT (self->files_tree_view),
                    "row-activated",
                    G_CALLBACK (on_row_activated),
                    NULL);
  GtkWidget * file_scroll_window =
    add_scroll_window (self->files_tree_view);
  gtk_box_pack_start (GTK_BOX (self->browser_bot),
                      file_scroll_window,
                      1, 1, 0);

  return self;
}

static void
file_browser_widget_class_init (FileBrowserWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass,
                                "file_browser.ui");

  gtk_widget_class_set_css_name (klass,
                                 "browser");

  gtk_widget_class_bind_template_child (
    klass,
    FileBrowserWidget,
    browser_top);
  gtk_widget_class_bind_template_child (
    klass,
    FileBrowserWidget,
    browser_search);
  gtk_widget_class_bind_template_child (
    klass,
    FileBrowserWidget,
    collections_exp);
  gtk_widget_class_bind_template_child (
    klass,
    FileBrowserWidget,
    types_exp);
  gtk_widget_class_bind_template_child (
    klass,
    FileBrowserWidget,
    locations_exp);
  gtk_widget_class_bind_template_child (
    klass,
    FileBrowserWidget,
    browser_bot);
  gtk_widget_class_bind_template_child (
    klass,
    FileBrowserWidget,
    file_info);
}

static void
file_browser_widget_init (FileBrowserWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
