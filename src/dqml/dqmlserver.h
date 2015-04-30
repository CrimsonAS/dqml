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

#ifndef DQMLSERVER_H
#define DQMLSERVER_H

#include <dqml/dqmlglobal.h>

#include <QtCore/QObject>

#include <QtNetwork/QAbstractSocket>

QT_BEGIN_NAMESPACE

class QTcpSocket;
class QTcpServer;
class QQmlEngine;
class QQuickView;

class DQML_EXPORT DQmlServer : public QObject
{
    Q_OBJECT
public:
    DQmlServer(QQmlEngine *engine, QQuickView *view, const QString &file);

    void setCreateViewIfNeeded(bool createView) { m_createViewIfNeeded = createView; }
    bool createsViewIfNeeded() const { return m_createViewIfNeeded; }

    void addTrackerMapping(const QString &id, const QString &path) { m_trackerMapping.insert(id, path); }

public Q_SLOTS:
    void listen(quint16 port);
    void reloadQml();

private Q_SLOTS:
    void newConnection();
    void acceptError(QAbstractSocket::SocketError error);

    void read();

private:
    QString m_file;

    QQmlEngine *m_engine;
    QQuickView *m_view;
    QObject *m_contentItem;

    bool m_createViewIfNeeded;
    bool m_ownsView;
    bool m_pendingReload;

    QTcpServer *m_tcpServer;
    QTcpSocket *m_clientSocket;

    QHash<QString, QString> m_trackerMapping;
};

QT_END_NAMESPACE

#endif // DQMLSERVER_H
