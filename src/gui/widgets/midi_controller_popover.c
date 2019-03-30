/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <string.h>

#include "gui/widgets/midi_controller_mb.h"
#include "gui/widgets/midi_controller_popover.h"
#include "settings/settings.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (MidiControllerPopoverWidget,
               midi_controller_popover_widget,
               GTK_TYPE_POPOVER)

static void
on_closed (MidiControllerPopoverWidget *self,
               gpointer    user_data)
{
  midi_controller_mb_widget_refresh (self->owner);
}

static void
get_controllers (
  char ** controllers,
  int *   num_controllers)
{
  FILE *fp;
  char line[1035];
  char cname[400];
  char dummy[400];

  /* Open the command for reading. */
  fp = popen("amidi -l", "r");
  if (fp == NULL) {
    g_error ("localization_init: popen failed");
    exit(1);
  }

  /* Read the output a line at a time - output it. */
  int i = 0;
  while (fgets(line, sizeof(line)-1, fp) != NULL)
    {
      if (i++ == 0)
        continue;

      sscanf (line, "%s %s %s", dummy, dummy, cname);
      controllers[(*num_controllers)++] =
        g_strdup (cname);
    }

  /* close */
  pclose (fp);
}

/**
 * Finds checkbutton with given label.
 */
GtkWidget *
find_checkbutton (
  MidiControllerPopoverWidget * self,
  const char * label)
{
  GList *children, *iter;
  GtkButton * chkbtn;

  children =
    gtk_container_get_children (
      GTK_CONTAINER (self->controllers_box));
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      if (!GTK_IS_CHECK_BUTTON (iter->data))
        continue;

      chkbtn = GTK_BUTTON (iter->data);
      if (!strcmp (gtk_button_get_label (chkbtn),
                   label))
        {
          g_list_free (children);
          return GTK_WIDGET (chkbtn);
        }
    }

  g_list_free (children);
  return NULL;
}


static void
setup (
  MidiControllerPopoverWidget * self)
{
  char * controllers[60];
  int num_controllers = 0;
  int i;
  GtkWidget * chkbtn;

  /* remove pre-existing controllers */
  z_gtk_container_destroy_all_children (
    GTK_CONTAINER (self->controllers_box));

  /* scan controllers and add them */
  get_controllers (controllers, &num_controllers);
  for (i = 0; i < num_controllers; i++)
    {
      chkbtn =
        gtk_check_button_new_with_label (
          controllers[i]);
      gtk_widget_set_visible (chkbtn, 1);
      gtk_container_add (
        GTK_CONTAINER (self->controllers_box),
        chkbtn);
    }

  /* fetch saved controllers and tick them if they
   * exist */
  gchar ** saved_controllers =
    g_settings_get_strv (
      S_PREFERENCES,
      "midi-controllers");
  char * tmp;
  i = 0;
  while ((tmp = saved_controllers[i]) != NULL)
    {
      /* find checkbutton matching saved controller */
      chkbtn = find_checkbutton (self, tmp);

      if (chkbtn)
        {
          /* tick it */
          gtk_toggle_button_set_active (
            GTK_TOGGLE_BUTTON (chkbtn), 1);
        }

      i++;
    }

  /* cleanup */
  for (i = 0; i < num_controllers; i++)
    g_free (controllers[i]);
}

static void
on_rescan (
  GtkButton * btn,
  MidiControllerPopoverWidget * self)
{
  setup (self);
}

MidiControllerPopoverWidget *
midi_controller_popover_widget_new (
  MidiControllerMbWidget * owner)
{
  MidiControllerPopoverWidget * self =
    g_object_new (
      MIDI_CONTROLLER_POPOVER_WIDGET_TYPE, NULL);

  self->owner = owner;

  setup (self);

  return self;
}

static void
midi_controller_popover_widget_class_init (
  MidiControllerPopoverWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "midi_controller_popover.ui");

  gtk_widget_class_bind_template_child (
    klass,
    MidiControllerPopoverWidget,
    rescan);
  gtk_widget_class_bind_template_child (
    klass,
    MidiControllerPopoverWidget,
    controllers_box);
  gtk_widget_class_bind_template_callback (
    klass,
    on_closed);
}

static void
midi_controller_popover_widget_init (
  MidiControllerPopoverWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  g_signal_connect (
    G_OBJECT (self->rescan), "clicked",
    G_CALLBACK (on_rescan), self);
}
