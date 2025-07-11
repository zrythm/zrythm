// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "dsp/port.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_owner.h"
#include "structure/arrangement/automation_point.h"

#include <QtQmlIntegration>

namespace zrythm::structure::tracks
{
class AutomatableTrack;
class AutomationTracklist;

class AutomationTrack final
    : public QObject,
      public arrangement::ArrangerObjectOwner<arrangement::AutomationRegion>
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (double height READ getHeight WRITE setHeight NOTIFY heightChanged)
  Q_PROPERTY (dsp::ProcessorParameter * parameter READ parameter CONSTANT)
  Q_PROPERTY (
    int automationMode READ getAutomationMode WRITE setAutomationMode NOTIFY
      automationModeChanged)
  Q_PROPERTY (
    int recordMode READ getRecordMode WRITE setRecordMode NOTIFY
      recordModeChanged)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    AutomationTrack,
    regions,
    arrangement::AutomationRegion)

public:
  using PortUuid = dsp::Port::Uuid;
  using TrackGetter = std::function<TrackPtrVariant ()>;
  using ArrangerObjectRegistry = arrangement::ArrangerObjectRegistry;
  using AutomationRegion = arrangement::AutomationRegion;
  using AutomationPoint = arrangement::AutomationPoint;

  /** Release time in ms when in touch record mode. */
  static constexpr int AUTOMATION_RECORDING_TOUCH_REL_MS = 800;

  static constexpr int AUTOMATION_TRACK_DEFAULT_HEIGHT = 48;

  enum class AutomationMode : basic_enum_base_type_t
  {
    Read,
    Record,
    Off,
  };

  enum class AutomationRecordMode : basic_enum_base_type_t
  {
    Touch,
    Latch,
  };

public:
  /** Creates an automation track for the given parameter. */
  AutomationTrack (
    dsp::FileAudioSourceRegistry        &file_audio_source_registry,
    ArrangerObjectRegistry              &obj_registry,
    TrackGetter                          track_getter,
    dsp::ProcessorParameterUuidReference param_id);

public:
  // ========================================================================
  // QML Interface
  // ========================================================================

  dsp::ProcessorParameter * parameter () const
  {
    return port_id_.get_object_as<dsp::ProcessorParameter> ();
  }

  double        getHeight () const { return height_; }
  void          setHeight (double height);
  Q_SIGNAL void heightChanged (double height);

  int getAutomationMode () const
  {
    return ENUM_VALUE_TO_INT (automation_mode_);
  }
  void          setAutomationMode (int automation_mode);
  Q_SIGNAL void automationModeChanged (int automation_mode);

  int  getRecordMode () const { return ENUM_VALUE_TO_INT (record_mode_); }
  void setRecordMode (int record_mode);
  Q_SIGNAL void recordModeChanged (int record_mode);

  // ========================================================================

  void init_loaded ();

  void set_automation_mode (AutomationMode mode, bool fire_events);

  void swap_record_mode ()
  {
    record_mode_ = static_cast<AutomationRecordMode> (
      (std::to_underlying (record_mode_) + 1)
      % magic_enum::enum_count<AutomationRecordMode> ());
  }

  AutomationTracklist * get_automation_tracklist () const;

  /**
   * Returns whether the automation in the automation track should be read.
   */
  [[gnu::hot]] bool should_read_automation () const;

  /**
   * Returns if the automation track should currently be recording data.
   *
   * Returns false if in touch mode after the release time has passed.
   *
   * This function assumes that the transport will be rolling.
   *
   * @param record_aps If set to true, this function will return whether we
   *   should be recording automation point data. If set to false, this
   *   function will return whether we should be recording a region (eg, if an
   *   automation point was already created before and we are still recording
   *   inside a region regardless of whether we should create/edit automation
   *   points or not.
   */
  [[gnu::hot]] bool should_be_recording (bool record_aps) const;

  /**
   * Sets the index of the AutomationTrack in the AutomationTracklist.
   */
  void set_index (int index);

  /**
   * Clones the AutomationTrack.
   */
  friend void init_from (
    AutomationTrack       &obj,
    const AutomationTrack &other,
    utils::ObjectCloneType clone_type);

  TrackPtrVariant get_track () const;

  /**
   * Returns an automation point found within +/-
   * delta_ticks from the position, or NULL.
   *
   * @param before_only Only check previous automation
   *   points.
   */
  AutomationPoint * get_ap_around (
    double position_ticks,
    double delta_ticks,
    bool   before_only,
    bool   use_snapshots);

  /**
   * Returns the automation point before the Position on the timeline.
   *
   * @param ends_after Whether to only check in regions that also end after
   * \ref pos (ie, the region surrounds @ref pos), otherwise check in the
   * region that ends last.
   */
  AutomationPoint *
  get_ap_before_pos (signed_frame_t pos, bool ends_after, bool use_snapshots)
    const;

  /**
   * Returns the Region that starts before given Position, if any.
   *
   * @param ends_after Whether to only check for regions that also end after
   * @ref pos (ie, the region surrounds @ref pos), otherwise get the region
   * that ends last.
   */
  AutomationRegion *
  get_region_before_pos (signed_frame_t pos, bool ends_after, bool use_snapshots)
    const;

  /**
   * Returns the normalized parameter value at the given position.
   *
   * If there is no automation point/curve during the position, it returns the
   * current value of the parameter it is automating.
   *
   * @param ends_after Whether to only check in regions that also end after
   * \ref pos (ie, the region surrounds @ref pos), otherwise check in the
   * region that ends last.
   * @param use_snapshots Whether to get the value from the snapshotted
   * (cached) regions. This should be set to true when called during dsp
   * playback. TODO unimplemented
   *
   * @return The normalized value, or std::nullopt if there is no automation
   * point/curve at the position.
   */
  std::optional<float> get_normalized_val_at_pos (
    signed_frame_t pos,
    bool           ends_after,
    bool           use_snapshots) const;

  static int get_y_px_from_height_and_normalized_val (
    const float height,
    const float normalized_val)
  {
    return static_cast<int> (height - normalized_val * height);
  }

  /**
   * Returns the y pixels from the value based on the allocation of the
   * automation track.
   */
  int get_y_px_from_normalized_val (float normalized_val) const
  {
    return get_y_px_from_height_and_normalized_val (
      static_cast<float> (height_), normalized_val);
  }

  void set_caches (CacheType types);

  bool contains_automation () const { return !get_children_vector ().empty (); }

  std::string
  get_field_name_for_serialization (const AutomationRegion *) const override
  {
    return "regions";
  }

private:
  static constexpr std::string_view kIndexKey = "index";
  static constexpr std::string_view kPortIdKey = "portId";
  static constexpr std::string_view kCreatedKey = "created";
  static constexpr std::string_view kAutomationModeKey = "automationMode";
  static constexpr std::string_view kRecordModeKey = "recordMode";
  static constexpr std::string_view kVisibleKey = "visible";
  static constexpr std::string_view kHeightKey = "height";
  friend void to_json (nlohmann::json &j, const AutomationTrack &track)
  {
    to_json (j, static_cast<const ArrangerObjectOwner &> (track));
    j[kIndexKey] = track.index_;
    j[kPortIdKey] = track.port_id_;
    j[kCreatedKey] = track.created_;
    j[kAutomationModeKey] = track.automation_mode_;
    j[kRecordModeKey] = track.record_mode_;
    j[kVisibleKey] = track.visible_;
    j[kHeightKey] = track.height_;
  }
  friend void from_json (const nlohmann::json &j, AutomationTrack &track)
  {
    from_json (j, static_cast<ArrangerObjectOwner &> (track));
    j.at (kIndexKey).get_to (track.index_);
    j.at (kPortIdKey).get_to (track.port_id_);
    j.at (kCreatedKey).get_to (track.created_);
    j.at (kAutomationModeKey).get_to (track.automation_mode_);
    j.at (kRecordModeKey).get_to (track.record_mode_);
    j.at (kVisibleKey).get_to (track.visible_);
    j.at (kHeightKey).get_to (track.height_);
  }

public:
  ArrangerObjectRegistry &object_registry_;
  TrackGetter             track_getter_;

  /** Index in parent AutomationTracklist. */
  int index_ = 0;

  /** Identifier of the Port this AutomationTrack is for (owned pointer). */
  dsp::ProcessorParameterUuidReference port_id_;

  /** Whether it has been created by the user yet or not. */
  bool created_ = false;

  /**
   * Whether visible or not.
   *
   * Being created is a precondition for this.
   *
   * @important Must only be set with automation_tracklist_set_at_visible().
   */
  bool visible_ = false;

  /** Y local to track. */
  int y_ = 0;

  /** Position of multipane handle. */
  double height_ = AUTOMATION_TRACK_DEFAULT_HEIGHT;

  /** Last value recorded in this automation track. */
  float last_recorded_value_ = 0.f;

  /** Automation mode. */
  AutomationMode automation_mode_ = AutomationMode::Read;

  /** Automation record mode, when @ref automation_mode_ is set to record. */
  AutomationRecordMode record_mode_ = (AutomationRecordMode) 0;

  /** To be set to true when recording starts (when the first change is
   * received) and false when recording ends. */
  bool recording_started_ = false;

  /** Region currently recording to. */
  AutomationRegion * recording_region_ = nullptr;

  /**
   * This is a flag to let the recording manager know that a START signal was
   * already sent for recording.
   *
   * This is because @ref AutomationTrack.recording_region takes a cycle or 2
   * to become non-NULL.
   */
  bool recording_start_sent_ = false;

  /**
   * This must only be set by the RecordingManager when temporarily pausing
   * recording, eg when looping or leaving the punch range.
   *
   * See \ref
   * RECORDING_EVENT_TYPE_PAUSE_AUTOMATION_RECORDING.
   */
  bool recording_paused_ = false;

private:
  /** Pointer to owner automation tracklist, if any. */
  // AutomationTracklist * atl_ = nullptr;

  /** Cache used during DSP. */
  // std::optional<std::reference_wrapper<ControlPort>> port_;
};

}

DEFINE_ENUM_FORMATTER (
  zrythm::structure::tracks::AutomationTrack::AutomationRecordMode,
  AutomationRecordMode,
  QT_TR_NOOP_UTF8 ("Touch"),
  QT_TR_NOOP_UTF8 ("Latch"));

DEFINE_ENUM_FORMATTER (
  zrythm::structure::tracks::AutomationTrack::AutomationMode,
  AutomationMode,
  "On",
  "Rec",
  "Off");
