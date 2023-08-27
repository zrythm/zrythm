// SPDX-FileCopyrightText: © 2018-2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_bus_track.h"
#include "dsp/channel.h"
#include "dsp/chord_region.h"
#include "dsp/instrument_track.h"
#include "dsp/midi_note.h"
#include "dsp/track.h"
#include "gui/backend/chord_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_region.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "utils/cairo.h"

/**
 * Recreates the pango layout for drawing chord
 * names inside the region.
 */
void
chord_region_recreate_pango_layouts (ZRegion * self)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  if (!PANGO_IS_LAYOUT (self->chords_layout))
    {
      PangoFontDescription * desc;
      self->chords_layout = gtk_widget_create_pango_layout (
        GTK_WIDGET (arranger_object_get_arranger (obj)), NULL);
      desc = pango_font_description_from_string (
        REGION_NAME_FONT_NO_SIZE " 6");
      pango_layout_set_font_description (
        self->chords_layout, desc);
      pango_font_description_free (desc);
    }
  pango_layout_get_pixel_size (
    self->chords_layout, &obj->textw, &obj->texth);
}
