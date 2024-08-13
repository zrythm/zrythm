// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/tracklist_selections.h"
#include "dsp/region.h"
#include "dsp/track.h"
#include "gui/widgets/dialogs/object_color_chooser_dialog.h"
#include "utils/color.h"
#include "utils/gtk.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

/**
 * Runs the widget and processes the result, then
 * closes the dialog.
 *
 * @param track Track, if track.
 * @param sel TracklistSelections, if multiple
 *   tracks.
 * @param region Region, if region.
 *
 * @return Whether the color was set or not.
 */
bool
object_color_chooser_dialog_widget_run (
  GtkWindow *                       parent,
  Track *                           track,
  const SimpleTracklistSelections * sel,
  Region *                          region)
{
  GtkColorChooserDialog * dialog = NULL;
  if (track)
    {
      auto str = format_str (_ ("{} color"), track->name_);
      dialog = GTK_COLOR_CHOOSER_DIALOG (
        gtk_color_chooser_dialog_new (str.c_str (), parent));
      auto color_rgba = track->color_.to_gdk_rgba ();
      gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (dialog), &color_rgba);
      gtk_color_chooser_set_use_alpha (GTK_COLOR_CHOOSER (dialog), false);
    }
  else if (region)
    {
      auto str = format_str (_ ("%s color"), region->name_);
      dialog = GTK_COLOR_CHOOSER_DIALOG (
        gtk_color_chooser_dialog_new (str.c_str (), parent));
    }
  else if (sel)
    {
      auto tr = sel->get_highest_track ();
      z_return_val_if_fail (tr, false);

      dialog = GTK_COLOR_CHOOSER_DIALOG (
        gtk_color_chooser_dialog_new (_ ("Track color"), parent));
      auto color_rgba = tr->color_.to_gdk_rgba ();
      gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (dialog), &color_rgba);
      gtk_color_chooser_set_use_alpha (GTK_COLOR_CHOOSER (dialog), false);
    }

  z_return_val_if_fail (dialog != nullptr, false);

  gtk_widget_add_css_class (GTK_WIDGET (dialog), "object-color-chooser-dialog");

  int  res = z_gtk_dialog_run (GTK_DIALOG (dialog), false);
  bool color_set = false;
  switch (res)
    {
    case GTK_RESPONSE_OK:
      color_set = true;
      break;
    default:
      break;
    }

  /* get selected color */
  GdkRGBA sel_color;
  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (dialog), &sel_color);

  if (color_set)
    {
      if (track)
        {
          /* get current object color */
          auto cur_color = track->color_;

          /* if changed, apply the change */
          if (!Color (sel_color).is_same (cur_color))
            {
              track->set_color (sel_color, true, true);
            }
        }
      else if (sel)
        {
          try
            {
              UNDO_MANAGER->perform (std::make_unique<EditTracksColorAction> (
                *sel->gen_tracklist_selections (), sel_color));
            }
          catch (const ZrythmException &e)
            {
              e.handle (_ ("Failed to change color"));
            }
        }
    }

  gtk_window_destroy (GTK_WINDOW (dialog));

  return color_set;
}
