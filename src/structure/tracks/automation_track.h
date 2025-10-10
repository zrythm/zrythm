// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "dsp/parameter.h"
#include "dsp/processor_base.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "structure/arrangement/arranger_object_owner.h"
#include "structure/arrangement/automation_region.h"
#include "utils/units.h"

#include <QtQmlIntegration>

namespace zrythm::structure::tracks
{
class AutomationTrack
    : public QObject,
      public arrangement::ArrangerObjectOwner<arrangement::AutomationRegion>
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (dsp::ProcessorParameter * parameter READ parameter CONSTANT)
  Q_PROPERTY (
    AutomationMode automationMode READ getAutomationMode WRITE setAutomationMode
      NOTIFY automationModeChanged)
  Q_PROPERTY (
    AutomationRecordMode recordMode READ getRecordMode WRITE setRecordMode
      NOTIFY recordModeChanged)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    AutomationTrack,
    regions,
    arrangement::AutomationRegion)
  QML_UNCREATABLE ("")

public:
  using ArrangerObjectRegistry = arrangement::ArrangerObjectRegistry;
  using AutomationRegion = arrangement::AutomationRegion;
  using AutomationPoint = arrangement::AutomationPoint;

  /** Release time in ms when in touch record mode. */
  static constexpr int AUTOMATION_RECORDING_TOUCH_REL_MS = 800;

  enum class AutomationMode : basic_enum_base_type_t
  {
    Read,
    Record,
    Off,
  };
  Q_ENUM (AutomationMode)

  enum class AutomationRecordMode : basic_enum_base_type_t
  {
    Touch,
    Latch,
  };
  Q_ENUM (AutomationRecordMode)

public:
  /** Creates an automation track for the given parameter. */
  AutomationTrack (
    const dsp::TempoMapWrapper          &tempo_map,
    dsp::FileAudioSourceRegistry        &file_audio_source_registry,
    ArrangerObjectRegistry              &obj_registry,
    dsp::ProcessorParameterUuidReference param_id,
    QObject *                            parent = nullptr);

public:
  // ========================================================================
  // QML Interface
  // ========================================================================

  dsp::ProcessorParameter * parameter () const
  {
    return param_id_.get_object_as<dsp::ProcessorParameter> ();
  }

  AutomationMode getAutomationMode () const { return automation_mode_; }
  void           setAutomationMode (AutomationMode automation_mode);
  Q_SIGNAL void  automationModeChanged (AutomationMode automation_mode);

  AutomationRecordMode getRecordMode () const { return record_mode_; }
  void                 setRecordMode (AutomationRecordMode record_mode);
  Q_INVOKABLE void     swapRecordMode ()
  {
    record_mode_ = static_cast<AutomationRecordMode> (
      (std::to_underlying (record_mode_) + 1)
      % magic_enum::enum_count<AutomationRecordMode> ());
    Q_EMIT recordModeChanged (record_mode_);
  }
  Q_SIGNAL void recordModeChanged (AutomationRecordMode record_mode);

  // ========================================================================

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
   * Find an automation point near the given position @p position_ticks with a
   * tolerance of @p delta_ticks.
   *
   * This is used eg, in the UI to check if we should select an existing
   * automation point on user click, or whether to create a new automation point.

   * @param search_only_backwards Only check previous automation points.
   */
  AutomationPoint * get_automation_point_around (
    units::precise_tick_t position_ticks,
    units::precise_tick_t delta_ticks,
    bool                  search_only_backwards = false);

  /**
   * Returns the last automation point before the given timeline position.
   *
   * This first finds the relevant automation regio nusing get_region_before(),
   * then finds the last point located before the position (converted to
   * region-local coordinates).
   *
   * @see get_region_before().
   */
  AutomationPoint * get_automation_point_before (
    units::sample_t timeline_position,
    bool            search_only_regions_enclosing_position) const;

  /**
   * Returns the active region at a given position, if any.
   *
   * @param search_only_regions_enclosing_position Whether to only check in
   * regions that span the given position. If this is false, positions that end
   * before the position are also considered. When this is true and multiple
   * regions exist that meet the requirements, the behavior is not defined - any
   * region could be picked.
   */
  auto get_region_before (
    units::sample_t pos_samples,
    bool search_only_regions_enclosing_position) const -> AutomationRegion *;

  /**
   * Returns the normalized parameter value at the given timeline position.
   *
   * @param search_only_regions_enclosing_position @see get_region_before().
   *
   * @return The normalized parameter value, or std::nullopt if there is no
   * automation point/curve at the position.
   */
  std::optional<float> get_normalized_value (
    units::sample_t timeline_frames,
    bool            search_only_regions_enclosing_position) const;

  bool contains_automation () const { return !get_children_vector ().empty (); }

  std::string
  get_field_name_for_serialization (const AutomationRegion *) const override
  {
    return "regions";
  }

public:
  static constexpr auto kParamIdKey = "paramId"sv;

private:
  static constexpr auto kAutomationModeKey = "automationMode"sv;
  static constexpr auto kRecordModeKey = "recordMode"sv;
  friend void to_json (nlohmann::json &j, const AutomationTrack &track)
  {
    to_json (j, static_cast<const ArrangerObjectOwner &> (track));
    j[kParamIdKey] = track.param_id_;
    j[kAutomationModeKey] = track.automation_mode_;
    j[kRecordModeKey] = track.record_mode_;
  }
  friend void from_json (const nlohmann::json &j, AutomationTrack &track)
  {
    from_json (j, static_cast<ArrangerObjectOwner &> (track));
    j.at (kParamIdKey).get_to (track.param_id_);
    j.at (kAutomationModeKey).get_to (track.automation_mode_);
    j.at (kRecordModeKey).get_to (track.record_mode_);
  }

  friend void init_from (
    AutomationTrack       &obj,
    const AutomationTrack &other,
    utils::ObjectCloneType clone_type);

private:
  const dsp::TempoMapWrapper &tempo_map_;
  ArrangerObjectRegistry     &object_registry_;

  /** Parameter this AutomationTrack is for. */
  dsp::ProcessorParameterUuidReference param_id_;

  /** Automation mode. */
  AutomationMode automation_mode_ = AutomationMode::Read;

  /** Automation record mode, when @ref automation_mode_ is set to record. */
  AutomationRecordMode record_mode_ = (AutomationRecordMode) 0;
};

/**
 * @brief Generates automatables for the given processor.
 */
inline void
generate_automation_tracks_for_processor (
  std::vector<utils::QObjectUniquePtr<AutomationTrack>> &ret,
  const dsp::ProcessorBase                              &processor,
  const dsp::TempoMapWrapper                            &tempo_map,
  dsp::FileAudioSourceRegistry        &file_audio_source_registry,
  arrangement::ArrangerObjectRegistry &object_registry)
{
  z_debug ("generating automation tracks for {}...", processor.get_node_name ());
  for (const auto &param_ref : processor.get_parameters ())
    {
      auto * const param =
        param_ref.template get_object_as<dsp::ProcessorParameter> ();
      if (!param->automatable ())
        continue;

      ret.emplace_back (
        utils::make_qobject_unique<AutomationTrack> (
          tempo_map, file_audio_source_registry, object_registry, param_ref));
    }
}
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
