/*
 * Copyright (C) 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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

typedef enum ItemFactoryType
{
  ITEM_FACTORY_TOGGLE,
  ITEM_FACTORY_TEXT,

  /** Integer display. */
  ITEM_FACTORY_INTEGER,

  /** Composite type (eg, used in plugin
   * browser). */
  ITEM_FACTORY_ICON_AND_TEXT,

  /** Position. */
  ITEM_FACTORY_POSITION,
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

  ItemFactoryType      type;

  bool                 editable;

  /** Column name, or NULL if used for list
   * views. */
  char *               column_name;
} ItemFactory;

/**
 * Creates a new item factory.
 *
 * @param editable Whether the item should be
 *   editable.
 * @param column_name Column name, if column view,
 *   otherwise NULL.
 */
ItemFactory *
item_factory_new (
  ItemFactoryType type,
  bool            editable,
  const char *    column_name);

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
item_factory_free (
  ItemFactory * self);

void
item_factory_free_func (
  void * self);

/**
 * @}
 */

#endif
