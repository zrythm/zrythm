// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/file_audio_source.h"
#include "structure/arrangement/arranger_object.h"
#include "utils/icloneable.h"
#include "utils/iobject_registry.h"

#include <nlohmann/json_fwd.hpp>

namespace zrythm::structure::arrangement
{
/**
 * Audio source for an AudioRegion.
 *
 * This is a separate class so that region linking can later be implemented more
 * easily via a delegate to ArrangerObjectOwner (since audio region is now also
 * an ArrangerObjectOwner).
 */
class AudioSourceObject final : public ArrangerObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  /**
   * @brief Construct a new Audio Source Object object
   *
   * When constructing for deserialization, @p source can be a dummy source
   * (from_json will generate the correct one).
   *
   * @param registry
   * @param source
   * @param tempo_map
   * @param parent
   */
  AudioSourceObject (
    const dsp::TempoMapWrapper       &tempo_map_wrapper,
    utils::IObjectRegistry           &registry,
    dsp::FileAudioSourceUuidReference source,
    QObject *                         parent = nullptr);
  Q_DISABLE_COPY_MOVE (AudioSourceObject)
  ~AudioSourceObject () override;

  // ========================================================================
  // QML Interface
  // ========================================================================

  // ========================================================================

  juce::PositionableAudioSource &get_audio_source () const;

  /**
   * @brief Returns the underlying FileAudioSource.
   *
   * Carries the raw samples and the clip's permanent source BPM, which the
   * region needs to compute its musical length and stretch ratios.
   */
  dsp::FileAudioSource &file_audio_source () const;

  dsp::FileAudioSourceUuidReference audio_source_ref () const;

private:
  friend void init_from (
    AudioSourceObject       &obj,
    const AudioSourceObject &other,
    utils::ObjectCloneType   clone_type);

  static constexpr auto kFileAudioSourceKey = "fileAudioSource"sv;
  friend void to_json (nlohmann::json &j, const AudioSourceObject &obj);
  friend void from_json (const nlohmann::json &j, AudioSourceObject &obj);

  Q_SLOT void generate_audio_source ();
  void        connect_file_audio_source_signals ();

private:
  utils::IObjectRegistry                        &registry_;
  dsp::FileAudioSourceUuidReference              source_id_;
  std::unique_ptr<juce::PositionableAudioSource> source_;

  BOOST_DESCRIBE_CLASS (AudioSourceObject, (ArrangerObject), (), (), ())
};

} // namespace zrythm::structure::arrangement
