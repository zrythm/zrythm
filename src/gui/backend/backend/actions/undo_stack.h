// SPDX-FileCopyrightText: © 2020-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/backend/backend/actions/undoable_action_all.h"
#include "utils/icloneable.h"
#include "utils/serialization.h"

namespace zrythm::gui::actions
{

/**
 * Serializable stack for undoable actions.
 *
 * This is used for both undo and redo.
 */
class UndoStack : public QAbstractListModel
{
  Q_OBJECT
  // QML_ELEMENT
  Q_PROPERTY (int rowCount READ rowCount NOTIFY rowCountChanged FINAL)

public:
  UndoStack (QObject * parent = nullptr);
  Q_DISABLE_COPY_MOVE (UndoStack)
  ~UndoStack () override = default;

  // ====================================================================
  // QML Interface
  // ====================================================================

  enum UndoableActionRoles
  {
    UndoableActionPtrRole = Qt::UserRole + 1,
  };

  QHash<int, QByteArray> roleNames () const override;
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  Q_SIGNAL void rowCountChanged (int rowCount);

  // ====================================================================

  void init_loaded (sample_rate_t engine_sample_rate);

  /**
   * Gets the list of actions as a string.
   */
  [[nodiscard]] std::string get_as_string (int limit) const;

  /* --- start wrappers --- */

  /**
   * @brief Take ownership of the given action.
   *
   * @tparam T
   * @param action
   */
  template <UndoableActionSubclass T> void push (T * action);

  /**
   * @note The caller takes ownership of the action.
   *
   * @return std::optional<UndoableActionPtrVariant>
   */
  std::optional<UndoableActionPtrVariant> pop ();

  /**
   * @brief Pops the last (first added) element and moves everything back.
   *
   * @note The caller takes ownership of the action.
   */
  std::optional<UndoableActionPtrVariant> pop_last ();
  auto size () const { return actions_.size (); }
  bool is_empty () const { return actions_.empty (); }
  bool empty () const { return is_empty (); }
  bool is_full () const { return size () == max_size_; }

  /**
   * @brief
   *
   * @note The action is owned by the undo stack.
   *
   * @return std::optional<UndoableActionPtrVariant>
   */
  std::optional<UndoableActionPtrVariant> peek () const;

  void clear () { actions_.clear (); };

  /* --- end wrappers --- */

  /**
   * Checks if the undo stack contains the given action pointer.
   */
  template <UndoableActionSubclass T>
  [[nodiscard]] bool contains_action (const T &ua) const;

  /**
   * Returns the plugins referred to in the undo stack.
   */
  void get_plugins (std::vector<zrythm::plugins::Plugin *> &arr) const;

  friend void init_from (
    UndoStack             &obj,
    const UndoStack       &other,
    utils::ObjectCloneType clone_type);

private:
  static constexpr auto kMaxLengthKey = "maxLength"sv;
  friend void           to_json (nlohmann::json &j, const UndoStack &stack)
  {
    // rest TODO
    j[kMaxLengthKey] = stack.max_size_;
  }
  friend void from_json (const nlohmann::json &j, UndoStack &stack)
  {
    j.at (kMaxLengthKey).get_to (stack.max_size_);
  }

public:
  /**
   * @brief Actions on the stack.
   */
  std::vector<UndoableActionPtrVariant> actions_;

  /**
   * @brief Max size of the stack (if 0, unlimited).
   */
  size_t max_size_ = 0;
};

extern template void
UndoStack::push (ArrangerSelectionsAction * action);
extern template void
UndoStack::push (ChannelSendAction * action);
extern template void
UndoStack::push (ChordAction * action);
extern template void
UndoStack::push (MidiMappingAction * action);
extern template void
UndoStack::push (MixerSelectionsAction * action);
extern template void
UndoStack::push (PortAction * action);
extern template void
UndoStack::push (PortConnectionAction * action);
extern template void
UndoStack::push (RangeAction * action);
extern template void
UndoStack::push (TracklistSelectionsAction * action);
extern template void
UndoStack::push (TransportAction * action);

extern template bool
UndoStack::contains_action (const ArrangerSelectionsAction &action) const;
extern template bool
UndoStack::contains_action (const ChannelSendAction &action) const;
extern template bool
UndoStack::contains_action (const ChordAction &action) const;
extern template bool
UndoStack::contains_action (const MidiMappingAction &action) const;
extern template bool
UndoStack::contains_action (const MixerSelectionsAction &action) const;
extern template bool
UndoStack::contains_action (const PortAction &action) const;
extern template bool
UndoStack::contains_action (const PortConnectionAction &action) const;
extern template bool
UndoStack::contains_action (const RangeAction &action) const;
extern template bool
UndoStack::contains_action (const TracklistSelectionsAction &action) const;
extern template bool
UndoStack::contains_action (const TransportAction &action) const;

}; // namespace zrythm::gui::actions
