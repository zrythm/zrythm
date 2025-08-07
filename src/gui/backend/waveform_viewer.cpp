// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/waveform_viewer.h"
#include "utils/dsp.h"

WaveformViewerProcessor::WaveformViewerProcessor (QObject * parent)
    : QObject (parent)
{
  setBufferSize (static_cast<int> (DEFAULT_BUFFER_SIZE));
  setDisplayPoints (DEFAULT_DISPLAY_POINTS);

  update_timer_ = new QTimer (this);
  update_timer_->setInterval (1000 / 60); // 60 FPS
  connect (
    update_timer_, &QTimer::timeout, this,
    &WaveformViewerProcessor::process_audio);
  update_timer_->start ();
}

WaveformViewerProcessor::~WaveformViewerProcessor ()
{
  if (update_timer_ != nullptr)
    {
      update_timer_->stop ();
    }
}

void
WaveformViewerProcessor::process_audio ()
{
  if (!left_port_obj_ || !right_port_obj_)
    {
      return;
    }

  auto &left_reader = left_ring_reader_;
  auto &right_reader = right_ring_reader_;

  if (!left_reader || !right_reader)
    {
      return;
    }

  // Read audio data
  size_t frames_read = 0;

  frames_read = left_port_obj_->audio_ring_->peek_multiple (
    left_buffer_.data (), buffer_size_);
  if (frames_read < buffer_size_)
    {
      utils::float_ranges::fill (
        &left_buffer_[frames_read], 0.f, buffer_size_ - frames_read);
    }

  frames_read = right_port_obj_->audio_ring_->peek_multiple (
    right_buffer_.data (), buffer_size_);
  if (frames_read < buffer_size_)
    {
      utils::float_ranges::fill (
        &right_buffer_[frames_read], 0.f, buffer_size_ - frames_read);
    }

  // Create mono mix
  utils::float_ranges::product (
    mono_buffer_.data (), left_buffer_.data (), 0.5f, buffer_size_);
  utils::float_ranges::mix_product (
    mono_buffer_.data (), right_buffer_.data (), 0.5f, buffer_size_);

  // Calculate peak values for display points
  const size_t samples_per_point = buffer_size_ / display_points_;
  for (size_t point = 0; point < display_points_; ++point)
    {
      const size_t start_idx = point * samples_per_point;
      const size_t end_idx =
        std::min (start_idx + samples_per_point, buffer_size_);

      auto ret = std::ranges::minmax (
        std::ranges::subrange (
          std::ranges::next (mono_buffer_.begin (), start_idx),
          std::ranges::next (mono_buffer_.begin (), end_idx)));
      const auto min_val = ret.min;
      const auto max_val = ret.max;

      waveform_data_[static_cast<int> (point)] = (max_val + min_val) * 0.5f;
    }

  Q_EMIT waveformDataChanged ();
}

void
WaveformViewerProcessor::setLeftPort (dsp::AudioPort * port_var)
{
  if (left_port_obj_ == port_var)
    return;

  left_port_obj_ = port_var;
  left_ring_reader_.emplace (*left_port_obj_);

  QObject::connect (left_port_obj_, &QObject::destroyed, this, [this] () {
    left_ring_reader_.reset ();
    left_port_obj_.clear ();
  });

  Q_EMIT leftPortChanged ();
}

void
WaveformViewerProcessor::setRightPort (dsp::AudioPort * port_var)
{
  if (right_port_obj_ == port_var)
    return;

  right_port_obj_ = port_var;
  right_ring_reader_.emplace (*right_port_obj_);

  QObject::connect (right_port_obj_, &QObject::destroyed, this, [this] () {
    right_ring_reader_.reset ();
    right_port_obj_.clear ();
  });

  Q_EMIT rightPortChanged ();
}

void
WaveformViewerProcessor::setBufferSize (int size)
{
  const auto size_unsigned = static_cast<size_t> (size);
  if (buffer_size_ != size_unsigned)
    {
      buffer_size_ = size_unsigned;
      mono_buffer_.resize (size);
      left_buffer_.resize (size);
      right_buffer_.resize (size);
      Q_EMIT bufferSizeChanged ();
    }
}

void
WaveformViewerProcessor::setDisplayPoints (int points)
{
  const auto points_unsigned = static_cast<size_t> (points);
  if (display_points_ != points_unsigned)
    {
      display_points_ = points_unsigned;
      waveform_data_.resize (points);
      Q_EMIT displayPointsChanged ();
    }
}
