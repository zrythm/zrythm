// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QByteArray>
#include <QDataStream>

constexpr const char * IPC_SOCKET_NAME = "ZrythmEngineSocket";

enum class MessageType
{
  SetVolume,
  Play,
  Stop,
  GetStatus,
  StatusUpdate
};

struct IPCMessage
{
  MessageType type_;
  QByteArray  data_;
};

inline QDataStream &
operator<< (QDataStream &out, const IPCMessage &message)
{
  out << static_cast<qint32> (message.type_) << message.data_;
  return out;
}

inline QDataStream &
operator>> (QDataStream &in, IPCMessage &message)
{
  qint32 type = 0;
  in >> type >> message.data_;
  message.type_ = static_cast<MessageType> (type);
  return in;
}