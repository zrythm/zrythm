/*
 * gui/widgets/preferences.c - Preferences window
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#include "audio/engine.h"
#include "gui/widgets/preferences.h"
#include "settings/preferences.h"
#include "settings/settings.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (PreferencesWidget,
               preferences_widget,
               GTK_TYPE_DIALOG)



/*static void*/
/*append_category (PreferencesWidget * self,*/
                 /*const char *        category)*/
/*{*/
  /*GtkWidget * category_label =*/
    /*gtk_label_new (category);*/
  /*gtk_widget_set_visible (category_label,*/
                          /*1);*/

  /*GtkFlowBox * flowbox =*/
    /*GTK_FLOW_BOX (gtk_flow_box_new ());*/
  /*gtk_widget_set_visible (GTK_WIDGET (flowbox),*/
                          /*1);*/
  /*gtk_orientable_set_orientation (*/
    /*GTK_ORIENTABLE (flowbox),*/
    /*GTK_ORIENTATION_VERTICAL);*/

  /*GValue value = G_VALUE_INIT;*/
  /*g_value_init (&value,*/
                /*G_TYPE_SETTINGS_SCHEMA);*/
  /*g_object_get_property (*/
    /*G_OBJECT (self->settings->org_zrythm_preferences_audio),*/
    /*"settings-schema",*/
    /*&value);*/
  /*g_assert (G_VALUE_HOLDS_BOXED (&value));*/
  /*GSettingsSchema * schema =*/
    /*(GSettingsSchema *) (g_value_get_boxed (&value));*/
  /*gchar ** keys = g_settings_schema_list_keys (schema);*/
  /*int i = 0;*/
  /*while (keys[i] != NULL)*/
    /*{*/
      /*g_message ("%s", keys[i]);*/
      /*GSettingsSchemaKey * key =*/
        /*g_settings_schema_get_key (schema,*/
                                   /*keys[i]);*/
      /*GVariantType * variant_type =*/
        /*g_settings_schema_key_get_value_type (key);*/
      /*i++;*/
    /*}*/

  /*gtk_notebook_append_page (self->categories,*/
                            /*GTK_WIDGET (flowbox),*/
                            /*category_label);*/
/*}*/

enum
{
  VALUE_COL,
  TEXT_COL
};

static GtkTreeModel *
create_audio_backend_model (void)
{
  const int values[NUM_ENGINE_BACKENDS] = {
    ENGINE_BACKEND_JACK,
    ENGINE_BACKEND_PORT_AUDIO,
  };
  const gchar *labels[NUM_ENGINE_BACKENDS] = {
    "Jack",
    "Port Audio",
  };

  GtkTreeIter iter;
  GtkListStore *store;
  gint i;

  store = gtk_list_store_new (NUM_ENGINE_BACKENDS,
                              G_TYPE_INT,
                              G_TYPE_STRING);

  for (i = 0; i < G_N_ELEMENTS (values); i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          VALUE_COL, values[i],
                          TEXT_COL, labels[i],
                          -1);
    }

  return GTK_TREE_MODEL (store);
}

static void
setup_audio (PreferencesWidget * self)
{
  GtkCellRenderer *renderer;
  gtk_combo_box_set_model (self->audio_backend,
                           create_audio_backend_model ());
  gtk_cell_layout_clear (GTK_CELL_LAYOUT (self->audio_backend));
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (self->audio_backend), renderer, TRUE);
  gtk_cell_layout_set_attributes (
    GTK_CELL_LAYOUT (self->audio_backend), renderer,
    "text", TEXT_COL,
    NULL);

  gtk_combo_box_set_active (
    GTK_COMBO_BOX (self->audio_backend),
    self->preferences->audio_backend);
}


static void
on_cancel_clicked (GtkWidget * widget,
          gpointer    user_data)
{
  PreferencesWidget * self =
    Z_PREFERENCES_WIDGET (user_data);

  /* TODO confirmation */

  gtk_window_close (GTK_WINDOW (self));
}

static void
on_ok_clicked (GtkWidget * widget,
                gpointer    user_data)
{
  PreferencesWidget * self =
    Z_PREFERENCES_WIDGET (user_data);

  preferences_set_audio_backend (
    self->preferences,
    gtk_combo_box_get_active (self->audio_backend));

  gtk_window_close (GTK_WINDOW (self));
}

/**
 * Sets up the preferences widget.
 */
PreferencesWidget *
preferences_widget_new (Preferences * preferences)
{
  PreferencesWidget * self =
    g_object_new (PREFERENCES_WIDGET_TYPE,
                  NULL);

  self->preferences = preferences;

  setup_audio (self);

  return self;
}

static void
preferences_widget_class_init (
  PreferencesWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass,
                                "preferences.ui");

  gtk_widget_class_bind_template_child (
    klass,
    PreferencesWidget,
    categories);
  gtk_widget_class_bind_template_child (
    klass,
    PreferencesWidget,
    ok);
  gtk_widget_class_bind_template_child (
    klass,
    PreferencesWidget,
    audio_backend);
  gtk_widget_class_bind_template_callback (
    klass,
    on_ok_clicked);
  gtk_widget_class_bind_template_callback (
    klass,
    on_cancel_clicked);
}

static void
preferences_widget_init (PreferencesWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
