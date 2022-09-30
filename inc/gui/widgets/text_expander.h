// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
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
text_expander_widget_refresh (TextExpanderWidget * self);

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
