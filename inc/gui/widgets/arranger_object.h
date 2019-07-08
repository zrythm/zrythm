/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * Macros for arranger object widgets.
 */

#ifndef __GUI_WIDGETS_ARRANGER_OBJECT_H__
#define __GUI_WIDGETS_ARRANGER_OBJECT_H__

/**
 * Selects the widget by adding it to the selections.
 *
 * @param cc CamelCase.
 * @param sc snake_case.
 */
#define ARRANGER_OBJECT_WIDGET_SELECT( \
  caps,cc,sc,selections_name,selections) \
  cc * main_##sc = \
    sc##_get_main_##sc (sc); \
  if (select) \
    { \
      selections_name##_add_##sc ( \
        selections, \
        main_##sc); \
    } \
  else \
    { \
      selections_name##_remove_##sc ( \
        selections, \
        main_##sc); \
    }

#define DECLARE_ARRANGER_OBJECT_WIDGET_SELECT( \
  cc, sc) \
  void \
  sc##_widget_select ( \
    cc##Widget * self, \
    int  select)

#define DEFINE_ARRANGER_OBJECT_WIDGET_SELECT( \
  caps,cc,sc,selections_name,selections) \
  void \
  sc##_widget_select ( \
    cc##Widget * self, \
    int  select) \
  { \
    cc * sc = self->sc; \
    ARRANGER_OBJECT_WIDGET_SELECT ( \
      caps, cc, sc, selections_name, selections); \
  }

#endif
