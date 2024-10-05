// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/gtk_widgets/multi_selection.h"
#include "utils/logger.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

G_DEFINE_TYPE (MultiSelectionWidget, multi_selection_widget, GTK_TYPE_WIDGET)

static void
on_toggle_toggled (GtkToggleButton * toggle, MultiSelectionWidget * self)
{
  bool active = gtk_toggle_button_get_active (toggle);

  guint idx = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (toggle), "index"));

  z_debug ("track filter toggle toggled at index {}: active {}", idx, active);
  if (active)
    {
      /* return if already selected */
      for (guint i = 0; i < self->selections->len; i++)
        {
          guint cur_idx = g_array_index (self->selections, guint, i);
          if (cur_idx == idx)
            return;
        }

      /* otherwise add to selections */
      g_array_append_val (self->selections, idx);
    }
  else
    {
      bool  found = false;
      guint found_idx;
      for (guint i = 0; i < self->selections->len; i++)
        {
          guint cur_idx = i;
          guint cur_selection = g_array_index (self->selections, guint, i);
          if (cur_selection == idx)
            {
              found_idx = cur_idx;
              found = true;
            }
        }

      /* return if already deselected */
      if (found == false)
        return;

      /* otherwise remove from selections */
      g_array_remove_index (self->selections, found_idx);
    }

  if (self->sel_changed_cb)
    {
      self->sel_changed_cb (self, self->selections, self->obj);
    }
}

static void
refresh (MultiSelectionWidget * self)
{
  for (guint i = 0; i < self->item_strings->len; i++)
    {
      char *            str = g_array_index (self->item_strings, char *, i);
      GtkToggleButton * toggle =
        GTK_TOGGLE_BUTTON (gtk_toggle_button_new_with_label (str));
      g_object_set_data (G_OBJECT (toggle), "index", GUINT_TO_POINTER (i));

      for (guint j = 0; j < self->selections->len; j++)
        {
          guint cur_selection = g_array_index (self->selections, guint, j);
          if (cur_selection == i)
            {
              gtk_toggle_button_set_active (toggle, true);
            }
        }

      gtk_flow_box_append (self->flow_box, GTK_WIDGET (toggle));
      g_signal_connect (toggle, "toggled", G_CALLBACK (on_toggle_toggled), self);
    }
}

void
multi_selection_widget_setup (
  MultiSelectionWidget *        self,
  const char **                 strings,
  const int                     num_items,
  MultiSelectionChangedCallback sel_changed_cb,
  const guint *                 selections,
  const int                     num_selections,
  void *                        object)
{
  z_return_if_fail (strings);

  for (int i = 0; i < num_items; i++)
    {
      char * str = g_strdup (_ (strings[i]));
      g_array_append_val (self->item_strings, str);
    }

  if (selections)
    {
      for (int i = 0; i < num_selections; i++)
        {
          g_array_append_val (self->selections, selections[i]);
        }
    }

  if (sel_changed_cb)
    {
      self->sel_changed_cb = sel_changed_cb;
      self->obj = object;
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
  MultiSelectionWidget * self = static_cast<MultiSelectionWidget *> (
    g_object_new (MULTI_SELECTION_WIDGET_TYPE, nullptr));

  return self;
}

static void
dispose (MultiSelectionWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->flow_box));

  G_OBJECT_CLASS (multi_selection_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
finalize (MultiSelectionWidget * self)
{
  g_array_unref (self->item_strings);
  g_array_unref (self->selections);

  G_OBJECT_CLASS (multi_selection_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
multi_selection_widget_class_init (MultiSelectionWidgetClass * klass)
{
  gtk_widget_class_set_layout_manager_type (
    GTK_WIDGET_CLASS (klass), GTK_TYPE_BIN_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}

static void
multi_selection_widget_init (MultiSelectionWidget * self)
{
  self->item_strings = g_array_new (true, true, sizeof (char *));
  g_array_set_clear_func (self->item_strings, clear_item_string);
  self->selections = g_array_new (true, true, sizeof (guint));

  self->flow_box = GTK_FLOW_BOX (gtk_flow_box_new ());
  gtk_flow_box_set_min_children_per_line (self->flow_box, 3);
  gtk_widget_set_parent (GTK_WIDGET (self->flow_box), GTK_WIDGET (self));
}
