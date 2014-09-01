/*
    Copyright (c) 2014, Gunnar Sletta
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "dqmlmonitor.h"
#include "dqmlfiletracker.h"

#include <QHostAddress>
#include <QTcpSocket>

DQmlMonitor::DQmlMonitor()
    : m_socket(0)
    , m_port(0)
    , m_connected(false)
    , m_connectTimer(0)
    , m_syncAll(false)
{
    m_tracker = new DQmlFileTracker(this);
    connect(m_tracker, SIGNAL(fileAdded(QString,QString,QString)), this, SLOT(fileWasAdded(QString,QString,QString)));
    connect(m_tracker, SIGNAL(fileRemoved(QString,QString,QString)), this, SLOT(fileWasRemoved(QString,QString,QString)));
    connect(m_tracker, SIGNAL(fileChanged(QString,QString,QString)), this, SLOT(fileWasChanged(QString,QString,QString)));
}

DQmlMonitor::~DQmlMonitor()
{
    if (m_socket) {
        m_socket->close();
        delete m_socket;
    }
}

void DQmlMonitor::connectToServer(const QString &host, quint16 port)
{
    if (m_socket)
        delete m_socket;

    m_host = host;
    m_port = port;

    qCDebug(DQML_LOG) << "Connecting to: " << host << ":" << port;
    m_socket = new QTcpSocket();

    connect(m_socket, SIGNAL(connected()), this, SLOT(socketConnected()));
    connect(m_socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));

    m_connected = false;
    m_socket->connectToHost(QHostAddress(host), port, QTcpSocket::WriteOnly);
}

QByteArray fileContent(const QString &path)
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "failed to read file" << path;
        return QByteArray();
    }
    return file.readAll();
}

void DQmlMonitor::writeEvent(EventType type, const QString &id, const QString &path, const QString &file)
{
    // If we're not supposed to be connected, don't try to write..
    if (!m_socket)
        return;

    if (m_connected) {
        {
            QDataStream stream(m_socket);
            stream << type << id << file;
            if (type != RemoveEvent) {
                QByteArray content = fileContent(path + "/" + file);
                stream << content.size();
                stream.writeRawData(content.constData(), content.size());
            }
        }
        m_socket->flush();

        qCDebug(DQML_LOG) << " -> event written to server";

    } else {
        qCDebug(DQML_LOG) << "monitored a change while disconnected, will be ignored..." << type << id << path << file;
    }
}

void DQmlMonitor::fileWasChanged(const QString &id, const QString &path, const QString &file)
{
    writeEvent(ChangeEvent, id, path, file);
}

void DQmlMonitor::fileWasAdded(const QString &id, const QString &path, const QString &file)
{
    writeEvent(AddEvent, id, path, file);
}

void DQmlMonitor::fileWasRemoved(const QString &id, const QString &path, const QString &file)
{
    writeEvent(RemoveEvent, id, path, file);
}


void DQmlMonitor::socketConnected()
{
    qCDebug(DQML_LOG) << "connected!";
    m_connected = true;
    if (m_connectTimer != 0) {
        killTimer(m_connectTimer);
        m_connectTimer = 0;
    }
    if (m_syncAll)
        syncAllFiles();
}

void DQmlMonitor::syncAllFiles()
{
    QHash<QString, DQmlFileTracker::Entry> all = m_tracker->trackingSet();
    for (QHash<QString, DQmlFileTracker::Entry>::const_iterator it = all.constBegin();
         it != m_tracker->trackingSet().constEnd(); ++it) {
        const DQmlFileTracker::Entry &e = it.value();
        foreach (const QString &file, e.content.keys())
            fileWasAdded(it.key(), it.value().path, file);
    }
}

void DQmlMonitor::socketDisconnected()
{
    qCDebug(DQML_LOG) << "disconnected...";
    maybeNoSocketSoTryLater();
}

void DQmlMonitor::socketError(QAbstractSocket::SocketError error)
{
    qCDebug(DQML_LOG) << "connection error, code:" << hex << error;
    maybeNoSocketSoTryLater();
}

void DQmlMonitor::maybeNoSocketSoTryLater()
{
    qCDebug(DQML_LOG) << " - socket is in state" << m_socket->state();
    if (m_socket->state() == QAbstractSocket::UnconnectedState
            || m_socket->state() == QAbstractSocket::ClosingState) {
        m_connected = false;
        if (m_connectTimer == 0) {
            qCDebug(DQML_LOG) << " -> starting reconnect timer..";
            m_connectTimer = startTimer(10000);
        }
    }
}

void DQmlMonitor::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == m_connectTimer)
        connectToServer(m_host, m_port);
}

