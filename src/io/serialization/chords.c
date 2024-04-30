// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_descriptor.h"
#include "io/serialization/chords.h"
#include "io/serialization/extra.h"
#include "settings/chord_preset.h"
#include "utils/objects.h"

bool
chord_descriptor_serialize_to_json (
  yyjson_mut_doc *        doc,
  yyjson_mut_val *        descr_obj,
  const ChordDescriptor * descr,
  GError **               error)
{
  yyjson_mut_obj_add_bool (doc, descr_obj, "hasBass", descr->has_bass);
  yyjson_mut_obj_add_int (doc, descr_obj, "rootNote", descr->root_note);
  yyjson_mut_obj_add_int (doc, descr_obj, "bassNote", descr->bass_note);
  yyjson_mut_obj_add_int (doc, descr_obj, "type", descr->type);
  yyjson_mut_obj_add_int (doc, descr_obj, "accent", descr->accent);
  yyjson_mut_val * notes_arr = yyjson_mut_obj_add_arr (doc, descr_obj, "notes");
  for (int i = 0; i < CHORD_DESCRIPTOR_MAX_NOTES; i++)
    {
      yyjson_mut_arr_add_bool (doc, notes_arr, descr->notes[i]);
    }
  yyjson_mut_obj_add_int (doc, descr_obj, "inversion", descr->inversion);
  return true;
}

bool
chord_preset_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    pset_obj,
  const ChordPreset * pset,
  GError **           error)
{
  yyjson_mut_obj_add_str (doc, pset_obj, "name", pset->name);
  yyjson_mut_val * descriptors_arr =
    yyjson_mut_obj_add_arr (doc, pset_obj, "descriptors");
  for (int i = 0; i < 12; i++)
    {
      ChordDescriptor * descr = pset->descr[i];
      if (descr)
        {
          yyjson_mut_val * descr_obj =
            yyjson_mut_arr_add_obj (doc, descriptors_arr);
          chord_descriptor_serialize_to_json (doc, descr_obj, descr, error);
        }
      else
        {
          yyjson_mut_arr_add_null (doc, descriptors_arr);
        }
    }
  return true;
}

bool
chord_descriptor_deserialize_from_json (
  yyjson_doc *      doc,
  yyjson_val *      descr_obj,
  ChordDescriptor * descr,
  GError **         error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (descr_obj);
  descr->has_bass = yyjson_get_bool (yyjson_obj_iter_get (&it, "hasBass"));
  descr->root_note =
    (MusicalNote) yyjson_get_int (yyjson_obj_iter_get (&it, "rootNote"));
  descr->bass_note =
    (MusicalNote) yyjson_get_int (yyjson_obj_iter_get (&it, "bassNote"));
  descr->type = (ChordType) yyjson_get_int (yyjson_obj_iter_get (&it, "type"));
  descr->accent =
    (ChordAccent) yyjson_get_int (yyjson_obj_iter_get (&it, "accent"));

  yyjson_val *    notes_arr = yyjson_obj_iter_get (&it, "notes");
  yyjson_arr_iter notes_it = yyjson_arr_iter_with (notes_arr);
  yyjson_val *    note_obj = NULL;
  size_t          count = 0;
  while ((note_obj = yyjson_arr_iter_next (&notes_it)))
    {
      descr->notes[count++] = yyjson_get_bool (note_obj);
    }
  descr->inversion = yyjson_get_int (yyjson_obj_iter_get (&it, "inversion"));
  return true;
}

bool
chord_preset_deserialize_from_json (
  yyjson_doc *  doc,
  yyjson_val *  pset_obj,
  ChordPreset * pset,
  GError **     error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (pset_obj);
  pset->name = g_strdup (yyjson_get_str (yyjson_obj_iter_get (&it, "name")));
  yyjson_val * descriptors_arr = yyjson_obj_iter_get (&it, "descriptors");
  size_t       arr_size = yyjson_arr_size (descriptors_arr);
  if (arr_size > 0)
    {
      yyjson_arr_iter descr_it = yyjson_arr_iter_with (descriptors_arr);
      yyjson_val *    descr_obj = NULL;
      int             count = 0;
      while ((descr_obj = yyjson_arr_iter_next (&descr_it)))
        {
          ChordDescriptor * descr = object_new (ChordDescriptor);
          pset->descr[count++] = descr;
          chord_descriptor_deserialize_from_json (doc, descr_obj, descr, error);
        }
    }
  return true;
}
