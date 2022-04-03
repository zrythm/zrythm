/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Text expander widget.
 */

#ifndef __GUI_WIDGETS_TEXT_EXPANDER_H__
#define __GUI_WIDGETS_TEXT_EXPANDER_H__

#include "gui/widgets/expander_box.h"
#include "utils/types.h"

#include <gtk/gtk.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored \
  "-Wdeprecated-declarations"
#include <gtksourceview/gtksource.h>
#pragma GCC diagnostic pop

#define TEXT_EXPANDER_WIDGET_TYPE \
  (text_expander_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TextExpanderWidget,
  text_expander_widget,
  Z,
  TEXT_EXPANDER_WIDGET,
  ExpanderBoxWidget);

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * A TwoColExpanderBoxWidget for showing the ports
 * in the InspectorWidget.
 */
typedef struct _TextExpanderWidget
{
  ExpanderBoxWidget parent_instance;

  /** Getter for the string. */
  GenericStringGetter getter;

  /** Setter for the string. */
  GenericStringSetter setter;

  /** Object to call get/set on. */
  void * obj;

  /** Scrolled window for the editor inside. */
  GtkScrolledWindow * scroll;
  GtkViewport *       viewport;

  /** Editor. */
  GtkSourceView * editor;

  /** Editor buffer. */
  GtkSourceBuffer * buffer;

  GtkLabel * label;

  GtkMenuButton * edit_btn;

  GtkPopover * popover;

  bool has_focus;
} TextExpanderWidget;

/**
 * Refreshes the text.
 */
void
text_expander_widget_refresh (
  TextExpanderWidget * self);

/**
 * Sets up the TextExpanderWidget.
 */
void
text_expander_widget_setup (
  TextExpanderWidget * self,
  bool                 wrap_text,
  GenericStringGetter  getter,
  GenericStringSetter  setter,
  void *               obj);

/**
 * @}
 */

#endif
