// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_descriptor.h"
#include "dsp/curve.h"
#include "dsp/position.h"
#include "io/serialization/extra.h"
#include "utils/objects.h"
#include "utils/stack.h"

typedef enum
{
  Z_IO_SERIALIZATION_EXTRA_ERROR_FAILED,
} ZIOSerializationExtraError;

#define Z_IO_SERIALIZATION_EXTRA_ERROR z_io_serialization_extra_error_quark ()
GQuark
z_io_serialization_extra_error_quark (void);
G_DEFINE_QUARK (z - io - serialization - extra - error - quark, z_io_serialization_extra_error)

bool
gdk_rgba_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * rgba_obj,
  const GdkRGBA *  rgba,
  GError **        error)
{
  yyjson_mut_obj_add_real (doc, rgba_obj, "red", rgba->red);
  yyjson_mut_obj_add_real (doc, rgba_obj, "green", rgba->green);
  yyjson_mut_obj_add_real (doc, rgba_obj, "blue", rgba->blue);
  yyjson_mut_obj_add_real (doc, rgba_obj, "alpha", rgba->alpha);
  return true;
}

bool
position_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * pos_obj,
  const Position * pos,
  GError **        error)
{
  yyjson_mut_obj_add_real (doc, pos_obj, "ticks", pos->ticks);
  yyjson_mut_obj_add_int (doc, pos_obj, "frames", pos->frames);
  return true;
}

bool
curve_options_serialize_to_json (
  yyjson_mut_doc *     doc,
  yyjson_mut_val *     opts_obj,
  const CurveOptions * opts,
  GError **            error)
{
  yyjson_mut_obj_add_int (doc, opts_obj, "algorithm", opts->algo);
  yyjson_mut_obj_add_real (doc, opts_obj, "curviness", opts->curviness);
  return true;
}

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
stack_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * stack_obj,
  const Stack *    stack,
  GError **        error)
{
  yyjson_mut_obj_add_int (doc, stack_obj, "maxLength", stack->max_length);
  return true;
}

bool
gdk_rgba_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * rgba_obj,
  GdkRGBA *    rgba,
  GError **    error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (rgba_obj);
  rgba->red = (float) yyjson_get_real (yyjson_obj_iter_get (&it, "red"));
  rgba->green = (float) yyjson_get_real (yyjson_obj_iter_get (&it, "green"));
  rgba->blue = (float) yyjson_get_real (yyjson_obj_iter_get (&it, "blue"));
  rgba->alpha = (float) yyjson_get_real (yyjson_obj_iter_get (&it, "alpha"));
  return true;
}

bool
position_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * pos_obj,
  Position *   pos,
  GError **    error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (pos_obj);
  pos->ticks = yyjson_get_real (yyjson_obj_iter_get (&it, "ticks"));
  pos->frames = yyjson_get_int (yyjson_obj_iter_get (&it, "frames"));
  return true;
}

bool
curve_options_deserialize_from_json (
  yyjson_doc *   doc,
  yyjson_val *   opts_obj,
  CurveOptions * opts,
  GError **      error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (opts_obj);
  opts->algo =
    (CurveAlgorithm) yyjson_get_int (yyjson_obj_iter_get (&it, "algorithm"));
  opts->curviness = yyjson_get_real (yyjson_obj_iter_get (&it, "curviness"));
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
stack_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * stack_obj,
  Stack *      stack,
  GError **    error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (stack_obj);
  stack->max_length = yyjson_get_int (yyjson_obj_iter_get (&it, "maxLength"));
  return true;
}
