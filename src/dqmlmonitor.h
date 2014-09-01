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

#ifndef DQMLMONITOR_H
#define DQMLMONITOR_H

#include <QObject>
#include <QAbstractSocket>

class DQmlFileTracker;

class QTcpSocket;


class DQmlMonitor: public QObject
{
    Q_OBJECT
public:
    DQmlMonitor();
    ~DQmlMonitor();

    DQmlFileTracker *fileTracker() { return m_tracker; }

public slots:
    void connectToServer(const QString &host, quint16 port);

private slots:
    void socketConnected();
    void socketDisconnected();
    void socketError(QAbstractSocket::SocketError error);

    void fileWasChanged(const QString &id, const QString &path, const QString &file);
    void fileWasAdded(const QString &id, const QString &path, const QString &file);
    void fileWasRemoved(const QString &id, const QString &path, const QString &file);

protected:
    void timerEvent(QTimerEvent *e);

private:
    enum EventType { ChangeEvent = 1, AddEvent = 2, RemoveEvent = 3 };
    void writeEvent(EventType type, const QString &id, const QString &path, const QString &file);
    void maybeNoSocketSoTryLater();

    DQmlFileTracker *m_tracker;

    QTcpSocket *m_socket;
    QString m_host;
    quint16 m_port;
    bool m_connected;
    int m_connectTimer;

};


#endif // DQMLMONITOR_H
