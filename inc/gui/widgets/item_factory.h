// SPDX-FileCopyrightText: © 2021-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Item factory.
 */

/**
 * @addtogroup widgets
 *
 * @{
 */

#ifndef __GUI_WIDGETS_ITEM_FACTORY_H__
#define __GUI_WIDGETS_ITEM_FACTORY_H__

#include <gtk/gtk.h>

/**
 * Item factory column type.
 */
typedef enum ItemFactoryType
{
  ITEM_FACTORY_TOGGLE,
  ITEM_FACTORY_TEXT,

  /** Integer display. */
  ITEM_FACTORY_INTEGER,

  /** Icon. */
  ITEM_FACTORY_ICON,

  /** Composite type (eg, used in plugin browser). */
  ITEM_FACTORY_ICON_AND_TEXT,

  /** Position. */
  ITEM_FACTORY_POSITION,

  /** Color. */
  ITEM_FACTORY_COLOR,
} ItemFactoryType;

/**
 * Item factory for column views.
 *
 * The owner widget is expected to have a GPtrArray
 * that holds an instance of this for each column.
 * 1 column = 1 ItemFactory.
 *
 * This can also be used on listviews (ie, only 1
 * ItemFactory would be needed).
 */
typedef struct ItemFactory
{
  GtkListItemFactory * list_item_factory;

  ItemFactoryType type;

  bool editable;

  bool ellipsize_label;

  /** Column name, or NULL if used for list
   * views. */
  char * column_name;
} ItemFactory;

/**
 * Creates a new item factory.
 *
 * @param editable Whether the item should be editable.
 * @param column_name Column name, if column view, otherwise
 *   NULL.
 */
ItemFactory *
item_factory_new (ItemFactoryType type, bool editable, const char * column_name);

/**
 * Shorthand to generate and append a column to
 * a column view.
 *
 * @return The newly created ItemFactory, for
 *   convenience.
 */
ItemFactory *
item_factory_generate_and_append_column (
  GtkColumnView * column_view,
  GPtrArray *     item_factories,
  ItemFactoryType type,
  bool            editable,
  bool            resizable,
  GtkSorter *     sorter,
  const char *    column_name);

void
item_factory_free (ItemFactory * self);

void
item_factory_free_func (void * self);

/**
 * @}
 */

#endif
