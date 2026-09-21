// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <set>

#include "dsp/cv_port.h"
#include "dsp/file_audio_source.h"
#include "dsp/parameter.h"
#include "plugins/faust/faust_plugin.h"
#include "plugins/plugin_configuration.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/plugin_factory.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/arrangement/audio_clip.h"
#include "structure/arrangement/audio_source_object.h"
#include "structure/arrangement/automation_clip.h"
#include "structure/arrangement/automation_point.h"
#include "structure/arrangement/chord_clip.h"
#include "structure/arrangement/chord_object.h"
#include "structure/arrangement/marker.h"
#include "structure/arrangement/midi_clip.h"
#include "structure/arrangement/midi_control_event.h"
#include "structure/arrangement/midi_note.h"
#include "structure/arrangement/scale_object.h"
#include "structure/arrangement/tempo_object.h"
#include "structure/arrangement/time_signature_object.h"
#include "structure/project/clipboard_json_schema.h"
#include "structure/project/clipboard_payload.h"
#include "structure/tracks/audio_bus_track.h"
#include "structure/tracks/audio_group_track.h"
#include "structure/tracks/audio_track.h"
#include "structure/tracks/channel_send.h"
#include "structure/tracks/chord_track.h"
#include "structure/tracks/folder_track.h"
#include "structure/tracks/instrument_track.h"
#include "structure/tracks/marker_track.h"
#include "structure/tracks/master_track.h"
#include "structure/tracks/midi_bus_track.h"
#include "structure/tracks/midi_group_track.h"
#include "structure/tracks/midi_track.h"
#include "structure/tracks/modulator_track.h"
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

class ClipboardPayloadTestBase
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
};

class ClipboardPayloadTest : public ClipboardPayloadTestBase
{
protected:
  void SetUp () override
  {
    ClipboardPayloadTestBase::SetUp ();

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

  const auto needed = payload.ids_needed_by_roots (
    std::vector<QUuid>{ to_quuid (midi_clip_ref_.id ()) });

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
  const auto needed = payload.ids_needed_by_roots (
    std::vector<QUuid>{ to_quuid (param_ref_.id ()) });
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

// A track payload's routing metadata may point at tracks outside the
// payload (e.g. the master track): it must survive the clipboard text
// round-trip instead of being rejected as a dangling reference
TEST_F (ClipboardPayloadTest, OutOfSetRoutingTargetDecodes)
{
  nlohmann::json metadata = nlohmann::json::object (
    {
      { std::string (ClipboardPayload::kRoutingMetadataKey),
       nlohmann::json::array (
          { nlohmann::json{
            { std::string (ClipboardPayload::kRoutingSourceMetadataKey),
              to_quuid (marker_ref_.id ())
                .toString (QUuid::WithoutBraces)
                .toStdString () },
            { std::string (ClipboardPayload::kRoutingTargetMetadataKey),
              QUuid::createUuid ()
                .toString (QUuid::WithoutBraces)
                .toStdString () } } }) }
  });

  auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::Tracks,
    { to_quuid (marker_ref_.id ()) }, QStringLiteral ("project-a"),
    std::move (metadata));

  const auto decoded = ClipboardPayload::decode_from_clipboard_text (
    payload.encode_to_clipboard_text ());
  ASSERT_TRUE (decoded.has_value ());
  const auto &routing = decoded->metadata ().at (
    std::string (ClipboardPayload::kRoutingMetadataKey));
  EXPECT_EQ (routing.size (), 1);
}

TEST_F (ClipboardPayloadTest, FilterSeversUnresolvableRoutingTarget)
{
  nlohmann::json metadata = nlohmann::json::object (
    {
      { std::string (ClipboardPayload::kRoutingMetadataKey),
       nlohmann::json::array (
          { nlohmann::json{
            { std::string (ClipboardPayload::kRoutingSourceMetadataKey),
              to_quuid (marker_ref_.id ())
                .toString (QUuid::WithoutBraces)
                .toStdString () },
            { std::string (ClipboardPayload::kRoutingTargetMetadataKey),
              QUuid::createUuid ()
                .toString (QUuid::WithoutBraces)
                .toStdString () } } }) }
  });

  auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::Tracks,
    { to_quuid (marker_ref_.id ()) }, QStringLiteral ("project-a"),
    std::move (metadata));

  const auto filtered = payload.filtered_for_target (target_registry_);
  ASSERT_TRUE (filtered.payload.has_value ());
  EXPECT_GE (filtered.severed_references, 1);
  const auto &routing = filtered.payload->metadata ().at (
    std::string (ClipboardPayload::kRoutingMetadataKey));
  ASSERT_EQ (routing.size (), 1);
  // The severed target becomes null (no route) instead of dangling
  EXPECT_TRUE (
    routing.at (0)
      .at (std::string (ClipboardPayload::kRoutingTargetMetadataKey))
      .is_null ());
}

// ==========================================================================
// Closure property test
//
// The clipboard's closure walk rests on one invariant: every reference
// between registry objects is serialized as a bare UUID string the walk
// can find and follow. These tests populate a project with every object
// type, copy it, regenerate the UUIDs and import into a fresh registry;
// the resolution test then walks the re-serialized objects with its own
// UUID scanner instead of reusing the production walk, so a new kind of
// reference the walk does not know about shows up as a dangling UUID.
// ==========================================================================

class ClipboardPayloadClosureTest : public ClipboardPayloadTestBase
{
protected:
  /** Object IDs of the populated project: what the selection roots are
   * and everything that must come along in the payload.
   *
   * Registry references are owning: the keep-alive members hold one
   * reference per created object so the population outlives this
   * function's locals. */
  struct Population
  {
    explicit Population (utils::IObjectRegistry &registry)
        : plugin_keep_alive_ (registry), cv_keep_alive_ (registry),
          param_keep_alive_ (registry), fas_keep_alive_ (registry)
    {
    }

    std::vector<QUuid> roots_;
    std::vector<QUuid> all_ids_;
    QUuid              fas_id_;

    std::vector<structure::tracks::TrackUuidReference> track_keep_alive_refs_;
    std::vector<arrangement::ArrangerObjectUuidReference>
                                           arranger_object_keep_alive_refs_;
    plugins::PluginUuidReference           plugin_keep_alive_;
    utils::TypedUuidReference<dsp::CVPort> cv_keep_alive_;
    dsp::ProcessorParameterUuidReference   param_keep_alive_;
    dsp::FileAudioSourceUuidReference      fas_keep_alive_;
  };

  template <typename T> auto add_track (Population &pop)
  {
    auto       ref = track_factory_->get_builder<T> ().build ();
    const auto id = to_quuid (ref.id ());
    pop.roots_.push_back (id);
    pop.all_ids_.push_back (id);
    if (const auto * lanes = ref.get ()->lanes ())
      {
        for (const auto &lane_ref : lanes->lanes ())
          pop.all_ids_.push_back (to_quuid (lane_ref.get ()->get_uuid ()));
      }
    pop.track_keep_alive_refs_.push_back (ref);
    return ref;
  }

  Population populate_every_object_type ()
  {
    Population pop (source_registry_);

    // One of every track type
    const auto audio_track = add_track<structure::tracks::AudioTrack> (pop);
    const auto midi_track = add_track<structure::tracks::MidiTrack> (pop);
    add_track<structure::tracks::InstrumentTrack> (pop);
    add_track<structure::tracks::MasterTrack> (pop);
    const auto chord_track = add_track<structure::tracks::ChordTrack> (pop);
    add_track<structure::tracks::MarkerTrack> (pop);
    add_track<structure::tracks::ModulatorTrack> (pop);
    const auto audio_bus = add_track<structure::tracks::AudioBusTrack> (pop);
    add_track<structure::tracks::MidiBusTrack> (pop);
    add_track<structure::tracks::AudioGroupTrack> (pop);
    add_track<structure::tracks::MidiGroupTrack> (pop);
    add_track<structure::tracks::FolderTrack> (pop);

    // MIDI content: clip with notes and a control event on the MIDI track
    const auto midi_clip = utils::create_object<arrangement::MidiClip> (
      source_registry_, *tempo_map_wrapper_, source_registry_);
    const auto note = utils::create_object<arrangement::MidiNote> (
      source_registry_, *tempo_map_wrapper_);
    const auto cc_event = utils::create_object<arrangement::MidiControlEvent> (
      source_registry_, *tempo_map_wrapper_);
    midi_clip.get_object_as<arrangement::MidiClip> ()->arrangement::
      ArrangerObjectOwner<arrangement::MidiNote>::add_object (note);
    midi_clip.get_object_as<arrangement::MidiClip> ()->arrangement::
      ArrangerObjectOwner<arrangement::MidiControlEvent>::add_object (cc_event);
    midi_track.get_object_as<structure::tracks::MidiTrack> ()
      ->lanes ()
      ->at (0)
      ->arrangement::ArrangerObjectOwner<arrangement::MidiClip>::add_object (
        midi_clip);

    // Audio content: file audio source -> audio source object -> audio
    // clip on the audio track
    const auto fas = utils::create_object<dsp::FileAudioSource> (
      source_registry_, utils::audio::AudioBuffer (2, 64),
      dsp::FileAudioSource::BitDepth::BIT_DEPTH_16, units::sample_rate (44100),
      units::bpm (120.0),
      utils::Utf8String::from_utf8_encoded_string ("closure"));
    const auto audio_source =
      utils::create_object<arrangement::AudioSourceObject> (
        source_registry_, *tempo_map_wrapper_, source_registry_, fas);
    const auto audio_clip = utils::create_object<arrangement::AudioClip> (
      source_registry_, *tempo_map_wrapper_, source_registry_);
    audio_clip.get_object_as<arrangement::AudioClip> ()
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioSourceObject>::add_object (audio_source);
    audio_track.get_object_as<structure::tracks::AudioTrack> ()
      ->lanes ()
      ->at (0)
      ->arrangement::ArrangerObjectOwner<arrangement::AudioClip>::add_object (
        audio_clip);
    pop.fas_id_ = to_quuid (fas.id ());

    // Automation on the audio track
    const auto automation_clip =
      utils::create_object<arrangement::AutomationClip> (
        source_registry_, *tempo_map_wrapper_, source_registry_);
    const auto automation_point = utils::create_object<
      arrangement::AutomationPoint> (source_registry_, *tempo_map_wrapper_);
    automation_clip.get_object_as<arrangement::AutomationClip> ()
      ->arrangement::ArrangerObjectOwner<
        arrangement::AutomationPoint>::add_object (automation_point);
    audio_track.get_object_as<structure::tracks::AudioTrack> ()
      ->automationTracklist ()
      ->automation_track_at (0)
      ->add_object (automation_clip);

    // Chords on the chord track
    const auto chord_clip = utils::create_object<arrangement::ChordClip> (
      source_registry_, *tempo_map_wrapper_, source_registry_);
    const auto chord_object = utils::create_object<arrangement::ChordObject> (
      source_registry_, *tempo_map_wrapper_);
    chord_clip.get_object_as<arrangement::ChordClip> ()->arrangement::
      ArrangerObjectOwner<arrangement::ChordObject>::add_object (chord_object);
    chord_track.get_object_as<structure::tracks::ChordTrack> ()->arrangement::
      ArrangerObjectOwner<arrangement::ChordClip>::add_object (chord_clip);

    // A plugin on the audio bus channel
    auto plugin_ref = utils::create_object<plugins::FaustPlugin> (
      source_registry_, source_registry_);
    auto descr = std::make_unique<plugins::PluginDescriptor> ();
    descr->name_ = u8"Closure Test Plugin";
    descr->protocol_ = plugins::Protocol::ProtocolType::Internal;
    auto config = std::make_unique<plugins::PluginConfiguration> ();
    config->descr_ = std::move (descr);
    plugin_ref.get ()->set_configuration (*config);
    audio_bus.get_object_as<structure::tracks::AudioBusTrack> ()
      ->channel ()
      ->inserts ()
      ->insert_plugin (plugin_ref);

    // Objects not owned by a track are their own roots
    const auto marker = utils::create_object<arrangement::Marker> (
      source_registry_, *tempo_map_wrapper_,
      arrangement::Marker::MarkerType::Custom);
    const auto tempo_object = utils::create_object<arrangement::TempoObject> (
      source_registry_, *tempo_map_wrapper_);
    const auto time_signature_object = utils::create_object<
      arrangement::TimeSignatureObject> (source_registry_, *tempo_map_wrapper_);
    const auto scale_object = utils::create_object<arrangement::ScaleObject> (
      source_registry_, *tempo_map_wrapper_);

    // A parameter modulated by a copied CV port: the modulation source
    // key is a boundary reference, and here it points into the payload
    const auto cv_port = utils::create_object<dsp::CVPort> (
      source_registry_, u8"closure-mod", dsp::PortFlow::Output);
    dsp::ProcessorParameterUuidReference param_ref{ source_registry_ };
    {
      auto param = std::make_unique<dsp::ProcessorParameter> (
        source_registry_, dsp::ProcessorParameter::UniqueId{},
        dsp::ParameterRange{}, utils::Utf8String{});
      source_registry_.register_object (*param);
      param_ref = dsp::ProcessorParameterUuidReference (
        param->get_uuid (), source_registry_);
      param.release ();

      nlohmann::json param_json = *param_ref.get ();
      param_json[dsp::ProcessorParameter::kModulationSourcePortIdKey] = cv_port;
      param_json.get_to (*param_ref.get ());
    }

    const auto trackless_ids = {
      to_quuid (midi_clip.id ()),       to_quuid (note.id ()),
      to_quuid (cc_event.id ()),        to_quuid (fas.id ()),
      to_quuid (audio_source.id ()),    to_quuid (audio_clip.id ()),
      to_quuid (automation_clip.id ()), to_quuid (automation_point.id ()),
      to_quuid (chord_clip.id ()),      to_quuid (chord_object.id ()),
      to_quuid (plugin_ref.id ()),      to_quuid (marker.id ()),
      to_quuid (tempo_object.id ()),    to_quuid (time_signature_object.id ()),
      to_quuid (scale_object.id ()),    to_quuid (cv_port.id ()),
      to_quuid (param_ref.id ()),
    };
    pop.all_ids_.insert (pop.all_ids_.end (), trackless_ids);
    // TypedUuidReference<T> constructs from any TypedUuidReference<U> with
    // U derived from T, so each push converts to the keep-alive element type
    pop.arranger_object_keep_alive_refs_.push_back (midi_clip);
    pop.arranger_object_keep_alive_refs_.push_back (note);
    pop.arranger_object_keep_alive_refs_.push_back (cc_event);
    pop.arranger_object_keep_alive_refs_.push_back (audio_source);
    pop.arranger_object_keep_alive_refs_.push_back (audio_clip);
    pop.arranger_object_keep_alive_refs_.push_back (automation_clip);
    pop.arranger_object_keep_alive_refs_.push_back (automation_point);
    pop.arranger_object_keep_alive_refs_.push_back (chord_clip);
    pop.arranger_object_keep_alive_refs_.push_back (chord_object);
    pop.arranger_object_keep_alive_refs_.push_back (marker);
    pop.arranger_object_keep_alive_refs_.push_back (tempo_object);
    pop.arranger_object_keep_alive_refs_.push_back (time_signature_object);
    pop.arranger_object_keep_alive_refs_.push_back (scale_object);
    pop.plugin_keep_alive_ = plugin_ref;
    pop.cv_keep_alive_ = cv_port;
    pop.param_keep_alive_ = param_ref;
    pop.fas_keep_alive_ = fas;
    pop.roots_.insert (
      pop.roots_.end (),
      { to_quuid (marker.id ()), to_quuid (tempo_object.id ()),
        to_quuid (time_signature_object.id ()), to_quuid (scale_object.id ()),
        to_quuid (cv_port.id ()), to_quuid (param_ref.id ()) });
    return pop;
  }

  static std::size_t payload_entry_count (const ClipboardPayload &payload)
  {
    std::size_t count = 0;
    for (const auto &[_, bucket] : payload.registry_json ().items ())
      count += bucket.size ();
    return count;
  }

  /** Keys whose values may point outside the copied subgraph by design
   * (severed or remapped at paste time). */
  static constexpr std::array<std::string_view, 2> kBoundaryKeys{
    dsp::ProcessorParameter::kModulationSourcePortIdKey,
    structure::tracks::ChannelSend::kDestinationPortKey,
  };

  /** Every UUID-shaped string in @p j must resolve in @p registry,
   * except under boundary keys. */
  static void expect_uuid_strings_resolve (
    const nlohmann::json  &j,
    const ProjectRegistry &registry,
    const std::string     &path)
  {
    if (j.is_string ())
      {
        const auto id =
          QUuid::fromString (QString::fromStdString (j.get<std::string> ()));
        if (!id.isNull ())
          {
            EXPECT_TRUE (registry.contains (id))
              << path << " references unregistered "
              << id.toString (QUuid::WithoutBraces).toStdString ();
          }
        return;
      }
    if (j.is_object ())
      {
        for (const auto &[key, value] : j.items ())
          {
            if (
              std::ranges::find (kBoundaryKeys, std::string_view{ key })
              != kBoundaryKeys.end ())
              continue;
            expect_uuid_strings_resolve (value, registry, path + "/" + key);
          }
        return;
      }
    if (j.is_array ())
      {
        for (std::size_t i = 0; i < j.size (); ++i)
          expect_uuid_strings_resolve (
            j.at (i), registry, path + "/" + std::to_string (i));
      }
  }
};

// A track payload carries lanes: the track's lane references and the lane
// entries must resolve within the payload, so the text form decodes
TEST_F (ClipboardPayloadClosureTest, TrackPayloadWithLanesDecodes)
{
  auto track =
    track_factory_->get_builder<structure::tracks::AudioTrack> ().build ();
  const auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::Tracks,
    { to_quuid (track.id ()) }, QStringLiteral ("x"));
  const auto decoded = ClipboardPayload::decode_from_clipboard_text (
    payload.encode_to_clipboard_text ());
  ASSERT_TRUE (decoded.has_value ());
  EXPECT_TRUE (json_contains_uuid (
    decoded->registry_json (),
    to_quuid (track.get ()->lanes ()->at (0)->get_uuid ())));
}

TEST_F (ClipboardPayloadClosureTest, CreateCollectsClosureOfEveryObjectType)
{
  const auto pop = populate_every_object_type ();

  auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::Tracks, pop.roots_,
    QStringLiteral ("closure-source"));

  for (const auto &id : pop.all_ids_)
    EXPECT_TRUE (json_contains_uuid (payload.registry_json (), id))
      << "missing from payload: "
      << id.toString (QUuid::WithoutBraces).toStdString ();
  EXPECT_GE (payload_entry_count (payload), pop.all_ids_.size ());
  EXPECT_TRUE (payload.references_resolve_internally ());
  const auto entry_count = payload_entry_count (payload);

  // UUID regeneration keeps the closure shape: same entry count, the
  // shared file audio source keeps its identity and every other object
  // gets a fresh UUID
  auto regenerated =
    ClipboardPayload::with_regenerated_uuids (std::move (payload));
  EXPECT_EQ (payload_entry_count (regenerated), entry_count);
  EXPECT_TRUE (json_contains_uuid (regenerated.registry_json (), pop.fas_id_));
  for (const auto &id : pop.all_ids_)
    if (id != pop.fas_id_)
      EXPECT_FALSE (json_contains_uuid (regenerated.registry_json (), id))
        << "stale UUID after regeneration: "
        << id.toString (QUuid::WithoutBraces).toStdString ();
}

// Importing goes through the same pipeline as pasting: external references
// are filtered against the target registry before import_into() sees them
TEST_F (ClipboardPayloadClosureTest, ImportIntoFreshRegistryResolvesAllReferences)
{
  const auto pop = populate_every_object_type ();

  auto payload = ClipboardPayload::create (
    source_registry_, ClipboardPayload::Type::Tracks, pop.roots_,
    QStringLiteral ("closure-source"));
  auto regenerated =
    ClipboardPayload::with_regenerated_uuids (std::move (payload));

  const auto  filter = regenerated.filtered_for_target (target_registry_);
  const auto &filtered = filter.payload;
  ASSERT_TRUE (filtered.has_value ());

  const auto imported = filtered->import_into (target_registry_);
  EXPECT_EQ (imported.size (), payload_entry_count (*filtered));

  for (const auto &id : imported)
    {
      nlohmann::json object_json;
      const auto     category =
        target_registry_.serialize_object_by_uuid (id, object_json);
      ASSERT_TRUE (category.has_value ())
        << "imported id not registered: "
        << id.toString (QUuid::WithoutBraces).toStdString ();
      expect_uuid_strings_resolve (
        object_json, target_registry_,
        id.toString (QUuid::WithoutBraces).toStdString ());
    }
}

} // namespace zrythm::structure::project
