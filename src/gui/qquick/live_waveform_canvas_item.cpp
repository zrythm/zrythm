// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/port_observation_token.h"
#include "gui/qquick/live_waveform_canvas_item.h"
#include "utils/float_ranges.h"
#include "utils/qt.h"

#include <QTimer>

namespace zrythm::gui::qquick
{

struct LiveWaveformCanvasItem::Impl
{
  QPointer<dsp::AudioPort>              port_;
  QPointer<dsp::PortObservationManager> observation_manager_;
  std::optional<dsp::ObservationToken>  observation_token_;

  size_t buffer_size_ = 2048;

  std::vector<float> left_buffer_;
  std::vector<float> right_buffer_;

  utils::QObjectUniquePtr<QTimer> timer_;
};

LiveWaveformCanvasItem::LiveWaveformCanvasItem (QQuickItem * parent)
    : WaveformCanvasItem (parent), impl_ (std::make_unique<Impl> ())
{
  impl_->left_buffer_.resize (impl_->buffer_size_, 0.f);
  impl_->right_buffer_.resize (impl_->buffer_size_, 0.f);

  impl_->timer_ = utils::make_qobject_unique<QTimer> (this);
  impl_->timer_->setInterval (1000 / 60);
  connect (
    impl_->timer_.get (), &QTimer::timeout, this,
    &LiveWaveformCanvasItem::process_audio);
  impl_->timer_->start ();
}

LiveWaveformCanvasItem::~LiveWaveformCanvasItem () = default;

zrythm::dsp::AudioPort *
LiveWaveformCanvasItem::stereoPort () const
{
  return impl_->port_;
}

void
LiveWaveformCanvasItem::setStereoPort (zrythm::dsp::AudioPort * port)
{
  if (impl_->port_ == port)
    return;

  if (impl_->port_ != nullptr)
    {
      impl_->observation_token_.reset ();
      QObject::disconnect (impl_->port_, &QObject::destroyed, this, nullptr);
      impl_->port_.clear ();
    }

  impl_->port_ = port;
  if (impl_->port_ != nullptr)
    {
      connect (impl_->port_, &QObject::destroyed, this, [this] () {
        impl_->observation_token_.reset ();
        impl_->port_.clear ();
      });
      try_create_token ();
    }

  Q_EMIT stereoPortChanged ();
}

zrythm::dsp::PortObservationManager *
LiveWaveformCanvasItem::portObservationManager () const
{
  return impl_->observation_manager_;
}

void
LiveWaveformCanvasItem::setPortObservationManager (
  zrythm::dsp::PortObservationManager * manager)
{
  impl_->observation_manager_ = manager;
  try_create_token ();
}

void
LiveWaveformCanvasItem::try_create_token ()
{
  if (impl_->port_ == nullptr || impl_->observation_manager_ == nullptr)
    return;
  impl_->observation_token_.emplace (
    *impl_->observation_manager_, *impl_->port_);
}

int
LiveWaveformCanvasItem::bufferSize () const
{
  return static_cast<int> (impl_->buffer_size_);
}

void
LiveWaveformCanvasItem::setBufferSize (int size)
{
  const auto n = static_cast<size_t> (size);
  if (impl_->buffer_size_ == n)
    return;
  impl_->buffer_size_ = n;
  impl_->left_buffer_.resize (n, 0.f);
  impl_->right_buffer_.resize (n, 0.f);
  Q_EMIT bufferSizeChanged ();
}

void
LiveWaveformCanvasItem::process_audio ()
{
  if (!impl_->port_ || !impl_->observation_token_)
    return;

  auto &cache = impl_->observation_token_->cache ();
  if (cache.audio.size () < 2)
    return;

  const auto &ch0 = cache.audio[0];
  const auto &ch1 = cache.audio[1];

  if (ch0.empty () && ch1.empty ())
    return;

  const auto n = impl_->buffer_size_;

  // Append new samples to rolling buffers, keeping the most recent n samples
  auto append_rolling =
    [n] (std::vector<float> &buf, const std::vector<float> &new_data) {
      if (new_data.empty ())
        return;
      const auto avail = std::min (new_data.size (), n);
      // Shift existing data left to make room
      if (buf.size () >= n + avail)
        std::copy (buf.begin () + avail, buf.begin () + n, buf.begin ());
      else if (buf.size () > avail)
        std::copy (buf.begin () + avail, buf.end (), buf.begin ());
      // Append new data at the end
      std::copy_n (new_data.end () - avail, avail, buf.begin () + n - avail);
    };

  append_rolling (impl_->left_buffer_, ch0);
  append_rolling (impl_->right_buffer_, ch1);

  cache.clear_audio ();

  // Write into the base class audio buffer (stereo)
  audio_buffer_.setSize (2, static_cast<int> (n), false, false, true);
  audio_buffer_.copyFrom (
    0, 0, impl_->left_buffer_.data (), static_cast<int> (n));
  audio_buffer_.copyFrom (
    1, 0, impl_->right_buffer_.data (), static_cast<int> (n));

  notifyBufferChanged ();
}

}
