/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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
} ItemFactoryType;

typedef struct ItemFactory
{
  GtkListItemFactory * list_item_factory;

  ItemFactoryType      type;

  bool                 editable;

  /** Column name. */
  char *               column_name;
} ItemFactory;

ItemFactory *
item_factory_new (
  ItemFactoryType type,
  bool            editable,
  const char *    column_name);

void
item_factory_generate_and_append_column (
  GtkColumnView * column_view,
  GPtrArray *     item_factories,
  ItemFactoryType type,
  bool            editable,
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
