// SPDX-FileCopyrightText: © 2021-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @addtogroup widgets
 *
 * @{
 */

#ifndef __GUI_WIDGETS_ITEM_FACTORY_H__
#define __GUI_WIDGETS_ITEM_FACTORY_H__

#include <memory>
#include <string>
#include <vector>

#include "gui/backend/gtk_widgets/gtk_wrapper.h"

/**
 * Item factory for column views.
 *
 * The owner widget is expected to have a std::vector<ItemFactory>
 * that holds an ItemFactory for each column.
 * 1 column = 1 ItemFactory.
 *
 * This can also be used on listviews (ie, only 1 ItemFactory would be needed).
 */
class ItemFactory
{
public:
  /**
   * Item factory column type.
   */
  enum class Type
  {
    Toggle,
    Text,

    /** Integer display. */
    Integer,

    /** Icon. */
    Icon,

    /** Composite type (eg, used in plugin browser). */
    IconAndText,

    /** Position. */
    Position,

    /** Color. */
    Color,
  };

public:
  /**
   * Creates a new item factory.
   *
   * @param editable Whether the item should be editable.
   * @param column_name Column name, if column view, otherwise
   *   NULL.
   */
  ItemFactory (Type type, bool editable, std::string column_name);
  ~ItemFactory ();

  /**
   * Shorthand to generate and append a column to a column view.
   */
  static std::unique_ptr<ItemFactory> &generate_and_append_column (
    GtkColumnView *                            column_view,
    std::vector<std::unique_ptr<ItemFactory>> &item_factories,
    Type                                       type,
    bool                                       editable,
    bool                                       resizable,
    GtkSorter *                                sorter,
    std::string                                column_name);

public:
  /**
   * @brief An owned GtkListItemFactory.
   */
  GtkListItemFactory * list_item_factory_ = nullptr;

  Type type_ = Type::Text;

  bool editable_ = false;

  bool ellipsize_label_ = true;

  gulong setup_id_ = 0;
  gulong bind_id_ = 0;
  gulong unbind_id_ = 0;
  gulong teardown_id_ = 0;

  /** Column name, or empty if used for list views. */
  std::string column_name_;
};

using ItemFactoryPtrVector = std::vector<std::unique_ptr<ItemFactory>>;

/**
 * @}
 */

#endif
