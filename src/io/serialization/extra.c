// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_descriptor.h"
#include "dsp/curve.h"
#include "dsp/position.h"
#include "io/serialization/extra.h"
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
  yyjson_mut_obj_add_real (doc, descr_obj, "hasBass", descr->has_bass);
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
