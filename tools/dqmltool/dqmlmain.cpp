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

#include <QGuiApplication>

#include <QQmlEngine>
#include <QQmlComponent>

#include <QQuickItem>
#include <QQuickView>

#include <QTcpServer>
#include <QTcpSocket>

#include <QFileInfo>
#include <QFileSystemWatcher>

#include <dqml/dqmlserver.h>
#include <dqml/dqmllocalserver.h>
#include <dqml/dqmlmonitor.h>
#include <dqml/dqmlfiletracker.h>

void printHelp()
{
    printf("Usage: \n"
           " > dqml file.qml               (same as --local)\n"
           " > dqml --local [--track path] file.qml\n"
           " > dqml --server port [--track id path] file.qml\n"
           " > dqml --monitor addr port [--track id path] [--sync]\n"
           "\n"
           "Application modes:\n"
           "    --local     The application runs locally and functions like qmlscene, except\n"
           "                that it will monitor the directory where 'file.qml' is located\n"
           "                and all changes in this directory will result in the QML being\n"
           "                re-evaluated. \n"
           "\n"
           "    --monitor   The application runs as a non-gui application, monitoring requested\n"
           "                files. The --monitor mode is followed by the address and port to the\n"
           "                server.\n"
           "\n"
           "    --server    The application runs in server mode with 'file.qml' as the main qml\n"
           "                file. The --server mode is followed by the port to accept connections\n"
           "                on.\n"
           "\n"
           "Options:\n"
           "    --track id path     The application will track the given path and name it 'id'.\n"
           "                        In server/monitor mode the path is used to map paths between\n"
           "                        monitor and server. The tracking is not recursive, but\n"
           "                        multiple --track arguments can be specified. When no\n"
           "                        arguments are specified, the current directory is tracked\n"
           "    --sync              Sync all files from the monitor to the server when connected.\n"
           "                        Useful to keep files in sync."
           "\n"
           );
}



int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    enum Mode {
        Monitor_Mode,
        Server_Mode,
        Local_Mode,
    } mode = Local_Mode;

    QList<QPair<QString,QString> > tracking;
    QString file;
    int port = -1;
    QString host;
    bool sync = false;

    QStringList args = app.arguments();
    for (int i=1; i<args.size(); ++i) {
        const QString &a = args.at(i);

        if (a == QStringLiteral("--monitor")) {
            mode = Monitor_Mode;
            if (args.size() < i + 2) {
                qDebug() << "Malformed --monitor command: requires host and port";
                return 1;
            }
            host = args.at(i+1);
            if (host.startsWith(QStringLiteral("--"))) {
                qDebug() << "Malformed --monitor command: invalid host" << host;
                return 1;
            }
            bool ok;
            port = args.at(i+2).toUShort(&ok);
            if (!ok) {
                qDebug() << "Malformed --monitor command: bad port number";
                return 1;
            }
            i += 2;

        } else if (a == QStringLiteral("--server")) {
            mode = Server_Mode;
            if (args.size() < i + 1) {
                qDebug() << "Malformed --monitor command: requires port";
                return 1;
            }
            bool ok;
            port = args.at(i+1).toUShort(&ok);
            if (!ok) {
                qDebug() << "Malformed --monitor command: bad port number";
                return 1;
            }
            i += 1;

        } else if (a == QStringLiteral("--local")) {
            mode = Local_Mode;

        } else if (a == QStringLiteral("--sync")) {
            sync = true;

        } else if (a == QStringLiteral("--track")) {
            if (mode == Local_Mode) {
                if (args.size() < i + 1) {
                    qDebug() << "Malformed --track command: requires a 'path' when in 'local' mode";
                    return 1;
                }
                // Just given them all a unique id, it isn't so it doesn't matter...
                tracking << QPair<QString, QString>(QString::number(tracking.size()),
                                                    args.at(i+1));
                i += 1;
            } else {
                if (args.size() < i + 2) {
                    qDebug() << "Malformed --track command: requires an 'id' and a 'path'";
                    return 1;
                }
                // Just given them all a unique id, it isn't so it doesn't matter...
                tracking << QPair<QString, QString>(args.at(i+1), args.at(i+2));
                i += 2;
            }

        } else if (a == QStringLiteral("-h") || a == QStringLiteral("--help")) {
            printHelp();
            return 0;

        } else if (i == args.size() - 1) {
            file = a;
        }
    }

    QScopedPointer<DQmlServer> server;
    QScopedPointer<DQmlMonitor> monitor;
    QScopedPointer<DQmlLocalServer> localServer;
    QScopedPointer<QQmlEngine> engine;
    DQmlFileTracker *tracker = 0;

    if (mode == Local_Mode) {
        if (file.isEmpty()) {
            printHelp();
            return 1;
        }

        QGuiApplication::setQuitOnLastWindowClosed(false);

        qDebug() << "running in local mode with" << file;
        engine.reset(new QQmlEngine());
        localServer.reset(new DQmlLocalServer(engine.data(), 0, file));
        localServer->setCreateViewIfNeeded(true);
        localServer->reloadQml();
        tracker = localServer->fileTracker();

    } else if (mode == Monitor_Mode) {
        qDebug() << "running monitor mode";
        monitor.reset(new DQmlMonitor());
        tracker = monitor->fileTracker();
        monitor->setSyncAllFilesWhenConnected(sync);
        monitor->connectToServer(host, port);

    } else if (mode == Server_Mode) {
        qDebug() << "running server mode with" << file;
        engine.reset(new QQmlEngine());
        server.reset(new DQmlServer(engine.data(), 0, file));
        server->setCreateViewIfNeeded(true);
        server->reloadQml();
        server->listen(port);
    }

    QString current = QStringLiteral(".");
    if (mode == Local_Mode || mode == Server_Mode)
        current = QFileInfo(file).canonicalPath();

    if (mode == Local_Mode || mode == Monitor_Mode) {
        Q_ASSERT(tracker);
        if (tracking.size() == 0) {
            tracker->track(QStringLiteral("current-directory"), current);
        } else {
            for (int i=0; i<tracking.size(); ++i) {
                const QPair<QString,QString> &pair = tracking.at(i);
                tracker->track(pair.first, pair.second);
            }
        }

    } else { // Server_Mode
        if (tracking.size() == 0) {
            server->addTrackerMapping(QStringLiteral("current-directory"), current);
        } else {
            for (int i=0; i<tracking.size(); ++i) {
                const QPair<QString,QString> &pair = tracking.at(i);
                server->addTrackerMapping(pair.first, pair.second);
            }
        }
    }

    return app.exec();
}
