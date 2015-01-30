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

#include "dqmlserver.h"

#include <QFile>
#include <QFileInfo>
#include <QTcpServer>
#include <QTcpSocket>

#include <QQmlEngine>
#include <QQmlComponent>

#include <QQuickView>
#include <QQuickItem>

DQmlServer::DQmlServer(QQmlEngine *engine, QQuickView *view, const QString &file)
    : m_file(file)
    , m_engine(engine)
    , m_view(view)
    , m_contentItem(0)
    , m_createViewIfNeeded(false)
    , m_ownsView(false)
    , m_pendingReload(false)
    , m_tcpServer(0)
    , m_clientSocket(0)
{
}

void DQmlServer::listen(quint16 port)
{
    if (m_tcpServer || m_clientSocket) {
        qCDebug(DQML_LOG) << "asked to listen on port" << port << "when already connected...";
        return;
    }
    m_tcpServer = new QTcpServer();
    if (m_tcpServer->listen(QHostAddress::Any, port)) {
        qCDebug(DQML_LOG) << "server listening on port" << port;
    } else {
        qCDebug(DQML_LOG) << "Server failed to listen.." << m_tcpServer->errorString();
    }

    connect(m_tcpServer, SIGNAL(newConnection()), this, SLOT(newConnection()));
    connect(m_tcpServer, SIGNAL(acceptError(QAbstractSocket::SocketError)), this, SLOT(acceptError(QAbstractSocket::SocketError)));
}

void DQmlServer::newConnection()
{
    if (m_clientSocket) {
        qCDebug(DQML_LOG) << " -> already got a connection, ignoring...";
        return;
    }
    m_clientSocket = m_tcpServer->nextPendingConnection();
    qCDebug(DQML_LOG) << "connecting to client" << m_clientSocket->peerAddress();
    connect(m_clientSocket, SIGNAL(readyRead()), this, SLOT(read()));
}

void DQmlServer::acceptError(QAbstractSocket::SocketError error)
{
    qDebug() << "Network error:" << error;
}

void DQmlServer::read()
{
    QDataStream stream(m_clientSocket);
    QString id, file, content;
    int type;

    char *data = 0;
    int dataLength = 0;

    stream >> type >> id >> file;

    if (type == 1 || type == 2) {
        stream >> dataLength;
        data = (char *) malloc(dataLength);

        char *d = data;

        int bytesLeft = dataLength;
        while (bytesLeft > 0) {
            char chunk[1024];
            int actual = stream.readRawData(chunk, qMin<int>(bytesLeft, sizeof(chunk)));
            memcpy(d, chunk, actual);
            bytesLeft -= actual;
            d += actual;
        }
    }

    if (!m_trackerMapping.contains(id)) {
        qCDebug(DQML_LOG) << " -> got data for unknown id, aborting" << id;
        qCDebug(DQML_LOG) << " --->" << m_trackerMapping.keys();
        return;
    }
    QString fileName = m_trackerMapping.value(id) + QStringLiteral("/") + file;

    if (type == 1 || type == 2) {
        QFile f(fileName);
        if (!f.open(QFile::WriteOnly)) {
            qCDebug(DQML_LOG) << " -> failed to write" << QFileInfo(f).absoluteFilePath() << f.errorString();
            return;
        }
        f.write(data, dataLength);
        qCDebug(DQML_LOG) << " -> updated" << id << ":" << file;
    } else if (type == 3) {
        QFile f(fileName);
        bool removed = f.remove();
        if (removed)
            qCDebug(DQML_LOG) << " -> removed" << id << ":" << file;
        else
            qCDebug(DQML_LOG) << " -> failed to remove" << id << ":" << file;
    }

    // More commands in the queue, invoke ourselves again..
    if (!m_clientSocket->atEnd())
        QMetaObject::invokeMethod(this, "read", Qt::QueuedConnection);
    else if (!m_pendingReload) {
        QMetaObject::invokeMethod(this, "reloadQml", Qt::QueuedConnection);
        m_pendingReload = true;
    }
}

void DQmlServer::reloadQml()
{
    m_pendingReload = false;
    qCDebug(DQML_LOG) << "reloading...";
    delete m_contentItem;
    m_contentItem = 0;
    m_engine->clearComponentCache();

    QQmlComponent *component = new QQmlComponent(m_engine);
    QUrl fileUrl = QUrl::fromLocalFile(m_file);
    component->loadUrl(fileUrl);
    qCDebug(DQML_LOG) << "loaded url..";

    if (!component->isReady()) {
        qWarning() << component->errorString();
        return;
    }

    QPoint winPos(-1, -1);
    QSize winSize(-1, -1);

    if (m_view) {
        winPos = m_view->position();
        winSize = m_view->size();
    }

    m_contentItem = component->create();
    qCDebug(DQML_LOG) << "created" << m_contentItem;
    if (qobject_cast<QQuickWindow *>(m_contentItem)) {
        if (m_view && m_ownsView) {
            delete m_view;
            m_view = 0;
            m_ownsView = false;
        }
    } else {
        QQuickItem *item = qobject_cast<QQuickItem *>(m_contentItem);
        if (item && item->width() > 0 && item->height() > 0 && winSize.width() < 0 && winSize.height() < 0)
            winSize = QSize(item->width(), item->height());
        if (!m_view && m_createViewIfNeeded) {
            m_view = new QQuickView();
            m_view->setResizeMode(QQuickView::SizeRootObjectToView);
            m_ownsView = true;
            m_view->show();
            qCDebug(DQML_LOG) << "created a view to hold the QML";
        }
        if (m_view) {
            m_view->setContent(fileUrl, component, m_contentItem);
            if (winPos.x() >= 0 && winPos.y() >= 0)
                m_view->setPosition(winPos);
            if (winSize.width() > 0 && winSize.height() > 0)
                m_view->resize(winSize);
            m_view->show();
        }
        else
            qCDebug(DQML_LOG) << "no view to show qml, set 'setCreatesViewIfNeeded(true)' or supply one.";
    }
}



