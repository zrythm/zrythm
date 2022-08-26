// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/widgets/multi_selection.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  MultiSelectionWidget,
  multi_selection_widget,
  GTK_TYPE_WIDGET)

static void
refresh (MultiSelectionWidget * self)
{
  for (guint i = 0; i < self->item_strings->len; i++)
    {
      char * str =
        g_array_index (self->item_strings, char *, i);
      GtkToggleButton * toggle = GTK_TOGGLE_BUTTON (
        gtk_toggle_button_new_with_label (str));
      gtk_flow_box_append (
        self->flow_box, GTK_WIDGET (toggle));
    }
}

void
multi_selection_widget_setup (
  MultiSelectionWidget *        self,
  const char **                 strings,
  const cyaml_strval_t *        cyaml_strings,
  const int                     num_items,
  MultiSelectionChangedCallback sel_changed_cb,
  const int *                   selections,
  const int                     num_selections,
  void *                        object)
{
  g_return_if_fail (strings || cyaml_strings);

  if (strings)
    {
      /* TODO */
      g_return_if_reached ();
    }
  else
    {
      for (int i = 0; i < num_items; i++)
        {
          char * str = g_strdup (_ (cyaml_strings[i].str));
          g_array_append_val (self->item_strings, str);
        }
    }

  refresh (self);
}

static void
clear_item_string (gpointer data)
{
  char * str = *((char **) data);
  g_free (str);
}

MultiSelectionWidget *
multi_selection_widget_new (void)
{
  MultiSelectionWidget * self =
    g_object_new (MULTI_SELECTION_WIDGET_TYPE, NULL);

  return self;
}

static void
multi_selection_widget_class_init (
  MultiSelectionWidgetClass * klass)
{
  gtk_widget_class_set_layout_manager_type (
    GTK_WIDGET_CLASS (klass), GTK_TYPE_BIN_LAYOUT);
}

static void
multi_selection_widget_init (MultiSelectionWidget * self)
{
  self->item_strings =
    g_array_new (true, true, sizeof (char *));
  g_array_set_clear_func (
    self->item_strings, clear_item_string);
  self->selections = g_array_new (true, true, sizeof (int));

  self->flow_box = GTK_FLOW_BOX (gtk_flow_box_new ());
  gtk_flow_box_set_min_children_per_line (self->flow_box, 3);
  gtk_widget_set_parent (
    GTK_WIDGET (self->flow_box), GTK_WIDGET (self));
}
