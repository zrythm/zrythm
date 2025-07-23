// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "dsp/midi_event.h"
#include "dsp/port.h"
#include "engine/session/midi_mapping.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_processor.h"
#include "utils/dsp.h"
#include "utils/midi.h"
#include "utils/mpmc_queue.h"

#include <boost/container/static_vector.hpp>
#include <fmt/format.h>

namespace zrythm::structure::tracks
{

template <size_t... Is>
static std::array<dsp::ProcessorParameterUuidReference, sizeof...(Is)>
make_port_uuid_reference_array (
  dsp::ProcessorParameterRegistry &param_registry,
  std::index_sequence<Is...>)
{
  return {
    ((void) Is, dsp::ProcessorParameterUuidReference (param_registry))...
  };
}

template <size_t N>
static std::array<dsp::ProcessorParameterUuidReference, N>
make_port_uuid_reference_array (dsp::ProcessorParameterRegistry &param_registry)
{
  return make_port_uuid_reference_array (
    param_registry, std::make_index_sequence<N> ());
}

TrackProcessor::TrackProcessor (
  const dsp::ITransport                 &transport,
  Track                      &tr,
  ProcessorBaseDependencies              dependencies,
  std::optional<FillEventsCallback>      fill_events_cb,
  std::optional<TransformMidiInputsFunc> transform_midi_inputs_func,
  std::optional<AppendMidiInputsToOutputsFunc> append_midi_inputs_to_outputs_func)
    : dsp::ProcessorBase (
        dependencies,
        utils::Utf8String::from_utf8_encoded_string (
          fmt::format ("{} Processor", tr.get_name ()))),
      transport_ (transport),
      is_midi_ (tr.get_input_signal_type () == PortType::Event),
      is_audio_ (tr.get_input_signal_type () == PortType::Audio),
      enabled_provider_ ([&tr] () { return tr.is_enabled (); }),
      track_name_provider_ ([&tr] () { return tr.get_name (); }),
      fill_events_cb_(std::move(fill_events_cb)),
      append_midi_inputs_to_outputs_func_ (
        append_midi_inputs_to_outputs_func.has_value() ?
        append_midi_inputs_to_outputs_func.value() :
        [] (
          dsp::MidiEventVector        &out_events,
          const dsp::MidiEventVector  &in_events,
          const EngineProcessTimeInfo &time_nfo) {
          out_events.append (
            in_events, time_nfo.local_offset_, time_nfo.nframes_);
        }),
      transform_midi_inputs_func_(std::move(transform_midi_inputs_func ))
{
  if (is_midi ())
    {
      {
        add_input_port (dependencies.port_registry_.create_object<dsp::MidiPort> (
          u8"TP MIDI in", dsp::PortFlow::Input));
        auto * midi_in = &get_midi_in_port ();
        midi_in->set_full_designation_provider (this);
        midi_in->set_symbol (u8"track_processor_midi_in");

        add_output_port (dependencies.port_registry_.create_object<dsp::MidiPort> (
          u8"TP MIDI out", dsp::PortFlow::Output));
        auto * midi_out = &get_midi_out_port ();
        midi_out->set_full_designation_provider (this);
        midi_out->set_symbol (u8"track_processor_midi_out");
      }

      /* set up piano roll port */
      if (tr.has_piano_roll () || tr.is_chord ())
        {
          add_input_port (
            dependencies.port_registry_.create_object<dsp::MidiPort> (
              u8"TP Piano Roll", PortFlow::Input));
          piano_roll_port_id_ = get_input_ports ().back ();
          auto * piano_roll = &get_piano_roll_port ();
          piano_roll->set_full_designation_provider (this);
          piano_roll->set_symbol (u8"track_processor_piano_roll");
          if (!tr.is_chord ())
            {
              midi_cc_ids_ = std::make_unique<
                decltype (midi_cc_ids_)::element_type> (
                make_port_uuid_reference_array<2048> (
                  dependencies.param_registry_));
              pitch_bend_ids_ = std::make_unique<
                decltype (pitch_bend_ids_)::element_type> (
                make_port_uuid_reference_array<16> (
                  dependencies.param_registry_));
              poly_key_pressure_ids_ = std::make_unique<
                decltype (poly_key_pressure_ids_)::element_type> (
                make_port_uuid_reference_array<16> (
                  dependencies.param_registry_));
              channel_pressure_ids_ = std::make_unique<
                decltype (channel_pressure_ids_)::element_type> (
                make_port_uuid_reference_array<16> (
                  dependencies.param_registry_));

              for (const auto i : std::views::iota (0, 16))
                {
                  /* starting from 1 */
                  int channel = i + 1;

                  for (const auto j : std::views::iota (0, 128))
                    {
                      (*midi_cc_ids_)[(i * 128) + j] =
                        dependencies.param_registry_.create_object<
                          dsp::ProcessorParameter> (
                          dependencies.port_registry_,
                          dsp::ProcessorParameter::UniqueId (
                            utils::Utf8String::from_utf8_encoded_string (
                              fmt::format (
                                "midi_controller_ch{}_{}", channel, j + 1))),
                          dsp::ParameterRange (
                            dsp::ParameterRange::Type::Integer, 0.f, 127.f, 0.f,
                            0.f),
                          utils::Utf8String::from_utf8_encoded_string (
                            fmt::format (
                              "Ch{} {}", channel,
                              utils::midi::midi_get_controller_name (j))));
                    }

                  (*pitch_bend_ids_)[i] = dependencies.param_registry_.create_object<
                    dsp::ProcessorParameter> (
                    dependencies.port_registry_,
                    dsp::ProcessorParameter::UniqueId (
                      utils::Utf8String::from_utf8_encoded_string (
                        fmt::format ("ch{}_pitch_bend", i + 1))),
                    dsp::ParameterRange (
                      dsp::ParameterRange::Type::Integer, -8192.f, 8191.f, 0.f,
                      0.f),
                    utils::Utf8String::from_utf8_encoded_string (
                      fmt::format ("Ch{} Pitch bend", i + 1)));

                  (*poly_key_pressure_ids_)[i] =
                    dependencies.param_registry_.create_object<
                      dsp::ProcessorParameter> (
                      dependencies.port_registry_,
                      dsp::ProcessorParameter::UniqueId (
                        utils::Utf8String::from_utf8_encoded_string (
                          fmt::format ("ch{}_poly_key_pressure", i + 1))),
                      dsp::ParameterRange (
                        dsp::ParameterRange::Type::Integer, 0.f, 127.f, 0.f, 0.f),
                      utils::Utf8String::from_utf8_encoded_string (
                        fmt::format ("Ch{} Poly key pressure", i + 1)));

                  (*channel_pressure_ids_)[i] =
                    dependencies.param_registry_.create_object<
                      dsp::ProcessorParameter> (
                      dependencies.port_registry_,
                      dsp::ProcessorParameter::UniqueId (
                        utils::Utf8String::from_utf8_encoded_string (
                          fmt::format ("ch{}_channel_pressure", i + 1))),
                      dsp::ParameterRange (
                        dsp::ParameterRange::Type::Integer, 0.f, 127.f, 0.f, 0.f),
                      utils::Utf8String::from_utf8_encoded_string (
                        fmt::format ("Ch{} Channel pressure", i + 1)));
                }
            }
        }
    }
  else if (is_audio ())
    {

      const auto init_stereo_out_ports = [&] (bool in) {
        auto stereo_ports = dsp::StereoPorts::create_stereo_ports (
          dependencies.port_registry_, in,
          in ? u8"TP Stereo in" : u8"TP Stereo out",
          utils::Utf8String (u8"track_processor_stereo_")
            + (in ? u8"in" : u8"out"));
        iterate_tuple (
          [&] (const auto &port_ref) {
            auto * port = std::get<dsp::AudioPort *> (port_ref.get_object ());
            port->set_full_designation_provider (this);
          },
          stereo_ports);

        if (in)
          {
            add_input_port (stereo_ports.first);
            add_input_port (stereo_ports.second);
          }
        else
          {
            add_output_port (stereo_ports.first);
            add_output_port (stereo_ports.second);
          }
      };
      init_stereo_out_ports (false);
      init_stereo_out_ports (true);
      if (tr.get_type () == Track::Type::Audio)
        {
          mono_id_ = dependencies.param_registry_.create_object<
            dsp::ProcessorParameter> (
            dependencies.port_registry_,
            dsp::ProcessorParameter::UniqueId (
              u8"track_processor_mono_toggle"

              ),
            dsp::ParameterRange::make_toggle (false), u8"TP Mono Toggle");
          input_gain_id_ = dependencies.param_registry_.create_object<
            dsp::ProcessorParameter> (
            dependencies.port_registry_,
            dsp::ProcessorParameter::UniqueId (
              u8"track_processor_input_gain"

              ),
            dsp::ParameterRange (
              dsp::ParameterRange::Type::GainAmplitude, 0.f, 4.f, 0.f, 1.f),
            u8"TP Input Gain");
        }
    }

  if (tr.get_type () == Track::Type::Audio)
    {
      output_gain_id_ = dependencies.param_registry_.create_object<
        dsp::ProcessorParameter> (
        dependencies.port_registry_,
        dsp::ProcessorParameter::UniqueId (
          u8"track_processor_output_gain"

          ),
        dsp::ParameterRange (
          dsp::ParameterRange::Type::GainAmplitude, 0.f, 4.f, 0.f, 1.f),
        u8"TP Output Gain");

      monitor_audio_id_ = dependencies.param_registry_.create_object<
        dsp::ProcessorParameter> (
        dependencies.port_registry_,
        dsp::ProcessorParameter::UniqueId (
          u8"track_processor_monitor_audio"

          ),
        dsp::ParameterRange::make_toggle (false), u8"Monitor audio");
    }

  if (midi_cc_ids_)
    {
      cc_mappings_ = std::make_unique<engine::session::MidiMappings> (
        dependencies.param_registry_);

      for (const auto i : std::views::iota (0, 16))
        {
          for (const auto j : std::views::iota (0, 128))
            {
              auto cc_port_id = (*midi_cc_ids_)[i * 128 + j];
              auto cc_port =
                cc_port_id.get_object_as<dsp::ProcessorParameter> ();

              /* set caches */
              // cc_port->midi_channel_ = i + 1;
              // cc_port->midi_cc_no_ = j;

              /* set model bytes for CC:
               * [0] = ctrl change + channel
               * [1] = controller
               * [2] (unused) = control */
              std::array<midi_byte_t, 3> buf{};
              buf[0] =
                (midi_byte_t) (utils::midi::MIDI_CH1_CTRL_CHANGE | (midi_byte_t) i);
              buf[1] = (midi_byte_t) j;
              buf[2] = 0;

              /* bind */
              cc_mappings_->bind_track (
                buf, { cc_port->get_uuid (), dependencies.param_registry_ },
                false);
            } /* endforeach 0..127 */
        } /* endforeach 0..15 */

      updated_midi_automatable_ports_ =
        std::make_unique<MPMCQueue<dsp::ProcessorParameter *>> (128 * 16);
    }
}

utils::Utf8String
TrackProcessor::get_full_designation_for_port (const dsp::Port &port) const
{
  return utils::Utf8String::from_utf8_encoded_string (
    fmt::format ("{}/{}", track_name_provider_ (), port.get_label ()));
}

void
init_from (
  TrackProcessor        &obj,
  const TrackProcessor  &other,
  utils::ObjectCloneType clone_type)
{
// TODO
#if 0
  init_from (
    static_cast<dsp::ProcessorBase &> (obj),
    static_cast<const dsp::ProcessorBase &> (other), clone_type);
#endif
}

/**
 * Clears all buffers.
 */
void
TrackProcessor::clear_buffers (std::size_t block_length)
{
  if (is_audio ())
    {
      iterate_tuple (
        [&] (auto &port) { port.clear_buffer (block_length); },
        get_stereo_in_ports ());
      iterate_tuple (
        [&] (auto &port) { port.clear_buffer (block_length); },
        get_stereo_out_ports ());
    }
  else if (is_midi ())
    {
      get_midi_in_port ().clear_buffer (block_length);
      get_midi_out_port ().clear_buffer (block_length);
    }
  if (piano_roll_port_id_.has_value ())
    {
      get_piano_roll_port ().clear_buffer (block_length);
    }
}

void
TrackProcessor::handle_recording (const EngineProcessTimeInfo &time_nfo)
{
  assert (handle_recording_cb_.has_value ());

  unsigned_frame_t start = time_nfo.g_start_frame_w_offset_;
  unsigned_frame_t end = time_nfo.g_start_frame_ + time_nfo.nframes_;
  const auto       loop = transport_.get_loop_range_positions ();

  // split point + nframes pairs
  boost::container::static_vector<std::pair<unsigned_frame_t, nframes_t>, 6>
    ranges;
  ranges.emplace_back (start, time_nfo.nframes_);

  const bool loop_hit = transport_.loop_enabled () && loop.second == end;

  // Handle loop case
  if (loop_hit)
    {
      nframes_t pre_loop = loop.second - start;
      ranges.clear ();
      ranges.emplace_back (start, pre_loop);
      ranges.emplace_back (loop.second, 0); // loop end pause
      ranges.emplace_back (loop.first, time_nfo.nframes_ - pre_loop);
    }
  // Handle punch points
  if (transport_.punch_enabled ())
    {
      auto punch = transport_.get_punch_range_positions ();

      bool punch_in_hit = false;
      bool punch_out_hit = false;

      if (loop_hit)
        {
          // before loop
          punch_in_hit = punch.first >= start && punch.first < loop.second;
          if (punch_in_hit)
            {
              constexpr size_t index_to_insert = 1;
              // add punch in
              ranges.insert (
                ranges.begin () + index_to_insert,
                std::make_pair (punch.first, loop.second - punch.first));

              // adjust frames of previous split
              ranges[index_to_insert - 1].second -=
                ranges[index_to_insert].second;
            }
          punch_out_hit = punch.second >= start && punch.second < loop.second;
          if (punch_out_hit)
            {
              if (punch_in_hit)
                {
                  ranges.insert (
                    ranges.begin () + 2,
                    std::make_pair (punch.second, loop.second - punch.second));

                  // add pause
                  ranges.insert (
                    ranges.begin () + 3,
                    std::make_pair (ranges[2].first + ranges[2].second, 0));

                  // adjust frames of previous split
                  ranges[1].second -= ranges[2].second;
                }
              else
                {
                  // add punch out
                  ranges.insert (
                    ranges.begin () + 1,
                    std::make_pair (punch.second, loop.second - punch.second));

                  // add pause
                  ranges.insert (
                    ranges.begin () + 2,
                    std::make_pair (ranges[1].first + ranges[1].second, 0));

                  // adjust frames of previous split
                  ranges[0].second -= ranges[1].second;
                }
            }
        }
      else // loop not hit
        {
          punch_in_hit = punch.first >= start && punch.first < end;
          if (punch_in_hit)
            {
              // punch in
              ranges.emplace_back (punch.first, end - punch.first);

              // adjust frames of previous split
              ranges[0].second -= ranges[1].second;
            }
          punch_out_hit = punch.second >= start && punch.second < end;
          if (punch_out_hit)
            {
              if (punch_in_hit)
                {
                  // add punch out
                  ranges.emplace_back (punch.second, end - punch.second);

                  // add pause
                  ranges.emplace_back (ranges[2].first + ranges[2].second, 0);

                  // adjust frames of previous split
                  ranges[1].second -= ranges[2].second;
                }
              else
                {
                  // add punch out
                  ranges.emplace_back (punch.second, end - punch.second);

                  // add pause
                  ranges.emplace_back (ranges[1].first + ranges[1].second, 0);

                  // adjust frames of previous split
                  ranges[0].second -= ranges[1].second;
                }
            }
        }
    }

  // Process recording ranges
  for (const auto &[index, range] : utils::views::enumerate (ranges))
    {
      // skip if same as previous point
      if (index != 0 && range.first == ranges[index - 1].first)
        continue;

      EngineProcessTimeInfo cur_time_nfo = {
        .g_start_frame_ = range.first,
        .g_start_frame_w_offset_ = range.first,
        .local_offset_ = 0,
        .nframes_ = range.second
      };
      std::invoke (*handle_recording_cb_, cur_time_nfo);
    }
}

void
TrackProcessor::add_events_from_midi_cc_control_ports (
  dsp::MidiEventVector &events,
  const nframes_t       local_offset)
{
  if (updated_midi_automatable_ports_ == nullptr)
    return;

  using AddEventCallback = std::function<void (
    const dsp::ProcessorParameter &cc, dsp::MidiEventVector &vec_to_fill,
    size_t index_in_vector, midi_byte_t time)>;

  // returns if an event was added
  const auto add_event_for_cc_if_in_range =
    [&events, local_offset] [[nodiscard]] (
      const RangeOf<dsp::ProcessorParameterUuidReference> auto &range,
      const dsp::ProcessorParameter &cc, const AddEventCallback &add_event_cb)
    -> bool {
    auto * it = std::ranges::find (
      range, cc.get_uuid (), &dsp::ProcessorParameterUuidReference::id);
    if (it != range.end ())
      {
        add_event_cb (
          cc, events, std::ranges::distance (range.begin (), it), local_offset);
        return true;
      }
    return false;
  };

  dsp::ProcessorParameter * cc{};
  while (updated_midi_automatable_ports_->pop_front (cc))
    {
      if (
        add_event_for_cc_if_in_range (
          *pitch_bend_ids_, *cc,
          [] (
            const dsp::ProcessorParameter &cc, dsp::MidiEventVector &vec_to_fill,
            const size_t index_in_vector, midi_byte_t time) {
            vec_to_fill.add_pitchbend (
              index_in_vector + 1,
              utils::math::round_to_signed_32 (
                cc.range ().convertFrom0To1 (cc.currentValue ()))
                + 0x2000,
              time);
          }))
        continue;

      if (
        add_event_for_cc_if_in_range (
          *poly_key_pressure_ids_, *cc,
          [] (
            const dsp::ProcessorParameter &cc, dsp::MidiEventVector &vec_to_fill,
            const size_t index_in_vector, midi_byte_t time) {
            // TODO
          }))
        continue;

      if (
        add_event_for_cc_if_in_range (
          *channel_pressure_ids_, *cc,
          [] (
            const dsp::ProcessorParameter &cc, dsp::MidiEventVector &vec_to_fill,
            const size_t index_in_vector, midi_byte_t time) {
            vec_to_fill.add_channel_pressure (
              index_in_vector + 1,
              (midi_byte_t) utils::math::round_to_signed_32 (
                cc.currentValue () * 127.f),
              time);
          }))
        continue;

      bool event_added{};
      for (const auto i : std::views::iota (0, 16))
        {
          /* starting from 1 */
          const midi_byte_t channel = i + 1;

          auto begin_it = midi_cc_ids_->begin () + (i * 128);
          event_added = add_event_for_cc_if_in_range (
            std::ranges::subrange (begin_it, begin_it + 128), *cc,
            [channel] (
              const dsp::ProcessorParameter &cc,
              dsp::MidiEventVector &vec_to_fill, const size_t index_in_vector,
              midi_byte_t time) {
              vec_to_fill.add_control_change (
                channel, index_in_vector,
                (midi_byte_t) utils::math::round_to_signed_32 (
                  cc.currentValue () * 127.f),
                time);
            });

          if (event_added)
            break;
        }
      if (event_added)
        continue;
    }
}

// ============================================================================
// IProcessable Interface
// ============================================================================

void
TrackProcessor::custom_process_block (EngineProcessTimeInfo time_nfo)
{
  if (!enabled_provider_ ())
    {
      return;
    }

  const bool should_fill_events =
    fill_events_cb_.has_value ()
    && (transport_.get_play_state () == dsp::ITransport::PlayState::Rolling);

  // fill ports based on arrangement (clip) contents
  if (fill_events_cb_.has_value () && should_fill_events)
    {
      // audio clips
      if (is_audio ())
        {
          const auto stereo_ports = get_stereo_out_ports ();
          fill_audio_events (
            time_nfo,
            std::make_pair (
              std::span (stereo_ports.first.buf_),
              std::span (stereo_ports.second.buf_)));
        }
      // MIDI clips
      else if (is_midi ())
        {
          auto &pr = get_piano_roll_port ();
          fill_midi_events (time_nfo, pr.midi_events_.queued_events_);
          pr.midi_events_.dequeue (time_nfo.local_offset_, time_nfo.nframes_);

          /* append the midi events from piano roll to MIDI out */
          get_midi_out_port ().midi_events_.queued_events_.append (
            pr.midi_events_.active_events_, time_nfo.local_offset_,
            time_nfo.nframes_);
        }
    }

  if (is_midi ())
    {
      // append midi events from modwheel and pitchbend control ports to
      // MIDI out
      add_events_from_midi_cc_control_ports (
        get_midi_out_port ().midi_events_.queued_events_,
        time_nfo.local_offset_);
    }

  /* add inputs to outputs */
  if (is_audio ())
    {
      const auto &stereo_in = get_stereo_in_ports ();
      const auto &stereo_out = get_stereo_out_ports ();
      const auto  input_gain = [this] () {
        const auto &input_gain_param = get_input_gain_param ();
        return input_gain_param.range ().convertFrom0To1 (
          input_gain_param.currentValue ());
      };
      const auto mono = [this] () {
        const auto &mono_param = get_mono_param ();
        return mono_param.range ().is_toggled (mono_param.currentValue ());
      };
      const auto monitor_audio = [this] () {
        const auto &monitor_audio_param = get_monitor_audio_param ();
        return monitor_audio_param.range ().is_toggled (
          monitor_audio_param.currentValue ());
      };

      // if track with enabled "monitor" param, or
      if (!monitor_audio_id_.has_value () || monitor_audio ())
        {
          utils::float_ranges::mix_product (
            &stereo_out.first.buf_[time_nfo.local_offset_],
            &stereo_in.first.buf_[time_nfo.local_offset_],
            input_gain_id_ ? input_gain () : 1.f, time_nfo.nframes_);

          if (mono_id_ && mono ())
            {
              utils::float_ranges::mix_product (
                &stereo_out.second.buf_[time_nfo.local_offset_],
                &stereo_in.first.buf_[time_nfo.local_offset_],
                input_gain_id_ ? input_gain () : 1.f, time_nfo.nframes_);
            }
          else
            {
              utils::float_ranges::mix_product (
                &stereo_out.second.buf_[time_nfo.local_offset_],
                &stereo_in.second.buf_[time_nfo.local_offset_],
                input_gain_id_ ? input_gain () : 1.f, time_nfo.nframes_);
            }
        }
    }
  else if (is_midi ())
    {
      // apply any transformations
      if (transform_midi_inputs_func_.has_value ())
        {
          std::invoke (
            *transform_midi_inputs_func_,
            get_midi_in_port ().midi_events_.active_events_);
        }

      /* process midi bindings */
      if (cc_mappings_ && transport_.recording_enabled ())
        {
          cc_mappings_->apply_from_cc_events (
            get_midi_in_port ().midi_events_.active_events_);
        }

      // append data from MIDI input -> MIDI output
      append_midi_inputs_to_outputs_func_ (
        get_midi_out_port ().midi_events_.queued_events_,
        get_midi_in_port ().midi_events_.active_events_, time_nfo);
    }

  if (
    !transport_.has_preroll_frames_remaining ()
    && handle_recording_cb_.has_value ())
    {
      /* handle recording. this will only create events in regions. it
       * will not copy the input content to the output ports. this will
       * also create automation for MIDI CC, if any (see
       * midi_mappings_apply_cc_events above) */
      handle_recording (time_nfo);
    }

  /* apply output gain */
  if (output_gain_id_.has_value ())
    {
      const auto &output_gain_param = get_output_gain_param ();
      const auto  output_gain = output_gain_param.range ().convertFrom0To1 (
        output_gain_param.currentValue ());

      const auto stereo_out = get_stereo_out_ports ();
      utils::float_ranges::mul_k2 (
        &stereo_out.first.buf_[time_nfo.local_offset_], output_gain,
        time_nfo.nframes_);
      utils::float_ranges::mul_k2 (
        &stereo_out.second.buf_[time_nfo.local_offset_], output_gain,
        time_nfo.nframes_);
    }
}

// ============================================================================

void
from_json (const nlohmann::json &j, TrackProcessor &tp)
{
// TODO
#if 0
  j.at (TrackProcessor::kMonoKey).get_to (tp.mono_id_);
  j.at (TrackProcessor::kInputGainKey).get_to (tp.input_gain_id_);
  j.at (TrackProcessor::kOutputGainKey).get_to (tp.output_gain_id_);
  j.at (TrackProcessor::kMidiInKey).get_to (tp.midi_in_id_);
  j.at (TrackProcessor::kMidiOutKey).get_to (tp.midi_out_id_);
  j.at (TrackProcessor::kPianoRollKey).get_to (tp.piano_roll_id_);
  j.at (TrackProcessor::kMonitorAudioKey).get_to (tp.monitor_audio_id_);
  j.at (TrackProcessor::kStereoInLKey).get_to (tp.stereo_in_left_id_);
  j.at (TrackProcessor::kStereoInRKey).get_to (tp.stereo_in_right_id_);
  j.at (TrackProcessor::kStereoOutLKey).get_to (tp.stereo_out_left_id_);
  j.at (TrackProcessor::kStereoOutRKey).get_to (tp.stereo_out_right_id_);
  if (j.contains (TrackProcessor::kMidiCcKey))
    {
      j.at (TrackProcessor::kMidiCcKey).get_to (tp.midi_cc_ids_);
      j.at (TrackProcessor::kPitchBendKey).get_to (tp.pitch_bend_ids_);
      j.at (TrackProcessor::kPolyKeyPressureKey)
        .get_to (tp.poly_key_pressure_ids_);
      j.at (TrackProcessor::kChannelPressureKey)
        .get_to (tp.channel_pressure_ids_);
    }
#endif
}
}
