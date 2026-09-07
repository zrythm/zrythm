// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <set>

#include "dsp/cv_port.h"
#include "dsp/file_audio_source.h"
#include "dsp/parameter.h"
#include "plugins/plugin_factory.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/arrangement/audio_clip.h"
#include "structure/arrangement/audio_source_object.h"
#include "structure/arrangement/marker.h"
#include "structure/arrangement/midi_clip.h"
#include "structure/arrangement/midi_note.h"
#include "structure/project/clipboard_json_schema.h"
#include "structure/project/clipboard_payload.h"
#include "structure/tracks/track_factory.h"
#include "utils/app_settings.h"
#include "utils/compression.h"
#include "utils/exceptions.h"
#include "utils/registry_utils.h"
#include "utils/serialization.h"

#include "helpers/in_memory_settings_backend.h"
#include "helpers/mock_plugin_host_window.h"
#include "helpers/scoped_qcoreapplication.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nlohmann/json-schema.hpp>

namespace zrythm::structure::project
{

class ClipboardPayloadTest
    : public ::testing::Test,
      public test_helpers::ScopedQCoreApplication
{
protected:
  void SetUp () override
  {
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper_ = std::make_unique<dsp::TempoMapWrapper> (*tempo_map_);

    const auto make_arranger_factory = [this] (utils::IObjectRegistry &reg) {
      return std::make_unique<arrangement::ArrangerObjectFactory> (
        arrangement::ArrangerObjectFactory::Dependencies{
          .tempo_map_ = *tempo_map_wrapper_,
          .registry_ = reg,
          .last_timeline_obj_len_provider_ = [] () { return 100.0; },
          .last_editor_obj_len_provider_ = [] () { return 50.0; },
          .automation_curve_algorithm_provider_ =
            [] () { return dsp::CurveOptions::Algorithm::Exponent; } },
        [] () { return units::sample_rate (44100); },
        [] () { return units::bpm (120.0); });
    };
    arranger_factory_ = make_arranger_factory (source_registry_);
    target_arranger_factory_ = make_arranger_factory (target_registry_);

    // Factories needed for payload imports (one set per registry)
    const auto make_track_factory = [this] (utils::IObjectRegistry &reg) {
      structure::tracks::FinalTrackDependencies track_deps{
        *tempo_map_wrapper_,
        reg,
        structure::tracks::SoloedTracksExistGetter{ [] () { return false; } },
        {},
      };
      return std::make_unique<structure::tracks::TrackFactory> ([track_deps] () {
        return track_deps;
      });
    };
    const auto make_plugin_factory = [this] (utils::IObjectRegistry &reg) {
      return std::make_unique<
        plugins::PluginFactory> (plugins::PluginFactory::CommonFactoryDependencies{
        .registry = reg,
        .create_plugin_instance_async_func_ =
          [] (
            const juce::PluginDescription &, double, int,
            juce::AudioPluginFormat::PluginCreationCallback callback) {
            callback (nullptr, "No plugin in payload tests");
          },
        .sample_rate_provider_ = [] () { return units::sample_rate (44100); },
        .buffer_size_provider_ = [] () { return units::samples (256u); },
        .top_level_window_provider_ =
          test_helpers::make_mock_plugin_host_window_factory (
            std::make_shared<test_helpers::MockPluginHostWindowState> ()),
        .main_thread_dispatcher_ = main_dispatcher_ });
    };
    track_factory_ = make_track_factory (source_registry_);
    plugin_factory_ = make_plugin_factory (source_registry_);
    target_track_factory_ = make_track_factory (target_registry_);
    target_plugin_factory_ = make_plugin_factory (target_registry_);
    source_registry_.set_deserialization_dependencies (
      { *track_factory_, *arranger_factory_, *plugin_factory_ });
    target_registry_.set_deserialization_dependencies (
      { *target_track_factory_, *target_arranger_factory_,
        *target_plugin_factory_ });

    // --- Source objects ---
    midi_clip_ref_ = utils::create_object<arrangement::MidiClip> (
      source_registry_, *tempo_map_wrapper_, source_registry_);
    note1_ref_ = utils::create_object<arrangement::MidiNote> (
      source_registry_, *tempo_map_wrapper_);
    note2_ref_ = utils::create_object<arrangement::MidiNote> (
      source_registry_, *tempo_map_wrapper_);
    cc_event_ref_ = utils::create_object<arrangement::MidiControlEvent> (
      source_registry_, *tempo_map_wrapper_);
    marker_ref_ = utils::create_object<arrangement::Marker> (
      source_registry_, *tempo_map_wrapper_,
      arrangement::Marker::MarkerType::Custom);

    note1_ref_.get ()->position ()->setTicks (100.0);
    note1_ref_.get ()->length ()->setTicks (400.0);
    note2_ref_.get ()->position ()->setTicks (600.0);
    midi_clip_ref_.get ()->position ()->setTicks (2000.0);
    midi_clip_ref_.get ()->length ()->setTicks (4000.0);
    marker_ref_.get ()->position ()->setTicks (5000.0);

    auto * midi_clip = midi_clip_ref_.get_object_as<arrangement::MidiClip> ();
    midi_clip->arrangement::ArrangerObjectOwner<
      arrangement::MidiNote>::add_object (note1_ref_);
    midi_clip->arrangement::ArrangerObjectOwner<
      arrangement::MidiNote>::add_object (note2_ref_);
    midi_clip->arrangement::ArrangerObjectOwner<
      arrangement::MidiControlEvent>::add_object (cc_event_ref_);

    // Audio content: file audio source -> audio source object -> audio clip
    fas_ref_ = utils::create_object<dsp::FileAudioSource> (
      source_registry_, utils::audio::AudioBuffer (2, 64),
      dsp::FileAudioSource::BitDepth::BIT_DEPTH_16, units::sample_rate (44100),
      units::bpm (120.0), utils::Utf8String::from_utf8_encoded_string ("test"));
    audio_source_ref_ = utils::create_object<arrangement::AudioSourceObject> (
      source_registry_, *tempo_map_wrapper_, source_registry_, fas_ref_);
    audio_clip_ref_ = utils::create_object<arrangement::AudioClip> (
      source_registry_, *tempo_map_wrapper_, source_registry_);
    audio_clip_ref_.get_object_as<arrangement::AudioClip> ()
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioSourceObject>::add_object (audio_source_ref_);

    // Parameter with a modulation source (boundary reference)
    cv_port_ref_ = utils::create_object<dsp::CVPort> (
      source_registry_, u8"mod", dsp::PortFlow::Output);
    {
      auto param = std::make_unique<dsp::ProcessorParameter> (
        source_registry_, dsp::ProcessorParameter::UniqueId{},
        dsp::ParameterRange{}, utils::Utf8String{});
      source_registry_.register_object (*param);
      param_ref_ = dsp::ProcessorParameterUuidReference (
        param->get_uuid (), source_registry_);
      param.release ();

      nlohmann::json param_json = *param_ref_.get ();
      param_json[dsp::ProcessorParameter::kModulationSourcePortIdKey] =
        cv_port_ref_;
      param_json.get_to (*param_ref_.get ());
    }
  }

  static bool json_contains_uuid (const nlohmann::json &j, const QUuid &id)
  {
    const auto str = id.toString (QUuid::WithoutBraces).toStdString ();
    if (j.is_string ())
      return j.get<std::string> () == str;
    if (j.is_object ())
      {
        for (const auto &value : j)
          if (json_contains_uuid (value, id))
            return true;
        return false;
      }
    if (j.is_array ())
      {
        for (const auto &el : j)
          if (json_contains_uuid (el, id))
            return true;
      }
    return false;
  }

  static QUuid to_quuid (const auto &typed_uuid)
  {
    return type_safe::get (typed_uuid);
  }

  static const nlohmann::json *
  find_entry (const nlohmann::json &bucket, const QUuid &id)
  {
    const auto str = id.toString (QUuid::WithoutBraces).toStdString ();
    const auto it = std::ranges::find_if (bucket, [&] (const auto &entry) {
      return entry.at ("id").template get<std::string> () == str;
    });
    return it == bucket.end () ? nullptr : &*it;
  }

  ProjectRegistry source_registry_;
  ProjectRegistry target_registry_;

  std::unique_ptr<dsp::TempoMap>        tempo_map_;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper_;

  std::unique_ptr<arrangement::ArrangerObjectFactory> arranger_factory_;
  std::unique_ptr<arrangement::ArrangerObjectFactory> target_arranger_factory_;
  std::unique_ptr<structure::tracks::TrackFactory>    track_factory_;
  std::unique_ptr<plugins::PluginFactory>             plugin_factory_;
  std::unique_ptr<structure::tracks::TrackFactory>    target_track_factory_;
  std::unique_ptr<plugins::PluginFactory>             target_plugin_factory_;

  QObject                            dispatcher_context_;
  utils::MainThreadClosureDispatcher main_dispatcher_{
    dispatcher_context_, std::chrono::milliseconds{ 10 }
  };

  arrangement::ArrangerObjectUuidReference midi_clip_ref_{ source_registry_ };
  arrangement::ArrangerObjectUuidReference note1_ref_{ source_registry_ };
  arrangement::ArrangerObjectUuidReference note2_ref_{ source_registry_ };
  arrangement::ArrangerObjectUuidReference cc_event_ref_{ source_registry_ };
  arrangement::ArrangerObjectUuidReference marker_ref_{ source_registry_ };
  dsp::FileAudioSourceUuidReference        fas_ref_{ source_registry_ };
  arrangement::ArrangerObjectUuidReference audio_source_ref_{ source_registry_ };
  arrangement::ArrangerObjectUuidReference audio_clip_ref_{ source_registry_ };
  utils::TypedUuidReference<dsp::CVPort>   cv_port_ref_{ source_registry_ };
  dsp::ProcessorParameterUuidReference     param_ref_{ source_registry_ };
};

TEST_F (ClipboardPayloadTest, CreateCollectsClosure)
{
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::ArrangerObjects,
    { to_quuid (midi_clip_ref_.id ()), to_quuid (marker_ref_.id ()) },
    QStringLiteral ("project-a"));

  const auto &arranger_bucket =
    payload.registry_json ().at (ProjectRegistry::kArrangerObjectsKey);
  // clip, 2 notes, CC event, marker
  EXPECT_EQ (arranger_bucket.size (), 5u);
  EXPECT_TRUE (
    json_contains_uuid (arranger_bucket, to_quuid (note1_ref_.id ())));
  EXPECT_TRUE (
    json_contains_uuid (arranger_bucket, to_quuid (note2_ref_.id ())));

  // the clip entry references its notes
  const auto * clip_entry =
    find_entry (arranger_bucket, to_quuid (midi_clip_ref_.id ()));
  ASSERT_NE (clip_entry, nullptr);
  EXPECT_TRUE (json_contains_uuid (*clip_entry, to_quuid (note1_ref_.id ())));
  EXPECT_TRUE (json_contains_uuid (*clip_entry, to_quuid (note2_ref_.id ())));

  EXPECT_EQ (payload.roots ().size (), 2u);
  EXPECT_EQ (payload.source_project_id (), u"project-a");
}

TEST_F (ClipboardPayloadTest, CreateDoesNotFollowModulationSource)
{
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::Plugins,
    { to_quuid (param_ref_.id ()) }, QStringLiteral ("project-a"));

  // The modulation source port is a boundary reference and must not be
  // pulled into the payload
  EXPECT_EQ (
    payload.registry_json ().at (ProjectRegistry::kParametersKey).size (), 1u);
  EXPECT_EQ (
    payload.registry_json ().at (ProjectRegistry::kPortsKey).size (), 0u);
}

TEST_F (ClipboardPayloadTest, EncodeDecodeRoundTrip)
{
  nlohmann::json metadata;
  metadata[ClipboardPayload::kAnchorTicksMetadataKey] = 2000.0;
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::ArrangerObjects,
    { to_quuid (midi_clip_ref_.id ()) }, QStringLiteral ("project-a"), metadata);

  // direct JSON round-trip (no compression)
  const nlohmann::json payload_json = payload;
  const auto           direct = payload_json.get<ClipboardPayload> ();
  EXPECT_EQ (direct.type (), payload.type ());
  EXPECT_TRUE (std::ranges::equal (direct.roots (), payload.roots ()));
  EXPECT_EQ (direct.registry_json (), payload.registry_json ());

  const auto decoded = ClipboardPayload::decode_from_clipboard_text (
    payload.encode_to_clipboard_text ());
  ASSERT_TRUE (decoded.has_value ());
  EXPECT_EQ (decoded->type (), ClipboardPayload::Type::ArrangerObjects);
  EXPECT_EQ (decoded->source_project_id (), u"project-a");
  EXPECT_TRUE (std::ranges::equal (decoded->roots (), payload.roots ()));
  EXPECT_EQ (decoded->metadata (), payload.metadata ());
  EXPECT_EQ (decoded->registry_json (), payload.registry_json ());
}

TEST_F (ClipboardPayloadTest, PayloadValidatesAgainstSchema)
{
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::ArrangerObjects,
    {
      to_quuid (midi_clip_ref_.id ()), to_quuid (marker_ref_.id ()),
      to_quuid (audio_clip_ref_.id ())
  },
    QStringLiteral ("project-a"),
    nlohmann::json{ { ClipboardPayload::kAnchorTicksMetadataKey, 2000.0 } });

  nlohmann::json_schema::json_validator validator (
    nlohmann::json::parse (kClipboardSchemaJsonStr), nullptr,
    nlohmann::json_schema::default_string_format_check, nullptr);
  const nlohmann::json payload_json = payload;
  EXPECT_NO_THROW (validator.validate (payload_json));
}

TEST_F (ClipboardPayloadTest, PayloadWithMidiControlEventValidatesAndDecodes)
{
  // The variant type discriminator of a MidiControlEvent is the highest
  // index in ArrangerObjectVariant; the schema's base bound must admit it
  // or every payload containing a CC/pitch-bend event is rejected at decode
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::ArrangerObjects,
    {
      to_quuid (midi_clip_ref_.id ())
  },
    QStringLiteral ("project-a"),
    nlohmann::json{ { ClipboardPayload::kAnchorTicksMetadataKey, 2000.0 } });
  ASSERT_TRUE (
    payload.registry_json ().at (ProjectRegistry::kArrangerObjectsKey).size ()
    >= 4u)
    << "fixture must actually contain a MidiControlEvent";

  nlohmann::json_schema::json_validator validator (
    nlohmann::json::parse (kClipboardSchemaJsonStr), nullptr,
    nlohmann::json_schema::default_string_format_check, nullptr);
  const nlohmann::json payload_json = payload;
  EXPECT_NO_THROW (validator.validate (payload_json));

  const auto decoded = ClipboardPayload::decode_from_clipboard_text (
    payload.encode_to_clipboard_text ());
  ASSERT_TRUE (decoded.has_value ());
  EXPECT_FALSE (
    decoded->registry_json ().at (ProjectRegistry::kArrangerObjectsKey).empty ());
}

TEST_F (ClipboardPayloadTest, DecodeRejectsOutlierAnchorTicks)
{
  // The schema bounds metadata numbers so hostile magnitudes cannot reach
  // paste positioning
  nlohmann::json j;
  j[utils::serialization::kDocumentTypeKey] =
    std::string (ClipboardPayload::kDocumentType);
  j["formatVersion"] = ClipboardPayload::kFormatVersion;
  j["payloadType"] = "arrangerObjects";
  j["sourceProjectId"] = "project-a";
  j["metadata"] = nlohmann::json{
    { ClipboardPayload::kAnchorTicksMetadataKey, 1e300 }
  };
  j["registry"] = nlohmann::json::object ();
  j["roots"] = nlohmann::json::array ();

  const auto text =
    QString::fromUtf8 (
      ClipboardPayload::kTextPrefix.data (),
      ClipboardPayload::kTextPrefix.size ())
    + QString::fromUtf8 (
      utils::compression::compress_to_base64_str (
        QByteArray::fromStdString (j.dump ())));
  EXPECT_FALSE (
    ClipboardPayload::decode_from_clipboard_text (text).has_value ());
}

TEST_F (ClipboardPayloadTest, DecodeRejectsGarbage)
{
  EXPECT_FALSE (
    ClipboardPayload::decode_from_clipboard_text (QStringLiteral ("hello"))
      .has_value ());
  EXPECT_FALSE (
    ClipboardPayload::decode_from_clipboard_text (QString ()).has_value ());
}

TEST_F (ClipboardPayloadTest, DecodeRejectsDeeplyNestedJson)
{
  // A deeply nested document must be rejected by the parser's depth cap
  // instead of overflowing the stack
  const std::string deep_json (100000, '[');
  const auto        text =
    QString::fromUtf8 (
      ClipboardPayload::kTextPrefix.data (),
      ClipboardPayload::kTextPrefix.size ())
    + QString::fromUtf8 (
      utils::compression::compress_to_base64_str (
        QByteArray::fromStdString (deep_json)));
  EXPECT_FALSE (
    ClipboardPayload::decode_from_clipboard_text (text).has_value ());
}

TEST_F (ClipboardPayloadTest, DecodeRejectsOversizedText)
{
  const auto text =
    QString::fromUtf8 (
      ClipboardPayload::kTextPrefix.data (),
      ClipboardPayload::kTextPrefix.size ())
    + QString (
      ClipboardPayload::kMaxClipboardTextLength
        - static_cast<qsizetype> (ClipboardPayload::kTextPrefix.size ()) + 1,
      QLatin1Char ('x'));
  EXPECT_FALSE (
    ClipboardPayload::decode_from_clipboard_text (text).has_value ());
}

TEST_F (ClipboardPayloadTest, DecodeRejectsSchemaInvalidPayload)
{
  // A valid envelope (document type and format version) but a registry
  // without the required buckets must be rejected at the decode boundary
  nlohmann::json j;
  j[utils::serialization::kDocumentTypeKey] =
    std::string (ClipboardPayload::kDocumentType);
  j["formatVersion"] = ClipboardPayload::kFormatVersion;
  j["payloadType"] = "arrangerObjects";
  j["sourceProjectId"] = "project-a";
  j["metadata"] = nlohmann::json::object ();
  j["registry"] = nlohmann::json::object ();
  j["roots"] = nlohmann::json::array ();
  const auto text =
    QString::fromUtf8 (
      ClipboardPayload::kTextPrefix.data (),
      ClipboardPayload::kTextPrefix.size ())
    + QString::fromUtf8 (
      utils::compression::compress_to_base64_str (
        QByteArray::fromStdString (j.dump ())));
  EXPECT_FALSE (
    ClipboardPayload::decode_from_clipboard_text (text).has_value ());
}

TEST_F (ClipboardPayloadTest, DecodeRejectsDanglingInteriorReference)
{
  // The schema proves shape, not closure: a child reference to an
  // object the payload does not carry must be rejected at the decode
  // boundary instead of importing a null-resolving reference
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::ArrangerObjects,
    {
      to_quuid (midi_clip_ref_.id ())
  },
    QStringLiteral ("project-a"),
    nlohmann::json{ { ClipboardPayload::kAnchorTicksMetadataKey, 2000.0 } });

  nlohmann::json j = payload;
  auto          &entries = j["registry"][ProjectRegistry::kArrangerObjectsKey];
  ASSERT_GT (entries.size (), 1u)
    << "fixture must copy a clip with at least one child";
  // Renaming the first child's id severs the clip's reference to it
  // while leaving the payload schema-valid
  entries[1]["id"] =
    QUuid::createUuid ().toString (QUuid::WithoutBraces).toStdString ();

  const auto text =
    QString::fromUtf8 (
      ClipboardPayload::kTextPrefix.data (),
      ClipboardPayload::kTextPrefix.size ())
    + QString::fromUtf8 (
      utils::compression::compress_to_base64_str (
        QByteArray::fromStdString (j.dump ())));
  EXPECT_FALSE (
    ClipboardPayload::decode_from_clipboard_text (text).has_value ());
}

TEST_F (ClipboardPayloadTest, DecodeRejectsWrongFormatVersion)
{
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::ArrangerObjects,
    { to_quuid (marker_ref_.id ()) }, QStringLiteral ("project-a"));
  nlohmann::json j = payload;
  j["formatVersion"] = ClipboardPayload::kFormatVersion + 1;

  const auto text =
    QString::fromUtf8 (
      ClipboardPayload::kTextPrefix.data (),
      ClipboardPayload::kTextPrefix.size ())
    + QString::fromUtf8 (
      utils::compression::compress_to_base64_str (
        QByteArray::fromStdString (j.dump ())));
  EXPECT_FALSE (
    ClipboardPayload::decode_from_clipboard_text (text).has_value ());
}

TEST_F (ClipboardPayloadTest, DecodeRejectsDecompressionBomb)
{
  // Compressed input whose decompressed size exceeds the 64 MiB payload
  // cap must be rejected by the size check before decompression
  const std::string bomb (65ULL * 1024 * 1024 + 1, 'a');
  const auto        text =
    QString::fromUtf8 (
      ClipboardPayload::kTextPrefix.data (),
      ClipboardPayload::kTextPrefix.size ())
    + QString::fromUtf8 (
      utils::compression::compress_to_base64_str (
        QByteArray::fromStdString (bomb)));
  EXPECT_FALSE (
    ClipboardPayload::decode_from_clipboard_text (text).has_value ());
}

TEST_F (ClipboardPayloadTest, DecodeRejectsValuesOutsideSchemaBounds)
{
  // JSON 1e999 parses to infinity, which a plain number check admits;
  // the schema's value bounds must reject hostile magnitudes (tempo
  // here is representative of every bounded field)
  const auto tempo_object_ref = utils::create_object<
    structure::arrangement::TempoObject> (source_registry_, *tempo_map_wrapper_);
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::ArrangerObjects,
    { to_quuid (tempo_object_ref.id ()) }, QStringLiteral ("project-a"));
  nlohmann::json j = payload;

  const auto decode_with_tempo_value = [&j] (const std::string &value) {
    auto       json_text = j.dump ();
    const auto tempo_pos = json_text.find ("\"tempo\":");
    EXPECT_NE (tempo_pos, std::string::npos);
    const auto value_start = json_text.find_first_not_of (' ', tempo_pos + 8);
    const auto value_end = json_text.find_first_of (",}", value_start);
    json_text.replace (value_start, value_end - value_start, value);
    return ClipboardPayload::decode_from_clipboard_text (
      QString::fromUtf8 (
        ClipboardPayload::kTextPrefix.data (),
        ClipboardPayload::kTextPrefix.size ())
      + QString::fromUtf8 (
        utils::compression::compress_to_base64_str (
          QByteArray::fromStdString (json_text))));
  };

  EXPECT_FALSE (decode_with_tempo_value ("1e999").has_value ());
  EXPECT_TRUE (decode_with_tempo_value ("999.0").has_value ());
}

TEST_F (ClipboardPayloadTest, ImportIntoRefusesDuplicateIds)
{
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::ArrangerObjects,
    { to_quuid (midi_clip_ref_.id ()) }, QStringLiteral ("project-a"));

  // Importing without regenerating UUIDs would clash with the objects
  // already registered in the source registry
  EXPECT_THROW (payload.import_into (source_registry_), ZrythmException);

  // The pre-existing objects are untouched
  EXPECT_TRUE (source_registry_.contains (to_quuid (midi_clip_ref_.id ())));
  EXPECT_TRUE (source_registry_.contains (to_quuid (note1_ref_.id ())));
}

TEST_F (ClipboardPayloadTest, ImportIntoRefusesPayloadInternalDuplicateIds)
{
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::ArrangerObjects,
    { to_quuid (midi_clip_ref_.id ()) }, QStringLiteral ("project-a"));

  // Duplicate one entry inside the payload itself: registration of the
  // second copy throws and the first must be rolled back cleanly
  nlohmann::json j = payload;
  auto          &arranger_bucket = j["registry"]["arrangerObjects"];
  arranger_bucket.push_back (arranger_bucket.front ());
  const auto broken = j.get<ClipboardPayload> ();

  EXPECT_THROW (broken.import_into (target_registry_), ZrythmException);
  EXPECT_FALSE (target_registry_.contains (to_quuid (midi_clip_ref_.id ())));
}

TEST_F (ClipboardPayloadTest, ImportFailureKeepsSharedFileAudioSourcesRegistered)
{
  // An asset registered in the target but not referenced by anything,
  // the state every freshly registered object has before its first
  // referencer attaches: a failed import must not be able to reach it
  auto target_fas = utils::make_qobject_unique<dsp::FileAudioSource> (
    utils::audio::AudioBuffer (2, 64),
    dsp::FileAudioSource::BitDepth::BIT_DEPTH_16, units::sample_rate (44100),
    units::bpm (120.0), utils::Utf8String::from_utf8_encoded_string ("test"));
  const auto target_fas_id = to_quuid (target_fas->get_uuid ());
  target_registry_.register_object (*target_fas);
  target_fas.release ();
  ASSERT_TRUE (target_registry_.contains (target_fas_id));

  // A marker payload whose file-audio-source bucket carries the target's
  // asset id, plus a duplicated entry that makes registration fail halfway
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::ArrangerObjects,
    { to_quuid (marker_ref_.id ()) }, QStringLiteral ("project-a"));
  const auto     remapped = ClipboardPayload::with_regenerated_uuids (payload);
  nlohmann::json j = remapped;
  auto          &arranger_bucket = j["registry"]["arrangerObjects"];
  arranger_bucket.push_back (arranger_bucket.front ());
  j["registry"]["fileAudioSources"] = nlohmann::json::array (
    { nlohmann::json{
      { "id", target_fas_id.toString (QUuid::WithoutBraces).toStdString () } } });
  const auto broken = j.get<ClipboardPayload> ();

  EXPECT_THROW (broken.import_into (target_registry_), ZrythmException);

  // The shared asset still belongs to the target project, and the
  // partially imported marker was rolled back
  EXPECT_TRUE (target_registry_.contains (target_fas_id));
  EXPECT_FALSE (target_registry_.contains (remapped.roots ().front ()));
}

TEST_F (ClipboardPayloadTest, RegeneratedUuidsAreFreshAndConsistent)
{
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::ArrangerObjects,
    { to_quuid (midi_clip_ref_.id ()) }, QStringLiteral ("project-a"));
  const auto remapped = ClipboardPayload::with_regenerated_uuids (payload);

  const auto &bucket =
    remapped.registry_json ().at (ProjectRegistry::kArrangerObjectsKey);
  EXPECT_EQ (bucket.size (), 4u);

  // every UUID is fresh
  for (const auto &entry : bucket)
    {
      const auto id = QUuid::fromString (
        QString::fromStdString (entry.at ("id").get<std::string> ()));
      EXPECT_NE (id, to_quuid (midi_clip_ref_.id ()));
      EXPECT_NE (id, to_quuid (note1_ref_.id ()));
      EXPECT_NE (id, to_quuid (note2_ref_.id ()));
    }

  // the remapped clip references the remapped notes
  const auto   new_clip_id = remapped.roots ().front ();
  const auto * clip_entry = find_entry (bucket, new_clip_id);
  ASSERT_NE (clip_entry, nullptr);
  std::vector<QUuid> note_ids;
  for (const auto &entry : bucket)
    {
      const auto id = QUuid::fromString (
        QString::fromStdString (entry.at ("id").get<std::string> ()));
      if (id != new_clip_id)
        note_ids.push_back (id);
    }
  ASSERT_EQ (note_ids.size (), 3u);
  EXPECT_TRUE (json_contains_uuid (*clip_entry, note_ids[0]));
  EXPECT_TRUE (json_contains_uuid (*clip_entry, note_ids[1]));
  EXPECT_TRUE (json_contains_uuid (*clip_entry, note_ids[2]));
}

TEST_F (ClipboardPayloadTest, RegeneratedUuidsKeepFileAudioSources)
{
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::ArrangerObjects,
    { to_quuid (audio_clip_ref_.id ()) }, QStringLiteral ("project-a"));
  const auto remapped = ClipboardPayload::with_regenerated_uuids (payload);

  // audio clip and source object get fresh UUIDs
  EXPECT_NE (remapped.roots ().front (), to_quuid (audio_clip_ref_.id ()));
  const auto &fas_bucket =
    remapped.registry_json ().at (ProjectRegistry::kFileAudioSourcesKey);
  ASSERT_EQ (fas_bucket.size (), 1u);
  EXPECT_TRUE (json_contains_uuid (fas_bucket, to_quuid (fas_ref_.id ())));

  // the source object still references the original file audio source
  const auto &arranger_bucket =
    remapped.registry_json ().at (ProjectRegistry::kArrangerObjectsKey);
  EXPECT_TRUE (json_contains_uuid (arranger_bucket, to_quuid (fas_ref_.id ())));
}

TEST_F (ClipboardPayloadTest, FilteredForTargetSameProjectKeepsEverything)
{
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::ArrangerObjects,
    { to_quuid (audio_clip_ref_.id ()), to_quuid (midi_clip_ref_.id ()) },
    QStringLiteral ("project-a"));

  const auto filtered = payload.filtered_for_target (source_registry_);
  ASSERT_TRUE (filtered.payload.has_value ());
  EXPECT_TRUE (
    std::ranges::equal (filtered.payload->roots (), payload.roots ()));
  EXPECT_EQ (filtered.payload->registry_json (), payload.registry_json ());
  EXPECT_EQ (filtered.dropped_audio_objects, 0u);
  EXPECT_EQ (filtered.severed_references, 0u);
}

TEST_F (ClipboardPayloadTest, FilteredForTargetSameProjectKeepsModulationSource)
{
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::Plugins,
    { to_quuid (param_ref_.id ()) }, QStringLiteral ("project-a"));

  const auto filtered = payload.filtered_for_target (source_registry_);
  ASSERT_TRUE (filtered.payload.has_value ());

  // The modulation source still resolves in this project, so the boundary
  // reference is kept
  const auto * param_entry = find_entry (
    filtered.payload->registry_json ().at (ProjectRegistry::kParametersKey),
    to_quuid (param_ref_.id ()));
  ASSERT_NE (param_entry, nullptr);
  EXPECT_EQ (
    param_entry->at (dsp::ProcessorParameter::kModulationSourcePortIdKey),
    to_quuid (cv_port_ref_.id ()).toString (QUuid::WithoutBraces).toStdString ());
}

TEST_F (
  ClipboardPayloadTest,
  FilteredForTargetCrossProjectClearsUnresolvableModulationSource)
{
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::Plugins,
    { to_quuid (param_ref_.id ()) }, QStringLiteral ("project-a"));

  const auto filtered = payload.filtered_for_target (target_registry_);
  ASSERT_TRUE (filtered.payload.has_value ());

  // The modulation source port lives only in the source project, so the
  // reference cannot resolve and is serialized as null
  const auto * param_entry = find_entry (
    filtered.payload->registry_json ().at (ProjectRegistry::kParametersKey),
    to_quuid (param_ref_.id ()));
  ASSERT_NE (param_entry, nullptr);
  EXPECT_TRUE (
    param_entry->at (dsp::ProcessorParameter::kModulationSourcePortIdKey)
      .is_null ());
  EXPECT_EQ (filtered.severed_references, 1u);
}

TEST_F (
  ClipboardPayloadTest,
  FilteredForTargetSameProjectSeversDeletedModulationSource)
{
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::Plugins,
    { to_quuid (param_ref_.id ()) }, QStringLiteral ("project-a"));

  // The modulation source port was deleted after the copy was made (the
  // target registry here plays the project with the port gone), so the
  // reference is severed even in a same-project paste
  const auto filtered = payload.filtered_for_target (target_registry_);
  ASSERT_TRUE (filtered.payload.has_value ());

  const auto * param_entry = find_entry (
    filtered.payload->registry_json ().at (ProjectRegistry::kParametersKey),
    to_quuid (param_ref_.id ()));
  ASSERT_NE (param_entry, nullptr);
  EXPECT_TRUE (
    param_entry->at (dsp::ProcessorParameter::kModulationSourcePortIdKey)
      .is_null ());
  EXPECT_EQ (filtered.severed_references, 1u);
}

TEST_F (ClipboardPayloadTest, FilteredForTargetCrossProjectDropsAudio)
{
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::ArrangerObjects,
    { to_quuid (audio_clip_ref_.id ()), to_quuid (midi_clip_ref_.id ()) },
    QStringLiteral ("project-a"));

  const auto filtered = payload.filtered_for_target (target_registry_);
  ASSERT_TRUE (filtered.payload.has_value ());

  // audio content dropped, MIDI content survives
  const auto &arranger_bucket = filtered.payload->registry_json ().at (
    ProjectRegistry::kArrangerObjectsKey);
  EXPECT_EQ (arranger_bucket.size (), 4u); // midi clip + 2 notes + CC event
  EXPECT_TRUE (
    json_contains_uuid (arranger_bucket, to_quuid (midi_clip_ref_.id ())));
  EXPECT_FALSE (
    json_contains_uuid (arranger_bucket, to_quuid (audio_clip_ref_.id ())));
  EXPECT_FALSE (
    json_contains_uuid (arranger_bucket, to_quuid (audio_source_ref_.id ())));
  EXPECT_EQ (
    filtered.payload->registry_json ()
      .at (ProjectRegistry::kFileAudioSourcesKey)
      .size (),
    0u);

  // one audio clip and its audio source object were dropped
  EXPECT_EQ (filtered.dropped_audio_objects, 2u);

  ASSERT_EQ (filtered.payload->roots ().size (), 1u);
  EXPECT_EQ (
    filtered.payload->roots ().front (), to_quuid (midi_clip_ref_.id ()));
}

TEST_F (ClipboardPayloadTest, FilteredForTargetCrossProjectAllAudioIsRejected)
{
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::ArrangerObjects,
    { to_quuid (audio_clip_ref_.id ()) }, QStringLiteral ("project-a"));
  EXPECT_FALSE (
    payload.filtered_for_target (target_registry_).payload.has_value ());
}

// A file audio source the target already has registered (loaded from its
// pool) makes the depending audio content pasteable, whichever project
// the payload came from
TEST_F (ClipboardPayloadTest, FilteredForTargetKeepsAudioSharedWithTarget)
{
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::ArrangerObjects,
    { to_quuid (audio_clip_ref_.id ()) }, QStringLiteral ("project-a"));
  const auto remapped = ClipboardPayload::with_regenerated_uuids (payload);
  ASSERT_EQ (remapped.import_into (target_registry_).size (), 3u);

  const auto later = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::ArrangerObjects,
    { to_quuid (audio_clip_ref_.id ()) }, QStringLiteral ("project-a"));
  const auto filtered = later.filtered_for_target (target_registry_);

  ASSERT_TRUE (filtered.payload.has_value ());
  EXPECT_EQ (filtered.dropped_audio_objects, 0u);
  const auto &arranger_bucket = filtered.payload->registry_json ().at (
    ProjectRegistry::kArrangerObjectsKey);
  EXPECT_TRUE (
    json_contains_uuid (arranger_bucket, to_quuid (audio_clip_ref_.id ())));
  EXPECT_TRUE (
    json_contains_uuid (arranger_bucket, to_quuid (audio_source_ref_.id ())));
  EXPECT_EQ (
    filtered.payload->registry_json ()
      .at (ProjectRegistry::kFileAudioSourcesKey)
      .size (),
    1u);
}

TEST_F (ClipboardPayloadTest, ImportIntoRegistersObjects)
{
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::ArrangerObjects,
    { to_quuid (midi_clip_ref_.id ()), to_quuid (marker_ref_.id ()) },
    QStringLiteral ("project-a"));
  const auto remapped = ClipboardPayload::with_regenerated_uuids (payload);
  remapped.import_into (target_registry_);

  ASSERT_EQ (remapped.roots ().size (), 2u);
  const auto new_clip_id = remapped.roots ()[0];
  const auto new_marker_id = remapped.roots ()[1];
  EXPECT_TRUE (target_registry_.contains (new_clip_id));
  EXPECT_TRUE (target_registry_.contains (new_marker_id));

  // the imported clip owns two notes with the source positions
  auto * clip = qobject_cast<arrangement::MidiClip *> (
    target_registry_.find_by_raw_uuid (new_clip_id));
  ASSERT_NE (clip, nullptr);
  EXPECT_EQ (clip->position ()->ticks (), 2000.0);
  EXPECT_EQ (clip->length ()->ticks (), 4000.0);
  const auto notes =
    clip->arrangement::ArrangerObjectOwner<
      arrangement::MidiNote>::get_sorted_children_view ()
    | std::ranges::to<std::vector> ();
  ASSERT_EQ (notes.size (), 2u);
  EXPECT_EQ (notes[0]->position ()->ticks (), 100.0);
  EXPECT_EQ (notes[0]->length ()->ticks (), 400.0);
  EXPECT_EQ (notes[1]->position ()->ticks (), 600.0);

  auto * marker = qobject_cast<arrangement::Marker *> (
    target_registry_.find_by_raw_uuid (new_marker_id));
  ASSERT_NE (marker, nullptr);
  EXPECT_EQ (marker->position ()->ticks (), 5000.0);
}

TEST_F (ClipboardPayloadTest, ImportIntoSameRegistrySharesFileAudioSources)
{
  // Simulate a same-project paste: import into the source registry itself.
  // The file audio source must be shared, not duplicated.
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::ArrangerObjects,
    { to_quuid (audio_clip_ref_.id ()) }, QStringLiteral ("project-a"));
  const auto remapped = ClipboardPayload::with_regenerated_uuids (payload);
  remapped.import_into (source_registry_);

  const auto new_clip_id = remapped.roots ().front ();
  EXPECT_TRUE (source_registry_.contains (new_clip_id));
  EXPECT_EQ (source_registry_.count_matching<dsp::FileAudioSource> (), 1u);

  auto * clip = qobject_cast<arrangement::AudioClip *> (
    source_registry_.find_by_raw_uuid (new_clip_id));
  ASSERT_NE (clip, nullptr);
  const auto sources =
    clip->arrangement::ArrangerObjectOwner<
      arrangement::AudioSourceObject>::get_sorted_children_view ()
    | std::ranges::to<std::vector> ();
  ASSERT_EQ (sources.size (), 1u);
  EXPECT_EQ (
    type_safe::get (sources[0]->audio_source_ref ().id ()),
    to_quuid (fas_ref_.id ()));
}

TEST_F (ClipboardPayloadTest, DeleteObjectsInAnyOrderHandlesReferences)
{
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::ArrangerObjects,
    { to_quuid (audio_clip_ref_.id ()) }, QStringLiteral ("project-a"));
  const auto remapped = ClipboardPayload::with_regenerated_uuids (payload);
  remapped.import_into (target_registry_);

  // Collect every imported id, listing the file audio sources (referenced
  // by the audio source objects) and each bucket's children before their
  // parents
  std::vector<QUuid> ids;
  for (const auto &[bucket_key, bucket] : remapped.registry_json ().items ())
    {
      for (auto it = bucket.rbegin (); it != bucket.rend (); ++it)
        {
          const auto id = QUuid::fromString (
            QString::fromStdString (it->at ("id").get<std::string> ()));
          if (bucket_key == ProjectRegistry::kFileAudioSourcesKey)
            ids.insert (ids.begin (), id);
          else
            ids.push_back (id);
        }
    }
  ASSERT_FALSE (ids.empty ());

  target_registry_.delete_objects_in_any_order (ids);

  for (const auto &id : ids)
    EXPECT_FALSE (target_registry_.contains (id));
  EXPECT_EQ (target_registry_.count_matching<dsp::FileAudioSource> (), 0u);
}

TEST_F (ClipboardPayloadTest, IdsNeededByRootsReturnsClosureOfGivenRoots)
{
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::ArrangerObjects,
    { to_quuid (midi_clip_ref_.id ()), to_quuid (marker_ref_.id ()) },
    QStringLiteral ("project-a"));

  const auto needed =
    payload.ids_needed_by_roots ({ to_quuid (midi_clip_ref_.id ()) });

  const auto is_needed = [&needed] (const auto &ref) {
    return std::ranges::find (needed, to_quuid (ref.id ())) != needed.end ();
  };
  // The clip's closure (its notes) is needed
  EXPECT_TRUE (is_needed (midi_clip_ref_));
  EXPECT_TRUE (is_needed (note1_ref_));
  EXPECT_TRUE (is_needed (note2_ref_));
  // The other root is not
  EXPECT_FALSE (is_needed (marker_ref_));
}

TEST_F (ClipboardPayloadTest, IdsNeededByRootsIncludesBoundaryReferences)
{
  // Both the parameter (modulated by the CV port) and the port itself are
  // copied roots
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::Plugins,
    { to_quuid (param_ref_.id ()), to_quuid (cv_port_ref_.id ()) },
    QStringLiteral ("project-a"));

  // The boundary reference must be followed so that discarding skipped
  // roots never severs a kept parameter's modulation source
  const auto needed =
    payload.ids_needed_by_roots ({ to_quuid (param_ref_.id ()) });
  EXPECT_NE (
    std::ranges::find (needed, to_quuid (cv_port_ref_.id ())), needed.end ());
}

// The serialized "variantType" of each entry is its index in
// ArrangerObjectVariant, and the embedded schema pins the same numbers as
// consts: this test fails if the variant is reordered without updating the
// schema (or the other way round)
TEST_F (ClipboardPayloadTest, EmittedVariantTypesMatchEmbeddedSchema)
{
  const auto chord_object_ref = utils::create_object<arrangement::ChordObject> (
    source_registry_, *tempo_map_wrapper_);
  const auto scale_object_ref = utils::create_object<arrangement::ScaleObject> (
    source_registry_, *tempo_map_wrapper_);
  const auto chord_clip_ref = utils::create_object<arrangement::ChordClip> (
    source_registry_, *tempo_map_wrapper_, source_registry_);
  const auto automation_clip_ref =
    utils::create_object<arrangement::AutomationClip> (
      source_registry_, *tempo_map_wrapper_, source_registry_);
  const auto automation_point_ref = utils::create_object<
    arrangement::AutomationPoint> (source_registry_, *tempo_map_wrapper_);
  const auto tempo_object_ref = utils::create_object<arrangement::TempoObject> (
    source_registry_, *tempo_map_wrapper_);
  const auto time_signature_object_ref = utils::create_object<
    arrangement::TimeSignatureObject> (source_registry_, *tempo_map_wrapper_);

  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::ArrangerObjects,
    { to_quuid (midi_clip_ref_.id ()), to_quuid (audio_clip_ref_.id ()),
      to_quuid (chord_clip_ref.id ()), to_quuid (automation_clip_ref.id ()),
      to_quuid (automation_point_ref.id ()), to_quuid (marker_ref_.id ()),
      to_quuid (audio_source_ref_.id ()), to_quuid (tempo_object_ref.id ()),
      to_quuid (time_signature_object_ref.id ()),
      to_quuid (chord_object_ref.id ()), to_quuid (scale_object_ref.id ()),
      to_quuid (note1_ref_.id ()), to_quuid (cc_event_ref_.id ()) },
    QStringLiteral ("project-a"));

  std::set<std::size_t> emitted_types;
  for (
    const auto &entry :
    payload.registry_json ().at (ProjectRegistry::kArrangerObjectsKey))
    {
      emitted_types.insert (
        entry.at (utils::serialization::kVariantTypeKey).get<std::size_t> ());
    }

  const auto schema = nlohmann::json::parse (kClipboardSchemaJsonStr);
  static constexpr std::array<std::pair<std::string_view, std::size_t>, 13> schema_definitions{
    {
     { "midiNote", 0 },
     { "chordObject", 1 },
     { "scaleObject", 2 },
     { "midiClip", 3 },
     { "audioClip", 4 },
     { "chordClip", 5 },
     { "automationClip", 6 },
     { "automationPoint", 7 },
     { "marker", 8 },
     { "audioSourceObject", 9 },
     { "tempoObject", 10 },
     { "timeSignatureObject", 11 },
     { "midiControlEvent", 12 },
     }
  };
  for (const auto &[definition_name, variant_index] : schema_definitions)
    {
      EXPECT_EQ (
        schema.at ("definitions")
          .at (std::string{ definition_name })
          .at ("allOf")
          .at (1)
          .at ("properties")
          .at ("variantType")
          .at ("const")
          .get<std::size_t> (),
        variant_index)
        << definition_name;
      EXPECT_TRUE (emitted_types.contains (variant_index)) << definition_name;
    }
  EXPECT_EQ (emitted_types.size (), schema_definitions.size ());
}

} // namespace zrythm::structure::project
