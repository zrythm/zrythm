// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/format_qt.h"

#include "actions/track_operator.h"
#include "utils/logger.h"
#include "utils/utf8_string.h"

#include "operations/operation_dispatcher.h"
#include "operations/track_operations.h"

namespace zrythm::actions
{
void
TrackOperator::rename (const QString &newName)
{
  if (track_ == nullptr || operation_dispatcher_ == nullptr)
    {
      z_warning ("Track rename needs a track and an operation dispatcher");
      return;
    }
  const auto report = operation_dispatcher_->invoke (
    operations::track_set_name_operation (), track_,
    utils::Utf8String::from_qstring (newName));
  if (!report.applied ())
    {
      z_warning ("Track rename refused: {}", report.message);
    }
}

void
TrackOperator::setColor (const QColor &color)
{
  if (track_ == nullptr || operation_dispatcher_ == nullptr)
    {
      z_warning ("Track color changes need a track and an operation dispatcher");
      return;
    }
  if (!color.isValid ())
    {
      z_warning ("cannot set an invalid track color");
      return;
    }
  const auto report = operation_dispatcher_->invoke (
    operations::track_set_color_operation (), track_, utils::Color (color));
  if (!report.applied ())
    {
      z_warning ("Track color change refused: {}", report.message);
    }
}

void
TrackOperator::unsetColor ()
{
  if (track_ == nullptr || operation_dispatcher_ == nullptr)
    {
      z_warning ("Track color changes need a track and an operation dispatcher");
      return;
    }
  const auto report = operation_dispatcher_->invoke (
    operations::track_unset_color_operation (), track_);
  if (!report.applied ())
    {
      z_warning ("Track color reset refused: {}", report.message);
    }
}

void
TrackOperator::setComment (const QString &comment)
{
  if (track_ == nullptr || operation_dispatcher_ == nullptr)
    {
      z_warning (
        "Track comment changes need a track and an operation dispatcher");
      return;
    }
  const auto report = operation_dispatcher_->invoke (
    operations::track_set_comment_operation (), track_,
    utils::Utf8String::from_qstring (comment));
  if (!report.applied ())
    {
      z_warning ("Track comment change refused: {}", report.message);
    }
}

void
TrackOperator::setOutputTrack (structure::tracks::Track * destination)
{
  if (track_ == nullptr || operation_dispatcher_ == nullptr)
    {
      z_warning (
        "Track routing changes need a track and an operation dispatcher");
      return;
    }
  if (destination != nullptr)
    {
      const auto report = operation_dispatcher_->invoke (
        operations::track_route_operation (), track_, destination);
      if (!report.applied ())
        {
          z_warning ("Track routing refused: {}", report.message);
        }
      return;
    }
  const auto report = operation_dispatcher_->invoke (
    operations::track_unroute_operation (), track_);
  if (!report.applied ())
    {
      z_warning ("Track unrouting refused: {}", report.message);
    }
}

void
TrackOperator::setTimebase (dsp::Timebase timebase)
{
  if (track_ == nullptr || operation_dispatcher_ == nullptr)
    {
      z_warning (
        "Track timebase changes need a track and an operation dispatcher");
      return;
    }
  const auto report = operation_dispatcher_->invoke (
    operations::track_set_timebase_operation (), track_, timebase);
  if (!report.applied ())
    {
      z_warning ("Track timebase change refused: {}", report.message);
    }
}

void
TrackOperator::addSend (bool isPrefader)
{
  if (track_ == nullptr || operation_dispatcher_ == nullptr)
    {
      z_warning ("Channel sends need a track and an operation dispatcher");
      return;
    }
  const auto report = operation_dispatcher_->invoke (
    operations::track_add_send_operation (), track_, isPrefader);
  if (!report.applied ())
    {
      z_warning ("Adding a channel send refused: {}", report.message);
    }
}

void
TrackOperator::removeSend (structure::tracks::ChannelSend * send)
{
  if (track_ == nullptr || operation_dispatcher_ == nullptr)
    {
      z_warning ("Channel sends need a track and an operation dispatcher");
      return;
    }
  const auto report = operation_dispatcher_->invoke (
    operations::track_remove_send_operation (), track_, send);
  if (!report.applied ())
    {
      z_warning ("Removing a channel send refused: {}", report.message);
    }
}

void
TrackOperator::setSendPrefader (
  structure::tracks::ChannelSend * send,
  bool                             isPrefader)
{
  if (send == nullptr || operation_dispatcher_ == nullptr)
    {
      z_warning ("Channel sends need a send and an operation dispatcher");
      return;
    }
  const auto report = operation_dispatcher_->invoke (
    operations::track_set_send_prefader_operation (), send, isPrefader);
  if (!report.applied ())
    {
      z_warning ("Changing a send's tap point refused: {}", report.message);
    }
}

void
TrackOperator::setSendTargetTrack (
  structure::tracks::ChannelSend * send,
  structure::tracks::Track *       destination)
{
  if (track_ == nullptr || send == nullptr || operation_dispatcher_ == nullptr)
    {
      z_warning ("Send routing needs a track, a send and a dispatcher");
      return;
    }
  if (destination != nullptr)
    {
      const auto report = operation_dispatcher_->invoke (
        operations::track_set_send_target_track_operation (), track_, send,
        destination);
      if (!report.applied ())
        {
          z_warning ("Pointing a send at a track refused: {}", report.message);
        }
      return;
    }
  const auto report = operation_dispatcher_->invoke (
    operations::track_clear_send_target_operation (), track_, send);
  if (!report.applied ())
    {
      z_warning ("Clearing a send destination refused: {}", report.message);
    }
}

void
TrackOperator::setSendTargetPort (
  structure::tracks::ChannelSend * send,
  dsp::AudioPort *                 destination)
{
  if (track_ == nullptr || send == nullptr || operation_dispatcher_ == nullptr)
    {
      z_warning ("Send routing needs a track, a send and a dispatcher");
      return;
    }
  if (destination != nullptr)
    {
      const auto report = operation_dispatcher_->invoke (
        operations::track_set_send_target_port_operation (), track_, send,
        destination);
      if (!report.applied ())
        {
          z_warning ("Pointing a send at a port refused: {}", report.message);
        }
      return;
    }
  const auto report = operation_dispatcher_->invoke (
    operations::track_clear_send_target_operation (), track_, send);
  if (!report.applied ())
    {
      z_warning ("Clearing a send destination refused: {}", report.message);
    }
}
}
