
// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

// #include "gui/dsp/engine.h"

#include "utils/backtrace.h"
#include "utils/format.h"
#include "utils/logger.h"

#include <QDir>

#include "engine/audio_engine_application.h"

using namespace Qt::StringLiterals;

namespace zrythm::engine
{

// JUCE_CREATE_APPLICATION_DEFINE (AudioEngineApplication)

AudioEngineApplication::AudioEngineApplication ()
{
  // Create Qt application without running event loop
  int argc = 0;
  qt_app_ = std::make_unique<QCoreApplication> (argc, nullptr);

  qt_app_->setApplicationName (u"ZrythmEngine"_s);
  qt_app_->setApplicationVersion (u"1.0"_s);
  qt_app_->setOrganizationName (u"Zrythm.org"_s);
  qt_app_->setOrganizationDomain (u"zrythm.org"_s);

  utils::Backtrace::init_signal_handlers ();

  /* setup command line parser */
  // TODO

  // Initialize JUCE
  // juce::JUCEApplicationBase::createInstance = &juce_CreateApplication;
  // juce::MessageManager::getInstance ()->setCurrentThreadAsMessageThread ();

  // AudioEngine::set_default_backends (false);

  constexpr const char * copyright_line =
    "Copyright (C) " COPYRIGHT_YEARS " " COPYRIGHT_NAME;
  std::cout
    << "\n==============================================================\n\n"
    << format_str (
         "ZrythmEngine-1.0\n{}\n\n"
         "Zrythm comes with ABSOLUTELY NO WARRANTY!\n\n"
         "This is free software, and you are welcome to redistribute it\n"
         "under certain conditions. See the file 'COPYING' for details.\n\n"
         "Write comments and bugs to {}\n"
         "Support this project at {}\n\n",
         copyright_line, ISSUE_TRACKER_URL, "https://liberapay.com/Zrythm")
    << "==============================================================\n\n";

  z_info ("Running ZrythmEngine in '{}'", QDir::currentPath ());

  // Schedule the post-exec initialization
  // QTimer::singleShot (
  //   0, this, &AudioEngineApplication::post_exec_initialization);

  setup_ipc ();
}

void
AudioEngineApplication::initialise (const juce::String &commandLine)
{
  setup_ipc ();

  // Start JUCE's message loop
  // juce::MessageManager::getInstance ()->runDispatchLoop ();
}

void
AudioEngineApplication::setup_ipc ()
{
#if 0
  server_ = new QLocalServer (this);
  qDebug () << "Listening for IPC connections";
  connect (
    server_, &QLocalServer::newConnection, this,
    &AudioEngineApplication::onNewConnection);

  qDebug () << "Removing old socket";
  QLocalServer::removeServer (QString::fromUtf8 (IPC_SOCKET_NAME));

  qDebug () << "Starting server";
  if (!server_->listen (QString::fromUtf8 (IPC_SOCKET_NAME)))
    {
      qDebug () << "Unable to start the server:" << server_->errorString ();
    }
#endif
}

void
AudioEngineApplication::post_exec_initialization ()
{
  // TODO
}

#if 0
void
AudioEngineApplication::onNewConnection ()
{
  z_info ("New connection");
  client_connection_ = server_->nextPendingConnection ();
  connect (
    client_connection_, &QLocalSocket::readyRead, this,
    &AudioEngineApplication::onDataReceived);
  connect (
    client_connection_, &QLocalSocket::disconnected, client_connection_,
    &QLocalSocket::deleteLater);
}

void
AudioEngineApplication::onDataReceived ()
{
  QDataStream in (client_connection_);
  IPCMessage  message;
  in >> message;

  qDebug () << "Received message:" << message.data_;
}
#endif

AudioEngineApplication::~AudioEngineApplication ()
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
  QLocalServer::removeServer (QString::fromUtf8 (IPC_SOCKET_NAME));
}

} // namespace zrythm::engine
