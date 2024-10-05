// SPDX-FileCopyrightText: © 2018-2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/audio_region.h"
#include "common/dsp/automation_region.h"
#include "common/dsp/chord_region.h"
#include "common/dsp/midi_region.h"
#include "gui/cpp/gtk_widgets/chord_region.h"

void
chord_region_recreate_pango_layouts (ChordRegion * self)
{
  if (!PANGO_IS_LAYOUT (self->chords_layout_.get ()))
    {
      PangoFontDescription * desc;
      self->chords_layout_ =
        PangoLayoutUniquePtr (gtk_widget_create_pango_layout (
          GTK_WIDGET (self->get_arranger ()), nullptr));
      desc = pango_font_description_from_string (REGION_NAME_FONT_NO_SIZE " 6");
      pango_layout_set_font_description (self->chords_layout_.get (), desc);
      pango_font_description_free (desc);
    }
  pango_layout_get_pixel_size (
    self->chords_layout_.get (), &self->textw_, &self->texth_);
}
