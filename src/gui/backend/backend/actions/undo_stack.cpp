// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/utils/gtest_wrapper.h"
#include "gui/backend/backend/actions/undo_stack.h"
#include "gui/backend/backend/actions/undoable_action_all.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"

using namespace zrythm::gui::actions;

UndoStack::UndoStack (QObject * parent) : QAbstractListModel (parent)
{
  const bool testing_or_benchmarking = ZRYTHM_TESTING || ZRYTHM_BENCHMARKING;

  int undo_stack_length =
    testing_or_benchmarking
      ? gZrythm->undo_stack_len_
      : zrythm::gui::SettingsManager::get_instance ()->get_undoStackLength ();
  max_size_ = undo_stack_length;

  connect (
    this, &QAbstractListModel::rowsInserted, this,
    [&] (const QModelIndex &, int, int) {
      Q_EMIT rowCountChanged (rowCount ());
    });
  connect (
    this, &QAbstractListModel::rowsRemoved, this,
    [&] (const QModelIndex &, int, int) {
      Q_EMIT rowCountChanged (rowCount ());
    });
}

void
UndoStack::init_loaded (sample_rate_t engine_sample_rate)
{
  for (auto &action : actions_)
    {
      std::visit (
        [engine_sample_rate] (auto * ptr) {
          ptr->init_loaded (engine_sample_rate);
        },
        action);
    }
}

void
UndoStack::init_after_cloning (const UndoStack &other)
{
  max_size_ = other.max_size_;

  /* clone all actions */
  for (const auto &action : other.actions_)
    {
      std::visit (
        [this] (auto * ptr) { actions_.push_back (ptr->clone_qobject (this)); },
        action);
    }

  /* init loaded to load the actions on the stack */
  // init_loaded ();
}

// ======================================================================
// QML Interface
// ======================================================================

QHash<int, QByteArray>
UndoStack::roleNames () const
{
  QHash<int, QByteArray> roles;
  roles[UndoableActionPtrRole] = "undoableAction";
  return roles;
}

int
UndoStack::rowCount (const QModelIndex &parent) const
{
  return static_cast<int> (actions_.size ());
}

QVariant
UndoStack::data (const QModelIndex &index, int role) const
{
  if (index.row () < 0 || index.row () >= static_cast<int> (actions_.size ()))
    return {};

  if (role == UndoableActionPtrRole)
    {
      return QVariant::fromStdVariant (actions_.at (index.row ()));
    }
  return {};
}

// ======================================================================

template <UndoableActionSubclass T>
void
UndoStack::push (T * action)
{
  if (is_full ())
    {
      pop_last ();
    }
  beginInsertRows ({}, 0, 0);
  actions_.insert (actions_.cbegin (), action);
  action->setParent (this);
  endInsertRows ();
}

std::optional<UndoableActionPtrVariant>
UndoStack::pop ()
{
  if (actions_.empty ())
    {
      return std::nullopt;
    }

  auto action = std::visit (
    [] (auto * ptr) -> std::optional<UndoableActionPtrVariant> {
      ptr->setParent (nullptr);
      return ptr;
    },
    actions_.front ());

  beginRemoveRows ({}, 0, 0);
  actions_.erase (actions_.begin ());
  endRemoveRows ();
  return action;
}

std::optional<UndoableActionPtrVariant>
UndoStack::pop_last ()
{
  if (actions_.empty ())
    {
      return std::nullopt;
    }

  auto action = std::visit (
    [] (auto * ptr) -> std::optional<UndoableActionPtrVariant> {
      ptr->setParent (nullptr);
      return ptr;
    },
    actions_.back ());

  beginRemoveRows ({}, std::ssize (actions_) - 1, std::ssize (actions_) - 1);
  actions_.pop_back ();
  endRemoveRows ();
  return action;
}

std::optional<UndoableActionPtrVariant>
UndoStack::peek () const
{
  return is_empty ()
           ? std::nullopt
           : std::visit (
               [] (auto * ptr) -> std::optional<UndoableActionPtrVariant> {
                 return ptr;
               },
               actions_.front ());
}

bool
UndoStack::contains_clip (const AudioClip &clip) const
{
  return std::ranges::any_of (actions_, [&clip] (const auto &action) {
    return std::visit (
      [&clip] (auto * ptr) { return ptr->contains_clip (clip); }, action);
  });
}

template <UndoableActionSubclass T>
bool
UndoStack::contains_action (const T &ua) const
{
  return std::ranges::any_of (actions_, [&ua] (const auto &action) {
    return std::visit (
      [&ua] (auto &&ptr) {
        using ActionT = base_type<decltype (ptr)>;
        if constexpr (std::is_same_v<ActionT, T>)
          {
            return ptr == &ua;
          }
        return false;
      },
      action);
  });
}

void
UndoStack::get_plugins (std::vector<zrythm::plugins::Plugin *> &arr) const
{
  for (const auto &action : actions_)
    {
      std::visit ([&arr] (auto * ptr) { ptr->get_plugins (arr); }, action);
    }
}

std::string
UndoStack::get_as_string (int limit) const
{
  std::string ret;

  auto count = std::ssize (actions_) - 1; // start from the last action
  for (
    auto it = actions_.rbegin ();
    it != actions_.rend () && count >= std::ssize (actions_) - limit;
    ++it, --count)
    {
      auto action_str =
        std::visit ([] (auto * ptr) { return ptr->to_string (); }, *it);
      ret += fmt::format ("[{}] {}\n", count, action_str);
    }

  return ret;
}

template void
UndoStack::push (ArrangerSelectionsAction * action);
template void
UndoStack::push (ChannelSendAction * action);
template void
UndoStack::push (ChordAction * action);
template void
UndoStack::push (MidiMappingAction * action);
template void
UndoStack::push (MixerSelectionsAction * action);
template void
UndoStack::push (PortAction * action);
template void
UndoStack::push (PortConnectionAction * action);
template void
UndoStack::push (RangeAction * action);
template void
UndoStack::push (TracklistSelectionsAction * action);
template void
UndoStack::push (TransportAction * action);

template bool
UndoStack::contains_action (const ArrangerSelectionsAction &action) const;
template bool
UndoStack::contains_action (const ChannelSendAction &action) const;
template bool
UndoStack::contains_action (const ChordAction &action) const;
template bool
UndoStack::contains_action (const MidiMappingAction &action) const;
template bool
UndoStack::contains_action (const MixerSelectionsAction &action) const;
template bool
UndoStack::contains_action (const PortAction &action) const;
template bool
UndoStack::contains_action (const PortConnectionAction &action) const;
template bool
UndoStack::contains_action (const RangeAction &action) const;
template bool
UndoStack::contains_action (const TracklistSelectionsAction &action) const;
template bool
UndoStack::contains_action (const TransportAction &action) const;