/* robTK
 *
 * Copyright (C) 2013 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#define USE_GTK_RESIZE_HACK

#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>

#warning ****************************************************
#warning *** GTK UI IS DEPRECATED DO NOT USE THIS ANYMORE ***
#warning ****************************************************

/*****************************************************************************/

#define ROBTK_MOD_SHIFT GDK_SHIFT_MASK
#define ROBTK_MOD_CTRL  GDK_CONTROL_MASK

#define GTK_BACKEND
#include "robtk.h"

static void robtk_close_self(void *h) {
	// TODO
}

static int robtk_open_file_dialog(void *h, const char *title) {
	return -1; // TODO
}

static void robwidget_toplevel_enable_scaling (RobWidget* rw) {
	;
}
static void robtk_queue_scale_change (RobWidget *rw, const float ws) {
	;
}

/*****************************************************************************/

#include PLUGIN_SOURCE

/*****************************************************************************/

typedef struct {
	RobWidget* tl;
	LV2UI_Handle ui;
} GtkMetersLV2UI;

static const char * robtk_info(void *h) {
	return "v" VERSION;
}
/******************************************************************************
 * LV2 callbacks
 */

static LV2UI_Handle
gtk_instantiate(const LV2UI_Descriptor*   descriptor,
                const char*               plugin_uri,
                const char*               bundle_path,
                LV2UI_Write_Function      write_function,
                LV2UI_Controller          controller,
                LV2UI_Widget*             widget,
                const LV2_Feature* const* features)
{
	GtkMetersLV2UI* self = (GtkMetersLV2UI*)calloc(1, sizeof(GtkMetersLV2UI));
	GtkWidget *parent = NULL;
	*widget = NULL;

	for (int i = 0; features && features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_UI__parent)) {
			parent = (GtkWidget*)features[i]->data;
		}
	}

	self->ui = instantiate(self, descriptor, plugin_uri, bundle_path,
			write_function, controller, &self->tl, features);
	if (!self->ui) {
		free(self);
		return NULL;
	}

	*widget = self->tl->c;
	gtk_widget_show(self->tl->c);

	if (self->tl->size_default && parent) {
		int w, h;
		self->tl->size_default(self->tl, &w, &h);
		// TODO add existing top-level window size // somehow set default size of *widget only
		if (gtk_widget_get_toplevel(GTK_WIDGET(parent))) {
			gtk_window_resize(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(parent))), w, h);
		}
	}

	return self;
}

static void
gtk_cleanup(LV2UI_Handle handle)
{
	GtkMetersLV2UI* self = (GtkMetersLV2UI*)handle;
	cleanup(self->ui);
	free(self);
}

static void
gtk_port_event(LV2UI_Handle handle,
               uint32_t     port_index,
               uint32_t     buffer_size,
               uint32_t     format,
               const void*  buffer)
{
	GtkMetersLV2UI* self = (GtkMetersLV2UI*)handle;
	port_event(self->ui, port_index, buffer_size, format, buffer);
}

/******************************************************************************
 * LV2 setup
 */

static const void*
gtk_extension_data(const char* uri)
{
	return extension_data(uri);
}

static const LV2UI_Descriptor gtk_descriptor = {
	RTK_URI RTK_GUI "_gtk",
	gtk_instantiate,
	gtk_cleanup,
	gtk_port_event,
	gtk_extension_data
};

#undef LV2_SYMBOL_EXPORT
#ifdef _WIN32
#    define LV2_SYMBOL_EXPORT __declspec(dllexport)
#else
#    define LV2_SYMBOL_EXPORT  __attribute__ ((visibility ("default")))
#endif
LV2_SYMBOL_EXPORT
const LV2UI_Descriptor*
lv2ui_descriptor(uint32_t index)
{
	switch (index) {
	case 0:
		return &gtk_descriptor;
	default:
		return NULL;
	}
}
