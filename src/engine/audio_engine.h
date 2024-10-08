// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <csignal>

#include <QCoreApplication>
#include <QLocalServer>
#include <QLocalSocket>

#include "engine/ipc_message.h"

namespace engine
{
class AudioEngine : public QObject
{
  Q_OBJECT

public:
  AudioEngine (QObject * parent = nullptr) : QObject (parent)
  {
    // Set up signal handling so we can close gracefully
    signal (SIGTERM, AudioEngine::signal_handler);
    signal (SIGSEGV, AudioEngine::signal_handler);

    setup_ipc ();
  }

  ~AudioEngine () override
  {
    if (server_)
      {
        server_->close ();
      }
    if (client_connection_)
      {
        client_connection_->disconnectFromServer ();
        client_connection_->close ();
      }
    QLocalServer::removeServer (IPC_SOCKET_NAME);
  }

private:
  static void signal_handler (int signal)
  {
    const char * signal_name = nullptr;
    switch (signal)
      {
      case SIGTERM:
        signal_name = "SIGTERM";
        break;
      case SIGSEGV:
        signal_name = "SIGSEGV";
        break;
      // Add more cases for other signals as needed
      default:
        signal_name = "Unknown signal";
      }
    qDebug () << "Received signal" << signal << ": " << signal_name;
    qApp->quit ();
    exit (EXIT_FAILURE);
  }

  void setup_ipc ()
  {
    server_ = new QLocalServer (this);
    qDebug () << "Listening for IPC connections";
    connect (
      server_, &QLocalServer::newConnection, this,
      &AudioEngine::onNewConnection);

    qDebug () << "Removing old socket";
    QLocalServer::removeServer (IPC_SOCKET_NAME);

    qDebug () << "Starting server";
    if (!server_->listen (IPC_SOCKET_NAME))
      {
        qDebug () << "Unable to start the server:" << server_->errorString ();
      }
  }

private slots:
  void onNewConnection ()
  {
    qDebug () << "New connection";
    client_connection_ = server_->nextPendingConnection ();
    connect (
      client_connection_, &QLocalSocket::readyRead, this,
      &AudioEngine::onDataReceived);
    connect (
      client_connection_, &QLocalSocket::disconnected, client_connection_,
      &QLocalSocket::deleteLater);
  }

  void onDataReceived ()
  {
    QDataStream in (client_connection_);
    IPCMessage  message;
    in >> message;

    qDebug () << "Received message:" << QString (message.data_);
  }

private:
  QLocalServer * server_ = nullptr;
  QLocalSocket * client_connection_ = nullptr;
};
}