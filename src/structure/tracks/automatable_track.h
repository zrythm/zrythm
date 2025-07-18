// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/processor_base.h"
#include "structure/tracks/automation_tracklist.h"

namespace zrythm::structure::tracks
{
/**
 * @brief Mixin for a track that has automatable parameters, to be used via
 * composition.
 *
 * @note Tracks that can have plugins must use this.
 */
class AutomatableTrackMixin : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    bool automationVisible READ automationVisible WRITE setAutomationVisible
      NOTIFY automationVisibleChanged)
  Q_PROPERTY (
    AutomationTracklist * automationTracklist READ automationTracklist CONSTANT)

public:
  AutomatableTrackMixin (
    AutomationTrackHolder::Dependencies dependencies,
    QObject *                           parent = nullptr);

  // ========================================================================
  // QML Interface
  // ========================================================================

  bool          automationVisible () const { return automation_visible_; }
  void          setAutomationVisible (bool visible);
  Q_SIGNAL void automationVisibleChanged (bool visible);

  AutomationTracklist * automationTracklist () const
  {
    return automation_tracklist_.get ();
  }

  // ========================================================================

  /**
   * @brief Generates automatables for the given processor.
   */
  void generate_automation_tracks_for_processor (
    std::vector<utils::QObjectUniquePtr<AutomationTrack>> &ret,
    const dsp::ProcessorBase                              &processor) const
  {
    z_debug (
      "generating automation tracks for {}...", processor.get_node_name ());
    for (const auto &param_ref : processor.get_parameters ())
      {
        auto * const param =
          param_ref.template get_object_as<dsp::ProcessorParameter> ();
        if (!param->automatable ())
          continue;

        ret.emplace_back (
          utils::make_qobject_unique<AutomationTrack> (
            dependencies_.tempo_map_, dependencies_.file_audio_source_registry_,
            dependencies_.object_registry_, param_ref));
      }
  }

protected:
  friend void init_from (
    AutomatableTrackMixin       &obj,
    const AutomatableTrackMixin &other,
    utils::ObjectCloneType       clone_type);

private:
  static constexpr auto kAutomationTracklistKey = "automationTracklist"sv;
  static constexpr auto kAutomationVisibleKey = "automationVisible"sv;
  friend void to_json (nlohmann::json &j, const AutomatableTrackMixin &track)
  {
    j[kAutomationTracklistKey] = track.automation_tracklist_;
    j[kAutomationVisibleKey] = track.automation_visible_;
  }
  friend void from_json (const nlohmann::json &j, AutomatableTrackMixin &track)
  {
    track.automation_tracklist_ = utils::make_qobject_unique<
      AutomationTracklist> (track.dependencies_, &track);
    from_json (j.at (kAutomationTracklistKey), *track.automation_tracklist_);
    j.at (kAutomationVisibleKey).get_to (track.automation_visible_);
  }

private:
  AutomationTrackHolder::Dependencies dependencies_;

  utils::QObjectUniquePtr<AutomationTracklist> automation_tracklist_;

  /** Flag to set automations visible or not. */
  bool automation_visible_{};
};
}
