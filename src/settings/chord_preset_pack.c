// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "io/serialization/chords.h"
#include "project.h"
#include "settings/chord_preset_pack.h"
#include "utils/arrays.h"
#include "utils/gtk.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#define CHORD_PRESET_PACK_JSON_TYPE "Zrythm Chord Preset Pack"
#define CHORD_PRESET_PACK_JSON_FORMAT 2

ChordPresetPack *
chord_preset_pack_new_empty (void)
{
  ChordPresetPack * self = object_new (ChordPresetPack);
  self->presets = g_ptr_array_new_full (12, (GDestroyNotify) chord_preset_free);
  return self;
}

ChordPresetPack *
chord_preset_pack_new (const char * name, bool is_standard)
{
  ChordPresetPack * self = chord_preset_pack_new_empty ();

  self->name = g_strdup (name);
  self->is_standard = is_standard;

  return self;
}

/**
 * @note The given preset is cloned so the caller is
 *   still responsible for @ref pset.
 */
void
chord_preset_pack_add_preset (ChordPresetPack * self, const ChordPreset * pset)
{
  ChordPreset * clone = chord_preset_clone (pset);
  clone->pack = self;

  g_ptr_array_add (self->presets, clone);
}

bool
chord_preset_pack_contains_name (const ChordPresetPack * self, const char * name)
{
  for (size_t i = 0; i < self->presets->len; i++)
    {
      ChordPreset * pset = g_ptr_array_index (self->presets, i);
      if (string_is_equal_ignore_case (pset->name, name))
        return true;
    }

  return false;
}

bool
chord_preset_pack_contains_preset (
  const ChordPresetPack * self,
  const ChordPreset *     pset)
{
  for (size_t i = 0; i < self->presets->len; i++)
    {
      ChordPreset * cur_pset = g_ptr_array_index (self->presets, i);
      if (pset == cur_pset)
        return true;
    }

  return false;
}

void
chord_preset_pack_delete_preset (ChordPresetPack * self, ChordPreset * pset)
{
  g_ptr_array_remove (self->presets, pset);

  EVENTS_PUSH (ET_CHORD_PRESET_REMOVED, NULL);
}

const char *
chord_preset_pack_get_name (const ChordPresetPack * self)
{
  return self->name;
}

void
chord_preset_pack_set_name (ChordPresetPack * self, const char * name)
{
  object_free_w_func_and_null (g_free, self->name);

  self->name = g_strdup (name);

  EVENTS_PUSH (ET_CHORD_PRESET_PACK_EDITED, NULL);
}

GMenuModel *
chord_preset_pack_generate_context_menu (const ChordPresetPack * self)
{
  if (self->is_standard)
    return NULL;

  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;
  char        action[800];

  /* rename */
  sprintf (action, "app.rename-chord-preset-pack::%p", self);
  menuitem = z_gtk_create_menu_item (_ ("_Rename"), "edit-rename", action);
  g_menu_append_item (menu, menuitem);

  /* delete */
  sprintf (action, "app.delete-chord-preset-pack::%p", self);
  menuitem = z_gtk_create_menu_item (_ ("_Delete"), "edit-delete", action);
  g_menu_append_item (menu, menuitem);

  return G_MENU_MODEL (menu);
}

char *
chord_preset_pack_serialize_to_json_str (
  const ChordPresetPack * self,
  GError **               error)
{
  yyjson_mut_doc * doc = yyjson_mut_doc_new (NULL);
  yyjson_mut_val * root = yyjson_mut_obj (doc);
  if (!root)
    {
      g_set_error_literal (error, 0, 0, "Failed to create root obj");
      return NULL;
    }
  yyjson_mut_doc_set_root (doc, root);

  yyjson_mut_obj_add_str (doc, root, "type", CHORD_PRESET_PACK_JSON_TYPE);
  yyjson_mut_obj_add_int (doc, root, "format", CHORD_PRESET_PACK_JSON_FORMAT);
  yyjson_mut_obj_add_str (doc, root, "name", self->name);
  yyjson_mut_val * psets_arr = yyjson_mut_obj_add_arr (doc, root, "presets");
  for (size_t i = 0; i < self->presets->len; i++)
    {
      const ChordPreset * pset = g_ptr_array_index (self->presets, i);
      yyjson_mut_val *    pset_obj = yyjson_mut_arr_add_obj (doc, psets_arr);
      chord_preset_serialize_to_json (doc, pset_obj, pset, error);
    }
  yyjson_mut_obj_add_bool (doc, root, "isStandard", self->is_standard);

  char * json = yyjson_mut_write (doc, YYJSON_WRITE_PRETTY_TWO_SPACES, NULL);
  g_message ("done writing json to string");

  yyjson_mut_doc_free (doc);

  return json;
}

ChordPresetPack *
chord_preset_pack_deserialize_from_json_str (const char * json, GError ** error)
{
  yyjson_doc * doc =
    yyjson_read_opts ((char *) json, strlen (json), 0, NULL, NULL);
  yyjson_val * root = yyjson_doc_get_root (doc);
  if (!root)
    {
      g_set_error_literal (error, 0, 0, "Failed to create root obj");
      return NULL;
    }

  yyjson_obj_iter it = yyjson_obj_iter_with (root);
  if (!yyjson_equals_str (
        yyjson_obj_iter_get (&it, "type"), CHORD_PRESET_PACK_JSON_TYPE))
    {
      g_set_error_literal (error, 0, 0, "Not a valid preset pack file");
      return NULL;
    }

  int format = yyjson_get_int (yyjson_obj_iter_get (&it, "format"));
  if (format != CHORD_PRESET_PACK_JSON_FORMAT)
    {
      g_set_error_literal (error, 0, 0, "Invalid version");
      return NULL;
    }

  ChordPresetPack * self = chord_preset_pack_new_empty ();

  self->name = g_strdup (yyjson_get_str (yyjson_obj_iter_get (&it, "name")));
  yyjson_val *    presets_arr = yyjson_obj_iter_get (&it, "presets");
  yyjson_arr_iter presets_it = yyjson_arr_iter_with (presets_arr);
  yyjson_val *    preset_obj = NULL;
  while ((preset_obj = yyjson_arr_iter_next (&presets_it)))
    {
      ChordPreset * preset = chord_preset_new ();
      g_ptr_array_add (self->presets, preset);
      chord_preset_deserialize_from_json (doc, preset_obj, preset, error);
    }
  self->is_standard = yyjson_get_bool (yyjson_obj_iter_get (&it, "isStandard"));

  yyjson_doc_free (doc);

  return self;
}

ChordPresetPack *
chord_preset_pack_clone (const ChordPresetPack * src)
{
  ChordPresetPack * self = chord_preset_pack_new (src->name, src->is_standard);

  for (size_t i = 0; i < src->presets->len; i++)
    {
      const ChordPreset * src_pset = g_ptr_array_index (src->presets, i);
      ChordPreset *       new_pset = chord_preset_clone (src_pset);
      new_pset->pack = self;
      g_ptr_array_add (self->presets, new_pset);
    }

  return self;
}

void
chord_preset_pack_free (ChordPresetPack * self)
{
  object_free_w_func_and_null (g_free, self->name);
  object_free_w_func_and_null (g_ptr_array_unref, self->presets);

  object_zero_and_free (self);
}

void
chord_preset_pack_destroy_cb (void * self)
{
  chord_preset_pack_free ((ChordPresetPack *) self);
}
