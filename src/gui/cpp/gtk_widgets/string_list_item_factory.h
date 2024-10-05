// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * String list item factory.
 */

#ifndef __GUI_WIDGETS_STRING_LIST_ITEM_FACTORY_H__
#define __GUI_WIDGETS_STRING_LIST_ITEM_FACTORY_H__

#include <string>

#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef std::string (*StringListItemFactoryEnumStringGetter) (int val);

/**
 * Returns a simple item factory that puts each string in a
 * GtkLabel.
 *
 * Can be used with AdwEnumListItem and GtkStringObject.
 */
GtkListItemFactory *
string_list_item_factory_new (StringListItemFactoryEnumStringGetter getter);

/**
 * @}
 */

#endif
