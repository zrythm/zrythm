// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Text expander widget.
 */

#ifndef __GUI_WIDGETS_TEXT_EXPANDER_H__
#define __GUI_WIDGETS_TEXT_EXPANDER_H__

#include "gui/cpp/gtk_widgets/expander_box.h"
#include "utils/types.h"

#include "gtk_wrapper.h"

#define TEXT_EXPANDER_WIDGET_TYPE (text_expander_widget_get_type ())
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
using TextExpanderWidget = struct _TextExpanderWidget
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
  GtkTextView * editor;

  /** Editor buffer. */
  GtkTextBuffer * buffer;

  GtkLabel * label;

  GtkMenuButton * edit_btn;

  GtkPopover * popover;

  bool has_focus;
};

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
