// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/file_audio_source.h"
#include "structure/arrangement/arranger_object.h"
#include "utils/icloneable.h"

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
    const dsp::TempoMap              &tempo_map,
    dsp::FileAudioSourceRegistry     &registry,
    dsp::FileAudioSourceUuidReference source,
    QObject *                         parent = nullptr);
  Z_DISABLE_COPY_MOVE (AudioSourceObject)
  ~AudioSourceObject () override;

  // ========================================================================
  // QML Interface
  // ========================================================================

  // ========================================================================

  juce::PositionableAudioSource &get_audio_source () const;

  dsp::FileAudioSourceUuidReference audio_source_ref () const;

private:
  friend void init_from (
    AudioSourceObject       &obj,
    const AudioSourceObject &other,
    utils::ObjectCloneType   clone_type);

  static constexpr auto kFileAudioSourceKey = "fileAudioSource"sv;
  friend void to_json (nlohmann::json &j, const AudioSourceObject &obj)
  {
    to_json (j, static_cast<const ArrangerObject &> (obj));
    j[kFileAudioSourceKey] = obj.source_id_;
  }
  friend void from_json (const nlohmann::json &j, AudioSourceObject &obj);

  void generate_audio_source ();

private:
  dsp::FileAudioSourceRegistry                  &registry_;
  dsp::FileAudioSourceUuidReference              source_id_;
  std::unique_ptr<juce::PositionableAudioSource> source_;

  BOOST_DESCRIBE_CLASS (AudioSourceObject, (ArrangerObject), (), (), ())
};

} // namespace zrythm::structure::arrangement
