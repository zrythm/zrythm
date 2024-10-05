// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_BACKEND_ARRANGER_SELECTIONS_H__
#define __GUI_BACKEND_ARRANGER_SELECTIONS_H__

#include <memory>
#include <vector>

#include "dsp/arranger_object_all.h"
#include "utils/icloneable.h"
#include "utils/traits.h"

class AudioClip;

/**
 * @addtogroup gui_backend
 *
 * @{
 */

constexpr int ARRANGER_SELECTIONS_MAGIC = 35867752;
#define IS_ARRANGER_SELECTIONS(x) \
  (static_cast<ArrangerSelections *> (x)->magic_ == ARRANGER_SELECTIONS_MAGIC)
#define IS_ARRANGER_SELECTIONS_AND_NONNULL(x) (x && IS_ARRANGER_SELECTIONS (x))

constexpr double ARRANGER_SELECTIONS_DEFAULT_NUDGE_TICKS = 0.1;

/**
 * @brief Represents a collection of selected objects in an arranger view.
 *
 * This class provides methods for managing the selections, such as
 * adding, removing, sorting, and pasting objects. It also includes
 * utility functions for querying the selections, like getting the
 * start and end positions, number of objects, and checking if the
 * selections contain certain properties.
 *
 * This collection is mainly used for performing actions.
 */
class ArrangerSelections : public ISerializable<ArrangerSelections>
{
public:
  enum class Type
  {
    None,
    Chord,
    Timeline,
    Midi,
    Automation,
    Audio,
  };

  enum class Property
  {
    HasLength,
    CanLoop,
    HasLooped,
    CanFade,
  };

public:
  ArrangerSelections () = default;
  ArrangerSelections (Type type) : type_ (type) { }
  virtual ~ArrangerSelections () = default;

  static std::unique_ptr<ArrangerSelections> new_from_type (Type type);

  /**
   * Inits the selections after loading a project.
   *
   * @param project Whether these are project selections (as opposed to
   * clones).
   * @param action To be passed when this is called from an undoable action.
   */
  void init_loaded (bool project, UndoableAction * action);

  /**
   * Initializes the selections.
   */
  void init (Type type);

  bool is_automation () const { return type_ == Type::Automation; }
  bool is_timeline () const { return type_ == Type::Timeline; }
  bool is_midi () const { return type_ == Type::Midi; }
  bool is_chord () const { return type_ == Type::Chord; }
  bool is_audio () const { return type_ == Type::Audio; }

#if 0
  /**
   * Appends the given object to the selections.
   */
  [[deprecated ("Use add_object() with rvalue ref instead")]] void
  add_object (const ArrangerObject &obj)
  {
    add_object (obj.clone_as<ArrangerObject> ());
  }
#endif

  /**
   * @brief Adds the given object clone to the selections.
   *
   * @param obj
   */
  void add_object_owned (std::unique_ptr<ArrangerObject> &&obj);

  /**
   * @brief Adds a reference to the given object to the selections.
   *
   * @param obj
   */
  void add_object_ref (const std::shared_ptr<ArrangerObject> &obj);

  /**
   * Sorts the selections by their indices (eg, for regions, their track
   * indices, then the lane indices, then the index in the lane).
   *
   * @note Only works for objects whose tracks exist.
   *
   * @param desc Descending or not.
   */
  virtual void sort_by_indices (bool desc) = 0;

  void sort_by_positions (bool desc);

  /**
   * Returns if there are any selections.
   */
  [[nodiscard]] virtual bool has_any () const { return objects_.size () > 0; };

#if 0
  /**
   * Returns the position of the leftmost object.
   *
   * @param global Return global (timeline) Position, otherwise returns the
   * local (from the start of the Region) Position.
   */
  Position get_start_pos (bool global) const;

  /**
   * Returns the end position of the rightmost object.
   *
   * @param global Return global (timeline) Position, otherwise returns the
   * local (from the start of the Region) Position.
   */
  Position get_end_pos (bool global) const;
#endif

  /**
   * Returns the number of selected objects.
   */
  [[nodiscard]] int get_num_objects () const { return objects_.size (); };

  template <typename T = ArrangerObject> int get_num_objects () const
  {
    return std::accumulate (
      objects_.begin (), objects_.end (), 0, [] (int sum, const auto &obj) {
        if (auto obj_ptr = std::dynamic_pointer_cast<T> (obj))
          {
            return sum + 1;
          }
        return sum;
      });
  }

  /**
   * Gets first object of the given type (if any, otherwise matches all types)
   * and its start position.
   *
   * @param global For non-timeline selections, whether to return the global
   * position (local + region start).
   */
  template <typename T = ArrangerObject>
    requires std::derived_from<T, ArrangerObject>
  std::pair<T *, Position> get_first_object_and_pos (bool global) const;

  /**
   * Gets last object of the given type (if any, otherwise matches all types)
   * and its end (if applicable, otherwise start) position.
   *
   * @param ends_last Whether to get the object that ends last, otherwise the
   * object that starts last.
   * @param global For non-timeline selections, whether to return the global
   * position (local + region start).
   */
  template <typename T = ArrangerObject>
    requires std::derived_from<T, ArrangerObject>
  std::pair<T *, Position>
  get_last_object_and_pos (bool global, bool ends_last) const;

  /**
   * Pastes the given selections to the given Position.
   */
  void paste_to_pos (const Position &pos, bool undoable);

  /**
   * Adds a clone of each object in the selection to the given region (if
   * applicable).
   */
  void add_to_region (Region &region);

  /**
   * Moves the selections by the given amount of ticks.
   *
   * @param ticks Ticks to add.
   */
  void add_ticks (const double ticks);

  /**
   * Selects all possible objects from the project.
   */
  void select_all (bool fire_events = false);

  /**
   * Clears selections.
   */
  void clear (bool fire_events = false);

  /**
   * Code to run after deserializing.
   */
  void post_deserialize ();

  /**
   * @brief Used for debugging.
   */
  bool validate () const;

  /**
   * Returns if the arranger object is in the selections or not.
   *
   * This uses the equality operator defined by each object.
   */
  virtual bool contains_object (const ArrangerObject &obj) const final;

  /**
   * Finds the given object, or null if not found.
   *
   * This uses the equality operator defined by each object.
   */
  template <typename T = ArrangerObject>
  T * find_object (const T &obj) const
    requires FinalClass<T> && DerivedButNotBase<T, ArrangerObject>
  {
    auto it =
      std::find_if (objects_.begin (), objects_.end (), [&obj] (auto &element) {
        return std::visit (
          [&] (auto &&derived_obj) {
            if constexpr (std::is_same_v<T, base_type<decltype (derived_obj)>>)
              {
                return obj == *derived_obj;
              }
            return false;
          },
          convert_to_variant<ArrangerObjectPtrVariant> (element.get ()));
        return obj == *element;
      });
    return it != objects_.end () ? dynamic_cast<T *> ((*it).get ()) : nullptr;
  }

  /**
   * Returns if the selections contain an undeletable object (such as the
   * start marker).
   */
  bool contains_undeletable_object () const;

  /**
   * Returns if the selections contain an unclonable object (such as the start
   * marker).
   */
  bool contains_unclonable_object () const;

  /** Whether the selections contain an unrenameable object (such as the start
   * marker). */
  bool contains_unrenamable_object () const;

  /**
   * Checks whether an object matches the given parameters.
   *
   * If a parameter should be checked, the has_* argument must be true and the
   * corresponding argument must have the value to be checked against.
   */
  bool contains_object_with_property (Property property, bool value) const;

  /**
   * Removes the object with the same address as the given object from the
   * selections.
   */
  void remove_object (const ArrangerObject &obj);

  /**
   * Merges the given selections into one region.
   *
   * @note All selections must be on the same lane.
   */
  virtual void merge () {};

  /**
   * Returns if the selections can be pasted at the current place/playhead.
   */
  bool can_be_pasted () const;

  /**
   * Returns if the selections can be pasted at the given position/region.
   *
   * @param pos Position to paste to.
   * @param idx Track index to start pasting to, if applicable.
   */
  bool can_be_pasted_at (const Position pos, const int idx = -1) const;

  virtual bool contains_looped () const { return false; };

  virtual bool can_be_merged () const { return false; };

  double get_length_in_ticks ();

  /** Whether the selections contain the given clip.*/
  virtual bool contains_clip (const AudioClip &clip) const { return false; };

  bool can_split_at_pos (const Position pos) const;

  static ArrangerSelections * get_for_type (ArrangerSelections::Type type);

protected:
  void copy_members_from (const ArrangerSelections &other)
  {
    type_ = other.type_;
    for (auto &obj : other.objects_)
      {
        objects_.emplace_back (
          clone_unique_with_variant<ArrangerObjectWithoutVelocityVariant> (
            obj.get ()));
      }
  }

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

private:
  bool is_object_allowed (const ArrangerObject &obj) const;

  /**
   * @brief Add owner region's ticks to the given position.
   *
   * @param[in,out] pos
   */
  void add_region_ticks (Position &pos) const;

  virtual bool
  can_be_pasted_at_impl (const Position pos, const int idx) const = 0;

public:
  /** Type of selections. */
  Type type_ = (Type) 0;

  int magic = ARRANGER_SELECTIONS_MAGIC;

  /** Either copies of selected objects (when used in actions), or shared
   * references to live objects in the project. */
  std::vector<std::shared_ptr<ArrangerObject>> objects_;
};

DEFINE_ENUM_FORMATTER (
  ArrangerSelections::Type,
  ArrangerSelections_Type,
  "None",
  "Chord",
  "Timeline",
  "MIDI",
  "Automation",
  "Audio");

class TimelineSelections;
class MidiSelections;
class ChordSelections;
class AutomationSelections;
class AudioSelections;
using ArrangerSelectionsVariant = std::variant<
  TimelineSelections,
  MidiSelections,
  ChordSelections,
  AutomationSelections,
  AudioSelections>;
using ArrangerSelectionsPtrVariant =
  to_pointer_variant<ArrangerSelectionsVariant>;

/**
 * @}
 */

extern template std::pair<MidiNote *, Position>
ArrangerSelections::get_first_object_and_pos (bool global) const;

extern template std::pair<MidiNote *, Position>
ArrangerSelections::get_last_object_and_pos (bool global, bool ends_last) const;

#endif
