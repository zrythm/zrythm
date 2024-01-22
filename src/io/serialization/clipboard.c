// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/clipboard.h"
#include "io/serialization/clipboard.h"
#include "io/serialization/selections.h"
#include "utils/compression.h"
#include "utils/error.h"
#include "utils/objects.h"

#include <yyjson.h>

typedef enum
{
  Z_IO_SERIALIZATION_CLIPBOARD_ERROR_FAILED,
} ZIOSerializationClipboardError;

#define Z_IO_SERIALIZATION_CLIPBOARD_ERROR \
  z_io_serialization_clipboard_error_quark ()
GQuark
z_io_serialization_clipboard_error_quark (void);
G_DEFINE_QUARK (z - io - serialization - clipboard - error - quark, z_io_serialization_clipboard_error)

char *
clipboard_serialize_to_json_str (
  const Clipboard * clipboard,
  bool              compress,
  GError **         error)
{
  /* create a mutable doc */
  yyjson_mut_doc * doc = yyjson_mut_doc_new (NULL);
  yyjson_mut_val * root = yyjson_mut_obj (doc);
  if (!root)
    {
      g_set_error_literal (
        error, Z_IO_SERIALIZATION_CLIPBOARD_ERROR,
        Z_IO_SERIALIZATION_CLIPBOARD_ERROR_FAILED, "Failed to create root obj");
      return NULL;
    }
  yyjson_mut_doc_set_root (doc, root);

  yyjson_mut_obj_add_str (doc, root, "type", "ZrythmClipboard");
  yyjson_mut_obj_add_int (doc, root, "format", 1);
  yyjson_mut_obj_add_int (doc, root, "clipboardType", clipboard->type);
  if (clipboard->timeline_sel)
    {
      yyjson_mut_val * timeline_sel_obj =
        yyjson_mut_obj_add_obj (doc, root, "timelineSelections");
      timeline_selections_serialize_to_json (
        doc, timeline_sel_obj, clipboard->timeline_sel, error);
    }
  if (clipboard->ma_sel)
    {
      yyjson_mut_val * ma_sel_obj =
        yyjson_mut_obj_add_obj (doc, root, "midiArrangerSelections");
      midi_arranger_selections_serialize_to_json (
        doc, ma_sel_obj, clipboard->ma_sel, error);
    }
  if (clipboard->chord_sel)
    {
      yyjson_mut_val * chord_sel_obj =
        yyjson_mut_obj_add_obj (doc, root, "chordSelections");
      chord_selections_serialize_to_json (
        doc, chord_sel_obj, clipboard->chord_sel, error);
    }
  if (clipboard->automation_sel)
    {
      yyjson_mut_val * automation_sel_obj =
        yyjson_mut_obj_add_obj (doc, root, "automationSelections");
      automation_selections_serialize_to_json (
        doc, automation_sel_obj, clipboard->automation_sel, error);
    }
  if (clipboard->audio_sel)
    {
      yyjson_mut_val * audio_sel_obj =
        yyjson_mut_obj_add_obj (doc, root, "audioSelections");
      audio_selections_serialize_to_json (
        doc, audio_sel_obj, clipboard->audio_sel, error);
    }
  if (clipboard->mixer_sel)
    {
      yyjson_mut_val * mixer_sel_obj =
        yyjson_mut_obj_add_obj (doc, root, "mixerSelections");
      mixer_selections_serialize_to_json (
        doc, mixer_sel_obj, clipboard->mixer_sel, error);
    }
  if (clipboard->tracklist_sel)
    {
      yyjson_mut_val * tracklist_sel_obj =
        yyjson_mut_obj_add_obj (doc, root, "tracklistSelections");
      tracklist_selections_serialize_to_json (
        doc, tracklist_sel_obj, clipboard->tracklist_sel, error);
    }

  /* to string - minified */
  char * json = yyjson_mut_write (doc, YYJSON_WRITE_NOFLAG, NULL);

  yyjson_mut_doc_free (doc);

  if (compress)
    {
      char *   tmp = json;
      GError * err = NULL;
      json = compression_compress_to_base64_str (json, &err);
      if (!json)
        {
          PROPAGATE_PREFIXED_ERROR_LITERAL (
            error, err, "Failed to compress clipboard");
          return NULL;
        }
      free (tmp);
    }

  return json;
}

Clipboard *
clipboard_deserialize_from_json_str (
  const char * json,
  bool         decompress,
  GError **    error)
{
  char * tmp = NULL;
  if (decompress)
    {
      GError * err = NULL;
      tmp = compression_decompress_from_base64_str (json, &err);
      json = tmp;
      if (!json)
        {
          PROPAGATE_PREFIXED_ERROR_LITERAL (
            error, err, "Failed to decompress clipboard");
          return NULL;
        }
    }

  yyjson_doc * doc =
    yyjson_read_opts ((char *) json, strlen (json), 0, NULL, NULL);
  yyjson_val * root = yyjson_doc_get_root (doc);
  if (!root)
    {
      g_set_error_literal (
        error, Z_IO_SERIALIZATION_CLIPBOARD_ERROR,
        Z_IO_SERIALIZATION_CLIPBOARD_ERROR_FAILED, "Failed to create root obj");
      return NULL;
    }

  Clipboard *     self = object_new (Clipboard);
  yyjson_obj_iter it = yyjson_obj_iter_with (root);
  self->type =
    (ClipboardType) yyjson_get_int (yyjson_obj_iter_get (&it, "clipboardType"));
  yyjson_val * timeline_sel_obj =
    yyjson_obj_iter_get (&it, "timelineSelections");
  if (timeline_sel_obj)
    {
      self->timeline_sel = object_new (TimelineSelections);
      timeline_selections_deserialize_from_json (
        doc, timeline_sel_obj, self->timeline_sel, error);
    }
  yyjson_val * ma_sel_obj = yyjson_obj_iter_get (&it, "midiArrangerSelections");
  if (ma_sel_obj)
    {
      self->ma_sel = object_new (MidiArrangerSelections);
      midi_arranger_selections_deserialize_from_json (
        doc, ma_sel_obj, self->ma_sel, error);
    }
  yyjson_val * chord_sel_obj = yyjson_obj_iter_get (&it, "chordSelections");
  if (chord_sel_obj)
    {
      self->chord_sel = object_new (ChordSelections);
      chord_selections_deserialize_from_json (
        doc, chord_sel_obj, self->chord_sel, error);
    }
  yyjson_val * automation_sel_obj =
    yyjson_obj_iter_get (&it, "automationSelections");
  if (automation_sel_obj)
    {
      self->automation_sel = object_new (AutomationSelections);
      automation_selections_deserialize_from_json (
        doc, automation_sel_obj, self->automation_sel, error);
    }
  yyjson_val * audio_sel_obj = yyjson_obj_iter_get (&it, "audioSelections");
  if (audio_sel_obj)
    {
      self->audio_sel = object_new (AudioSelections);
      audio_selections_deserialize_from_json (
        doc, audio_sel_obj, self->audio_sel, error);
    }
  yyjson_val * mixer_sel_obj = yyjson_obj_iter_get (&it, "mixerSelections");
  if (mixer_sel_obj)
    {
      self->mixer_sel = object_new (MixerSelections);
      mixer_selections_deserialize_from_json (
        doc, mixer_sel_obj, self->mixer_sel, error);
    }
  yyjson_val * tracklist_sel_obj =
    yyjson_obj_iter_get (&it, "tracklistSelections");
  if (tracklist_sel_obj)
    {
      self->tracklist_sel = object_new (TracklistSelections);
      tracklist_selections_deserialize_from_json (
        doc, tracklist_sel_obj, self->tracklist_sel, error);
    }

  yyjson_doc_free (doc);

  if (tmp)
    {
      free (tmp);
    }

  return self;
}
