// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map_qml_adapter.h"
#include "structure/tracks/track.h"
#include "utils/views.h"

namespace zrythm::structure::tracks
{
Track::~Track () = default;

Track::Track (
  Type                  type,
  PortType              in_signal_type,
  PortType              out_signal_type,
  TrackFeatures         enabled_features,
  BaseTrackDependencies dependencies)
    : base_dependencies_ (std::move (dependencies)), type_ (type),
      features_ (enabled_features), in_signal_type_ (in_signal_type),
      out_signal_type_ (out_signal_type)
{
  z_debug ("creating {} track", type);

  if (
    out_signal_type == dsp::PortType::Audio
    || out_signal_type == dsp::PortType::Midi)
    {
      channel_ = make_channel ();
    }
  if (ENUM_BITSET_TEST (enabled_features, TrackFeatures::Modulators))
    {
      modulators_ = make_modulators ();
    }
  if (ENUM_BITSET_TEST (enabled_features, TrackFeatures::Automation))
    {
      automation_tracklist_ = make_automation_tracklist ();
      generate_basic_automation_tracks ();
    }
  if (ENUM_BITSET_TEST (enabled_features, TrackFeatures::Lanes))
    {
      lanes_ = make_lanes ();
    }
  if (ENUM_BITSET_TEST (enabled_features, TrackFeatures::Recording))
    {
      recordable_track_mixin_ = make_recordable_track_mixin ();
    }
  if (ENUM_BITSET_TEST (enabled_features, TrackFeatures::PianoRoll))
    {
      piano_roll_track_mixin_ = make_piano_roll_track_mixin ();
    }

  // Listen to height changes
  QObject::connect (
    this, &Track::heightChanged, this, &Track::fullVisibleHeightChanged);
}

void
init_from (Track &obj, const Track &other, utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Track::UuidIdentifiableObject &> (obj),
    static_cast<const Track::UuidIdentifiableObject &> (other), clone_type);
  obj.type_ = other.type_;
  obj.name_ = other.name_;
  obj.icon_name_ = other.icon_name_;
  obj.visible_ = other.visible_;
  obj.main_height_ = other.main_height_;
  obj.enabled_ = other.enabled_;
  obj.color_ = other.color_;
  obj.in_signal_type_ = other.in_signal_type_;
  obj.out_signal_type_ = other.out_signal_type_;
  obj.comment_ = other.comment_;
  obj.frozen_clip_id_ = other.frozen_clip_id_;
  init_from (*obj.processor_, *other.processor_, clone_type);
  init_from (
    *obj.automation_tracklist_, *other.automation_tracklist_, clone_type);
  utils::clone_unique_ptr_container (
    obj.modulator_macro_processors_, other.modulator_macro_processors_);
}

utils::Utf8String
Track::get_full_designation_for_port (const dsp::Port &port) const
{
  return utils::Utf8String::from_utf8_encoded_string (
    fmt::format ("{}/{}", get_name (), port.get_label ()));
}

Track::Type
Track::type_get_from_plugin_descriptor (
  const zrythm::plugins::PluginDescriptor &descr)
{
  if (descr.isInstrument ())
    return Track::Type::Instrument;
  else if (descr.isMidiModifier ())
    return Track::Type::Midi;
  else
    return Track::Type::AudioBus;
}

uint8_t
Track::get_midi_ch (const arrangement::MidiRegion &midi_region) const
{
  assert (lanes_);
  assert (piano_roll_track_mixin_);

  // return lane MIDI channel if set
  for (const auto &lane : lanes_->lanes_view ())
    {
      auto midi_regions = lane->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_view ();
      auto it = std::ranges::find (midi_regions, &midi_region);
      if (it != midi_regions.end () && lane->midiChannel () > 0)
        {
          return lane->midiChannel ();
        }
    }

  // otherwise return piano roll's channel
  return piano_roll_track_mixin_->midiChannel ();
}

utils::QObjectUniquePtr<TrackProcessor>
Track::make_track_processor (
  std::optional<TrackProcessor::FillEventsCallback> fill_events_cb,
  std::optional<TrackProcessor::TransformMidiInputsFunc>
    transform_midi_inputs_func,
  std::optional<TrackProcessor::AppendMidiInputsToOutputsFunc>
    append_midi_inputs_to_outputs_func)
{
  return utils::make_qobject_unique<TrackProcessor> (
    base_dependencies_.tempo_map_.get_tempo_map (), in_signal_type_,
    [this] () { return get_name (); }, [this] () { return enabled (); },
    has_piano_roll () || is_chord (), has_piano_roll (), is_audio (),
    TrackProcessor::ProcessorBaseDependencies{
      .port_registry_ = base_dependencies_.port_registry_,
      .param_registry_ = base_dependencies_.param_registry_ },
    fill_events_cb, transform_midi_inputs_func,
    append_midi_inputs_to_outputs_func, this);
}

utils::QObjectUniquePtr<AutomationTracklist>
Track::make_automation_tracklist ()
{
  auto atl = utils::make_qobject_unique<AutomationTracklist> (
    AutomationTrackHolder::Dependencies{
      .tempo_map_ = base_dependencies_.tempo_map_,
      .file_audio_source_registry_ =
        base_dependencies_.file_audio_source_registry_,
      .port_registry_ = base_dependencies_.port_registry_,
      .param_registry_ = base_dependencies_.param_registry_,
      .object_registry_ = base_dependencies_.obj_registry_ },
    this);

  // Listen to height changes
  QObject::connect (
    atl.get (), &AutomationTracklist::automationVisibleChanged, this,
    &Track::fullVisibleHeightChanged);

  return atl;
}

utils::QObjectUniquePtr<Channel>
Track::make_channel ()
{
  return utils::make_qobject_unique<Channel> (
    get_plugin_registry (),
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = get_port_registry (),
      .param_registry_ = get_param_registry () },
    out_signal_type_, [&] () { return get_name (); }, is_master (),
    [this] (bool fader_solo_status) {
      // Effectively muted if other track(s) is soloed and this isn't
      return base_dependencies_.soloed_tracks_exist_getter_ ()
             && !fader_solo_status && !is_master ();
    },
    this);
}

utils::QObjectUniquePtr<plugins::PluginGroup>
Track::make_modulators ()
{
  return utils::make_qobject_unique<plugins::PluginGroup> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = base_dependencies_.port_registry_,
      .param_registry_ = base_dependencies_.param_registry_ },
    base_dependencies_.plugin_registry_,
    plugins::PluginGroup::DeviceGroupType::CV,
    plugins::PluginGroup::ProcessingTypeHint::Custom, this);
}

utils::QObjectUniquePtr<TrackLaneList>
Track::make_lanes ()
{
  auto ret = utils::make_qobject_unique<TrackLaneList> (
    base_dependencies_.obj_registry_,
    base_dependencies_.file_audio_source_registry_, this);
  ret->create_missing_lanes (0);

  playable_content_cache_request_debouncer_ =
    utils::make_qobject_unique<utils::PlaybackCacheScheduler> (this);

  QObject::connect (
    ret.get (), &TrackLaneList::laneObjectsNeedRecache,
    playable_content_cache_request_debouncer_.get (),
    &utils::PlaybackCacheScheduler::queueCacheRequest);
  QObject::connect (
    &base_dependencies_.tempo_map_, &dsp::TempoMapWrapper::tempoEventsChanged,
    playable_content_cache_request_debouncer_.get (), [this] () {
      playable_content_cache_request_debouncer_->queueCacheRequest ({});
    });

  QObject::connect (
    playable_content_cache_request_debouncer_.get (),
    &utils::PlaybackCacheScheduler::cacheRequested, this,
    &Track::regeneratePlaybackCaches);

  // Listen to height changes
  QObject::connect (
    ret.get (), &TrackLaneList::lanesVisibleChanged, this,
    &Track::fullVisibleHeightChanged);

  return ret;
}

utils::QObjectUniquePtr<RecordableTrackMixin>
Track::make_recordable_track_mixin ()
{
  return utils::make_qobject_unique<RecordableTrackMixin> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = get_port_registry (),
      .param_registry_ = get_param_registry () },
    [this] () { return name_; }, this);
}

utils::QObjectUniquePtr<PianoRollTrackMixin>
Track::make_piano_roll_track_mixin ()
{
  return utils::make_qobject_unique<PianoRollTrackMixin> (this);
}

void
Track::generate_basic_automation_tracks ()
{
  if (!automation_tracklist_)
    return;

  std::vector<utils::QObjectUniquePtr<AutomationTrack>> ats;

  const auto gen = [&] (const dsp::ProcessorBase &processor) {
    generate_automation_tracks_for_processor (ats, processor);
  };

  if (channel_)
    {
      gen (*channel_->fader ());
    }

  if (processor_)
    {
      gen (*processor_);
    }

  // insert the generated automation tracks
  auto * atl = automationTracklist ();
  for (auto &at : ats)
    {
      atl->add_automation_track (std::move (at));
    }

  // mark first automation track as created & visible
  auto * ath = atl->get_first_invisible_automation_track_holder ();
  if (ath != nullptr)
    {
      ath->created_by_user_ = true;
      ath->setVisible (true);
    }

  z_debug ("generated automation tracks for '{}'", name ());
}

double
Track::get_full_visible_height () const
{
  if (!visible_)
    {
      return 0.0;
    }

  double height = main_height_;

  if (lanes_)
    {
      height += lanes_->get_visible_lane_heights ();
    }
  if (automation_tracklist_)
    {
      if (automation_tracklist_->automationVisible ())
        {
          for (
            const auto &at_holder :
            const_cast<const AutomationTracklist &> (*automation_tracklist_)
              .automation_track_holders ())
            {
              if (at_holder->visible ())
                height += at_holder->height ();
            }
        }
    }
  return height;
}

bool
Track::multiply_heights (double multiplier, bool visible_only, bool check_only)
{
  if (main_height_ * multiplier < MIN_HEIGHT)
    return false;

  if (!check_only)
    {
      main_height_ *= multiplier;
    }

  if (lanes_)
    {
      if (!visible_only || lanes_->lanesVisible ())
        {
          for (const auto &lane : lanes_->lanes_view ())
            {
              if (lane->height () * multiplier < MIN_HEIGHT)
                {
                  return false;
                }

              if (!check_only)
                {
                  lane->setHeight (lane->height () * multiplier);
                }
            }
        }
    }
  if (automation_tracklist_)
    {
      if (!visible_only || automation_tracklist_->automationVisible ())
        {
          for (
            const auto &ath :
            const_cast<const AutomationTracklist &> (*automation_tracklist_)
              .automation_track_holders ())
            {
              if (visible_only && !ath->visible ())
                continue;

              if (ath->height () * multiplier < MIN_HEIGHT)
                {
                  return false;
                }

              if (!check_only)
                {
                  ath->setHeight (ath->height () * multiplier);
                }
            }
        }
    }

  return true;
}

bool
Track::contains_uninstantiated_plugin () const
{
  std::vector<plugins::PluginPtrVariant> plugins;
  collect_plugins (plugins);
  return std::ranges::any_of (
    plugins | std::views::transform ([] (const auto &pl_var) {
      return std::visit (
        [] (auto &&pl) -> plugins::Plugin * { return pl; }, pl_var);
    }),
    [] (const auto &pl) {
      return pl->instantiationStatus ()
             != plugins::Plugin::InstantiationStatus::Successful;
    });
}

void
Track::set_caches (CacheType types)
{
  if (ENUM_BITSET_TEST (types, CacheType::PlaybackSnapshots))
    {
      set_playback_caches ();
    }

  if (
    ENUM_BITSET_TEST (types, CacheType::AutomationLaneRecordModes)
    || ENUM_BITSET_TEST (types, CacheType::AutomationLanePorts))
    {
// TODO
#if 0
      if (auto automatable_track = dynamic_cast<AutomatableTrack *> (this))
        {
          automatable_track->get_automation_tracklist ().set_caches (
            CacheType::AutomationLaneRecordModes
            | CacheType::AutomationLanePorts);
        }
#endif
    }
}

void
Track::setClipLauncherMode (bool mode)
{
  if (mode == clip_launcher_mode_)
    return;

  auto * processor = get_track_processor ();
  if (processor == nullptr)
    return;

  processor->set_midi_providers_active (
    tracks::TrackProcessor::ActiveMidiEventProviders::Timeline, !mode);
  processor->set_audio_providers_active (
    tracks::TrackProcessor::ActiveAudioProviders::Timeline, !mode);
  processor->set_midi_providers_active (
    tracks::TrackProcessor::ActiveMidiEventProviders::ClipLauncher, mode);
  processor->set_audio_providers_active (
    tracks::TrackProcessor::ActiveAudioProviders::ClipLauncher, mode);
  clip_launcher_mode_ = mode;
  Q_EMIT clipLauncherModeChanged (mode);
}

void
Track::regeneratePlaybackCaches (utils::ExpandableTickRange affectedRange)
{
  const auto generate_events_for_region_type =
    [&]<arrangement::RegionObject RegionT> () {
      auto all_regions =
        lanes_->lanes ()
        | std::views::transform ([] (auto &uptr) -> const TrackLane * {
            return uptr.get ();
          })
        | std::views::transform ([] (const TrackLane * lane) {
            return lane
              ->arrangement::ArrangerObjectOwner<RegionT>::get_children_view ();
          })
        | std::views::join;
      z_debug (
        "Arranger object contents changed for track '{}' - regenerating caches for range [{}]",
        name (), affectedRange);
      if constexpr (std::is_same_v<RegionT, arrangement::MidiRegion>)
        {
          if (processor_->is_midi ())
            {
              processor_->timeline_midi_data_provider ().generate_midi_events (
                base_dependencies_.tempo_map_.get_tempo_map (), all_regions,
                affectedRange);
            }
        }
      else if constexpr (std::is_same_v<RegionT, arrangement::AudioRegion>)
        {
          if (processor_->is_audio ())
            {
              processor_->timeline_audio_data_provider ().generate_audio_events (
                base_dependencies_.tempo_map_.get_tempo_map (), all_regions,
                affectedRange);
            }
        }
    };
  generate_events_for_region_type.operator()<arrangement::MidiRegion> ();
  generate_events_for_region_type.operator()<arrangement::AudioRegion> ();
}

void
Track::collect_plugins (std::vector<plugins::PluginPtrVariant> &plugins) const
{
  if (channel_)
    {
      channel_->get_plugins (plugins);
    }

  const auto count = modulators_->rowCount ();
  for (const auto i : std::views::iota (0, count))
    {
      auto element = modulators_->element_at_idx (i);
      if (element.canConvert<plugins::Plugin *> ())
        {
          plugins.push_back (
            plugins::plugin_base_to_ptr_variant (
              element.value<plugins::Plugin *> ()));
        }
    }
}

bool
Track::is_plugin_descriptor_valid_for_slot_type (
  const plugins::PluginDescriptor &descr,
  zrythm::plugins::PluginSlotType  slot_type,
  Track::Type                      track_type)
{
  switch (slot_type)
    {
    case zrythm::plugins::PluginSlotType::Insert:
      if (track_type == Track::Type::Midi)
        {
          return descr.num_midi_outs_ > 0;
        }
      else
        {
          return descr.num_audio_outs_ > 0;
        }
    case zrythm::plugins::PluginSlotType::MidiFx:
      return descr.num_midi_outs_ > 0;
      break;
    case zrythm::plugins::PluginSlotType::Instrument:
      return track_type == Track::Type::Instrument && descr.isInstrument ();
    default:
      break;
    }

  z_return_val_if_reached (false);
}

void
Track::collect_timeline_objects (
  std::vector<ArrangerObjectPtrVariant> &objects) const
{
  if (automation_tracklist_)
    {
      // TODO
    }
  if (lanes_)
    {
      // TODO
    }
  collect_additional_timeline_objects (objects);
}

void
from_json (const nlohmann::json &j, Track &track)
{
  from_json (j, static_cast<Track::UuidIdentifiableObject &> (track));
  j.at (Track::kTypeKey).get_to (track.type_);
  j.at (Track::kNameKey).get_to (track.name_);
  j.at (Track::kIconNameKey).get_to (track.icon_name_);
  j.at (Track::kVisibleKey).get_to (track.visible_);
  j.at (Track::kMainHeightKey).get_to (track.main_height_);
  j.at (Track::kEnabledKey).get_to (track.enabled_);
  j.at (Track::kColorKey).get_to (track.color_);
  j.at (Track::kInputSignalTypeKey).get_to (track.in_signal_type_);
  j.at (Track::kOutputSignalTypeKey).get_to (track.out_signal_type_);
  j.at (Track::kCommentKey).get_to (track.comment_);
  // TODO
  // j.at (Track::kFrozenClipIdKey).get_to (track.frozen_clip_id_);
  j[Track::kProcessorKey].get_to (*track.processor_);
  j[Track::kAutomationTracklistKey].get_to (*track.automation_tracklist_);
  j.at (Track::kModulatorsKey).get_to (*track.modulators_);
  for (
    const auto &[index, macro_proc_json] :
    utils::views::enumerate (j.at (Track::kModulatorMacroProcessorsKey)))
    {
      auto macro_proc = utils::make_qobject_unique<dsp::ModulatorMacroProcessor> (
        dsp::ModulatorMacroProcessor::ProcessorBaseDependencies{
          .port_registry_ = track.base_dependencies_.port_registry_,
          .param_registry_ = track.base_dependencies_.param_registry_ },
        index, &track);
      from_json (macro_proc_json, *macro_proc);
      track.modulator_macro_processors_.push_back (std::move (macro_proc));
    }
  j[Track::kTrackLanesKey].get_to (*track.lanes_);
  j[Track::kRecordableTrackMixinKey].get_to (*track.recordable_track_mixin_);
  j[Track::kPianoRollTrackMixinKey].get_to (*track.piano_roll_track_mixin_);
}
}
