// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstddef>
#include <utility>

#include "dsp/midi_event.h"
#include "dsp/port.h"
#include "structure/tracks/track_processor.h"
#include "utils/dsp.h"
#include "utils/midi.h"
#include "utils/mpmc_queue.h"
#include "utils/views.h"

#include <boost/container/static_vector.hpp>
#include <fmt/format.h>
#include <scn/scan.h>

namespace zrythm::structure::tracks
{

static constexpr auto midi_cc_id_format_str = "midi_controller_ch{}_{}"sv;
static constexpr auto pitch_bend_id_format_str = "ch{}_pitch_bend"sv;
static constexpr auto poly_key_pressure_id_format_str =
  "ch{}_poly_key_pressure"sv;
static constexpr auto channel_pressure_id_format_str = "ch{}_channel_pressure"sv;
static constexpr auto mono_param_id = "track_processor_mono_toggle"sv;
static constexpr auto input_gain_param_id = "track_processor_input_gain"sv;
static constexpr auto output_gain_param_id = "track_processor_output_gain"sv;
static constexpr auto monitor_audio_param_id = "track_processor_monitor_audio"sv;

TrackProcessor::TrackProcessor (
  const dsp::ITransport                 &transport,
  PortType signal_type,
    TrackNameProvider track_name_provider,
    EnabledProvider enabled_provider,
    bool                                   generates_midi_events,
    bool has_midi_cc,
    bool                                   is_audio_track,
  ProcessorBaseDependencies              dependencies,
  std::optional<FillEventsCallback>      fill_events_cb,
  std::optional<TransformMidiInputsFunc> transform_midi_inputs_func,
  std::optional<AppendMidiInputsToOutputsFunc> append_midi_inputs_to_outputs_func, QObject * parent)
    : QObject(parent), dsp::ProcessorBase (
        dependencies,
        utils::Utf8String::from_utf8_encoded_string (
          fmt::format ("{} Processor", track_name_provider()))),
      transport_ (transport),
      is_midi_ (signal_type == PortType::Midi),
      is_audio_ (signal_type == PortType::Audio),
      has_piano_roll_port_(generates_midi_events),
      has_midi_cc_(has_midi_cc),
      enabled_provider_ (std::move(enabled_provider)),
      track_name_provider_ (track_name_provider),
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
      if (has_piano_roll_port_)
        {
          add_input_port (
            dependencies.port_registry_.create_object<dsp::MidiPort> (
              u8"TP Piano Roll", PortFlow::Input));
          auto &piano_roll = get_piano_roll_port ();
          piano_roll.set_full_designation_provider (this);
          piano_roll.set_symbol (u8"track_processor_piano_roll");
        }

      if (has_midi_cc_)
        {
          for (const auto i : std::views::iota (0, 16))
            {
              /* starting from 1 */
              int channel = i + 1;

              for (const auto j : std::views::iota (0, 128))
                {
                  add_parameter (dependencies.param_registry_.create_object<
                                 dsp::ProcessorParameter> (
                    dependencies.port_registry_,
                    dsp::ProcessorParameter::UniqueId (
                      utils::Utf8String::from_utf8_encoded_string (
                        fmt::format (midi_cc_id_format_str, channel, j + 1))),
                    dsp::ParameterRange (
                      dsp::ParameterRange::Type::Integer, 0.f, 127.f, 0.f, 0.f),
                    utils::Utf8String::from_utf8_encoded_string (
                      fmt::format (
                        "Ch{} {}", channel,
                        utils::midi::midi_get_controller_name (j)))));
                }

              add_parameter (dependencies.param_registry_.create_object<
                             dsp::ProcessorParameter> (
                dependencies.port_registry_,
                dsp::ProcessorParameter::UniqueId (
                  utils::Utf8String::from_utf8_encoded_string (
                    fmt::format (pitch_bend_id_format_str, i + 1))),
                dsp::ParameterRange (
                  dsp::ParameterRange::Type::Integer, -8192.f, 8191.f, 0.f, 0.f),
                utils::Utf8String::from_utf8_encoded_string (
                  fmt::format ("Ch{} Pitch bend", i + 1))));

              add_parameter (dependencies.param_registry_.create_object<
                             dsp::ProcessorParameter> (
                dependencies.port_registry_,
                dsp::ProcessorParameter::UniqueId (
                  utils::Utf8String::from_utf8_encoded_string (
                    fmt::format (poly_key_pressure_id_format_str, i + 1))),
                dsp::ParameterRange (
                  dsp::ParameterRange::Type::Integer, 0.f, 127.f, 0.f, 0.f),
                utils::Utf8String::from_utf8_encoded_string (
                  fmt::format ("Ch{} Poly key pressure", i + 1))));

              add_parameter (dependencies.param_registry_.create_object<
                             dsp::ProcessorParameter> (
                dependencies.port_registry_,
                dsp::ProcessorParameter::UniqueId (
                  utils::Utf8String::from_utf8_encoded_string (
                    fmt::format (channel_pressure_id_format_str, i + 1))),
                dsp::ParameterRange (
                  dsp::ParameterRange::Type::Integer, 0.f, 127.f, 0.f, 0.f),
                utils::Utf8String::from_utf8_encoded_string (
                  fmt::format ("Ch{} Channel pressure", i + 1))));
            }
        } // endif has MIDI CC
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
    }

  if (is_audio_track)
    {
      add_parameter (
        dependencies.param_registry_.create_object<dsp::ProcessorParameter> (
          dependencies.port_registry_,
          dsp::ProcessorParameter::UniqueId (
            utils::Utf8String::from_utf8_encoded_string (mono_param_id)),
          dsp::ParameterRange::make_toggle (false), u8"TP Mono Toggle"));
      mono_id_ = get_parameters ().back ().id ();

      add_parameter (
        dependencies.param_registry_.create_object<dsp::ProcessorParameter> (
          dependencies.port_registry_,
          dsp::ProcessorParameter::UniqueId (
            utils::Utf8String::from_utf8_encoded_string (input_gain_param_id)),
          dsp::ParameterRange (
            dsp::ParameterRange::Type::GainAmplitude, 0.f, 4.f, 0.f, 1.f),
          u8"TP Input Gain"));
      input_gain_id_ = get_parameters ().back ().id ();

      add_parameter (
        dependencies.param_registry_.create_object<dsp::ProcessorParameter> (
          dependencies.port_registry_,
          dsp::ProcessorParameter::UniqueId (
            utils::Utf8String::from_utf8_encoded_string (output_gain_param_id)),
          dsp::ParameterRange (
            dsp::ParameterRange::Type::GainAmplitude, 0.f, 4.f, 0.f, 1.f),
          u8"TP Output Gain"));
      output_gain_id_ = get_parameters ().back ().id ();

      add_parameter (
        dependencies.param_registry_.create_object<dsp::ProcessorParameter> (
          dependencies.port_registry_,
          dsp::ProcessorParameter::UniqueId (
            utils::Utf8String::from_utf8_encoded_string (monitor_audio_param_id)),
          dsp::ParameterRange::make_toggle (false), u8"Monitor audio"));
      monitor_audio_id_ = get_parameters ().back ().id ();
    }

  // generate MIDI CC caches and set up mappings
  set_param_id_caches ();
  // set_midi_mappings ();
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
      if (is_midi ())
        {
          std::invoke (
            *handle_recording_cb_, cur_time_nfo,
            &processing_caches_->midi_in_rt_->midi_events_.active_events_,
            std::nullopt);
        }
      else if (is_audio ())
        {
          // assumed audio track (other audio-based tracks are not recordable)
          assert (mono_id_.has_value ());
          const auto &l = *processing_caches_->audio_ins_rt_[0];
          const auto &r =
            processing_caches_->mono_param_->range ().is_toggled (
              processing_caches_->mono_param_->currentValue ())
              ? *processing_caches_->audio_ins_rt_[0]
              : *processing_caches_->audio_ins_rt_[1];
          std::invoke (
            *handle_recording_cb_, cur_time_nfo, nullptr,
            std::make_pair (std::span (l.buf_), std::span (r.buf_)));
        }
    }
}

void
TrackProcessor::add_events_from_midi_cc_control_ports (
  dsp::MidiEventVector &events,
  const nframes_t       local_offset)
{
  assert (has_midi_cc_);

  using AddEventCallback = std::function<void (
    const dsp::ProcessorParameter &cc, dsp::MidiEventVector &vec_to_fill,
    size_t index_in_vector, midi_byte_t time)>;

  // returns if an event was added
  const auto add_event_for_cc_if_in_range =
    [&events, local_offset] [[nodiscard]] (
      const RangeOf<dsp::ProcessorParameter::Uuid> auto &range,
      const dsp::ProcessorParameter &cc, const AddEventCallback &add_event_cb)
    -> bool {
    auto it = std::ranges::find (range, cc.get_uuid ());
    if (it != range.end ())
      {
        add_event_cb (
          cc, events, std::ranges::distance (range.begin (), it), local_offset);
        return true;
      }
    return false;
  };

  dsp::ProcessorParameter * popped_cc{};
  while (midi_cc_caches_->updated_midi_automatable_ports_.pop_front (popped_cc))
    {
      if (
        add_event_for_cc_if_in_range (
          midi_cc_caches_->pitch_bend_ids_, *popped_cc,
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
          midi_cc_caches_->poly_key_pressure_ids_, *popped_cc,
          [] (
            const dsp::ProcessorParameter &cc, dsp::MidiEventVector &vec_to_fill,
            const size_t index_in_vector, midi_byte_t time) {
            // TODO
          }))
        continue;

      if (
        add_event_for_cc_if_in_range (
          midi_cc_caches_->channel_pressure_ids_, *popped_cc,
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

          auto begin_it =
            std::next (midi_cc_caches_->midi_cc_ids_.begin (), i * 128);
          event_added = add_event_for_cc_if_in_range (
            std::ranges::subrange (begin_it, std::next (begin_it, 128)),
            *popped_cc,
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

void
TrackProcessor::set_param_id_caches ()
{
  if (has_midi_cc_)
    {
      midi_cc_caches_ = std::make_unique<MidiCcCaches> ();
    }

  for (const auto &param_ref : get_parameters ())
    {
      const auto * param = param_ref.get_object_as<dsp::ProcessorParameter> ();

      const auto &id_str = type_safe::get (param->get_unique_id ()).view ();

      if (id_str == mono_param_id)
        {
          mono_id_ = param_ref.id ();
        }
      else if (id_str == input_gain_param_id)
        {
          input_gain_id_ = param_ref.id ();
        }
      else if (id_str == output_gain_param_id)
        {
          output_gain_id_ = param_ref.id ();
        }
      else if (id_str == monitor_audio_param_id)
        {
          monitor_audio_id_ = param_ref.id ();
        }
      else if (
        auto midi_cc_scan_result =
          scn::scan<midi_byte_t, midi_byte_t> (id_str, midi_cc_id_format_str))
        {
          auto [midi_channel, cc_no] = midi_cc_scan_result->values ();
          --midi_channel;
          --cc_no;
          midi_cc_caches_->midi_cc_ids_[(midi_channel * 128) + cc_no] =
            param_ref.id ();
        }
      else if (
        auto pitch_bend_scan_result =
          scn::scan<midi_byte_t> (id_str, pitch_bend_id_format_str))
        {
          auto midi_channel = pitch_bend_scan_result->value ();
          --midi_channel;
          midi_cc_caches_->pitch_bend_ids_[midi_channel] = param_ref.id ();
        }
      else if (
        auto poly_key_pressure_scan_result =
          scn::scan<midi_byte_t> (id_str, poly_key_pressure_id_format_str))
        {
          auto midi_channel = poly_key_pressure_scan_result->value ();
          --midi_channel;
          midi_cc_caches_->poly_key_pressure_ids_[midi_channel] =
            param_ref.id ();
        }
      else if (
        auto channel_pressure_scan_result =
          scn::scan<midi_byte_t> (id_str, channel_pressure_id_format_str))
        {
          auto midi_channel = channel_pressure_scan_result->value ();
          --midi_channel;
          midi_cc_caches_->channel_pressure_ids_[midi_channel] = param_ref.id ();
        }
    }
}

// TODO
#if 0
void
TrackProcessor::set_midi_mappings ()
{
  if (!has_midi_cc_)
    return;

  cc_mappings_ = std::make_unique<engine::session::MidiMappings> (
    dependencies_.param_registry_);

  for (const auto i : std::views::iota (0, 16))
    {
      for (const auto j : std::views::iota (0, 128))
        {
          auto   cc_port_id = midi_cc_caches_->midi_cc_ids_[(i * 128) + j];
          auto * cc_port = std::get<dsp::ProcessorParameter *> (
            dependencies_.param_registry_.find_by_id_or_throw (cc_port_id));

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
            buf, { cc_port->get_uuid (), dependencies_.param_registry_ }, false);
        } /* endforeach 0..127 */
    } /* endforeach 0..15 */
}
#endif

// ============================================================================
// IProcessable Interface
// ============================================================================

void
TrackProcessor::custom_process_block (EngineProcessTimeInfo time_nfo) noexcept
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
          fill_audio_events (
            time_nfo,
            std::make_pair (
              std::span (processing_caches_->audio_outs_rt_[0]->buf_),
              std::span (processing_caches_->audio_outs_rt_[1]->buf_)));
        }
      // MIDI clips
      else if (has_piano_roll_port_)
        {
          auto &pr = get_piano_roll_port ();
          fill_midi_events (time_nfo, pr.midi_events_.queued_events_);
        }
    }

  // dequeue piano roll contents into MIDI output port
  if (has_piano_roll_port_)
    {
      auto &pr = *processing_caches_->piano_roll_rt_;
      pr.midi_events_.dequeue (time_nfo.local_offset_, time_nfo.nframes_);

      /* append the midi events from piano roll to MIDI out */
      processing_caches_->midi_out_rt_->midi_events_.queued_events_.append (
        pr.midi_events_.active_events_, time_nfo.local_offset_,
        time_nfo.nframes_);
    }

  if (has_midi_cc_)
    {
      // append midi events from modwheel and pitchbend control ports to
      // MIDI out
      add_events_from_midi_cc_control_ports (
        processing_caches_->midi_out_rt_->midi_events_.queued_events_,
        time_nfo.local_offset_);
    }

  /* add inputs to outputs */
  if (is_audio ())
    {
      const auto &stereo_in = std::make_pair (
        processing_caches_->audio_ins_rt_[0],
        processing_caches_->audio_ins_rt_[1]);
      const auto &stereo_out = std::make_pair (
        processing_caches_->audio_outs_rt_[0],
        processing_caches_->audio_outs_rt_[1]);
      const auto input_gain = [this] () {
        const auto &input_gain_param = *processing_caches_->input_gain_;
        return input_gain_param.range ().convertFrom0To1 (
          input_gain_param.currentValue ());
      };
      const auto mono = [this] () {
        const auto &mono_param = *processing_caches_->mono_param_;
        return mono_param.range ().is_toggled (mono_param.currentValue ());
      };
      const auto monitor_audio = [this] () {
        const auto &monitor_audio_param = *processing_caches_->monitor_audio_;
        return monitor_audio_param.range ().is_toggled (
          monitor_audio_param.currentValue ());
      };

      // only take into account inputs if track has a "monitor" param (such as
      // audio tracks) which is enabled, or the track does not have a
      // "monitor" param (in which case inputs are always taken into account)
      if (!monitor_audio_id_.has_value () || monitor_audio ())
        {
          utils::float_ranges::product (
            &stereo_out.first->buf_[time_nfo.local_offset_],
            &stereo_in.first->buf_[time_nfo.local_offset_],
            input_gain_id_ ? input_gain () : 1.f, time_nfo.nframes_);

          const auto &src_right_buf =
            (mono_id_ && mono ()) ? stereo_in.first->buf_ : stereo_in.second->buf_;
          utils::float_ranges::product (
            &stereo_out.second->buf_[time_nfo.local_offset_],
            &src_right_buf[time_nfo.local_offset_],
            input_gain_id_ ? input_gain () : 1.f, time_nfo.nframes_);
        }
    }
  else if (is_midi ())
    {
      // apply any transformations
      if (transform_midi_inputs_func_.has_value ())
        {
          std::invoke (
            *transform_midi_inputs_func_,
            processing_caches_->midi_in_rt_->midi_events_.active_events_);
        }

        /* process midi bindings */
        // TODO
#if 0
      if (cc_mappings_ && transport_.recording_enabled ())
        {
          cc_mappings_->apply_from_cc_events (
            midi_in_rt_->midi_events_.active_events_);
        }
#endif

      // append data from MIDI input -> MIDI output
      append_midi_inputs_to_outputs_func_ (
        processing_caches_->midi_out_rt_->midi_events_.queued_events_,
        processing_caches_->midi_in_rt_->midi_events_.active_events_, time_nfo);
    }

  if (
    !transport_.has_recording_preroll_frames_remaining ()
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
      const auto &output_gain_param = *processing_caches_->output_gain_;
      const auto  output_gain = output_gain_param.range ().convertFrom0To1 (
        output_gain_param.currentValue ());

      utils::float_ranges::mul_k2 (
        &processing_caches_->audio_outs_rt_[0]->buf_[time_nfo.local_offset_],
        output_gain, time_nfo.nframes_);
      utils::float_ranges::mul_k2 (
        &processing_caches_->audio_outs_rt_[1]->buf_[time_nfo.local_offset_],
        output_gain, time_nfo.nframes_);
    }
}

void
TrackProcessor::custom_prepare_for_processing (
  sample_rate_t sample_rate,
  nframes_t     max_block_length)
{
  processing_caches_ = std::make_unique<TrackProcessorProcessingCaches> ();

  if (is_audio ())
    {
      const auto stereo_in = get_stereo_in_ports ();
      processing_caches_->audio_ins_rt_.push_back (&stereo_in.first);
      processing_caches_->audio_ins_rt_.push_back (&stereo_in.second);
      const auto stereo_out = get_stereo_out_ports ();
      processing_caches_->audio_outs_rt_.push_back (&stereo_out.first);
      processing_caches_->audio_outs_rt_.push_back (&stereo_out.second);
    }
  else if (is_midi ())
    {
      processing_caches_->midi_in_rt_ = &get_midi_in_port ();
      processing_caches_->midi_out_rt_ = &get_midi_out_port ();
    }

  if (has_piano_roll_port_)
    {
      processing_caches_->piano_roll_rt_ = &get_piano_roll_port ();
    }

  if (mono_id_.has_value ())
    {
      processing_caches_->mono_param_ = &get_mono_param ();
    }
  if (input_gain_id_.has_value ())
    {
      processing_caches_->input_gain_ = &get_input_gain_param ();
    }
  if (output_gain_id_.has_value ())
    {
      processing_caches_->output_gain_ = &get_output_gain_param ();
    }
  if (monitor_audio_id_.has_value ())
    {
      processing_caches_->monitor_audio_ = &get_monitor_audio_param ();
    }
}

void
TrackProcessor::custom_release_resources ()
{
  processing_caches_.reset ();
}

// ============================================================================

void
from_json (const nlohmann::json &j, TrackProcessor &tp)
{
  from_json (j, static_cast<dsp::ProcessorBase &> (tp));

  // generate MIDI CC caches and set up mappings
  tp.set_param_id_caches ();
  // tp.set_midi_mappings ();
}
}
