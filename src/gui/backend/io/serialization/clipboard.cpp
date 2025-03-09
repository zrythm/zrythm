// SPDX-FileCopyrightText: © 2023-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/audio_selections.h"
#include "gui/backend/backend/automation_selections.h"
#include "gui/backend/backend/chord_selections.h"
#include "gui/backend/backend/clipboard.h"
#include "gui/backend/backend/midi_selections.h"
#include "gui/backend/backend/timeline_selections.h"

void
Clipboard::define_fields (const Context &ctx)
{
  using T = ISerializable<Clipboard>;
  T::serialize_fields (ctx, T::make_field ("type", type_));

  if (ctx.is_serializing ())
    {
      T::serialize_field<decltype (arranger_sel_), ArrangerSelectionsPtrVariant> (
        "arrangerSelections", arranger_sel_, ctx, true);
    }
  else
    {
      auto         obj_it = yyjson_obj_iter_with (ctx.obj_);
      yyjson_val * arranger_sel =
        yyjson_obj_iter_get (&obj_it, "arrangerSelections");
      if (arranger_sel)
        {
          auto arranger_sel_it = yyjson_obj_iter_with (arranger_sel);
          auto type =
            yyjson_get_int (yyjson_obj_iter_get (&arranger_sel_it, "type"));
          try
            {
              auto sel_type = ENUM_INT_TO_VALUE (ArrangerSelections::Type, type);
              auto sel = ArrangerSelections::new_from_type (sel_type);
              std::visit (
                [&] (auto &&s) {
                  using ArrangerSelectionsT = base_type<decltype (s)>;
                  s->ISerializable<ArrangerSelectionsT>::deserialize (
                    Context (arranger_sel, ctx));
                },
                convert_to_variant<ArrangerSelectionsPtrVariant> (sel.get ()));
              arranger_sel_ = std::move (sel);
            }
          catch (const std::runtime_error &e)
            {
              throw ZrythmException (
                "Invalid type id: " + std::to_string (type));
            }
        }
    }

  T::serialize_fields (
    ctx, T::make_field ("plugins", plugins_, true),
    T::make_field ("tracklistSelections", tracks_, true));
}
