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

#include <QtCore>

#include "filetracker.h"

class MonitorInputParser : public QThread
{
    Q_OBJECT

public:
    void run();
    void printHelp();

signals:
    void tracked();
    void track(const QString &id, const QString &path);
    void untrack(const QString &id);

    void queryServer();
    void server(const QString &ip, int port);
    void connectToServer();

    void quit();
};

QString popString(QString *input)
{
    int cmdEnd = input->indexOf(QLatin1Char(' '));
    if (cmdEnd < 0) {
        return *input;
    } else {
        QString tmp = input->mid(0, cmdEnd);
        *input = input->mid(cmdEnd + 1).trimmed();
        return tmp;
    }
}

void MonitorInputParser::run()
{
    QTextStream stream(stdin);

    QString command;
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();

        // skip comments..
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;

        command = popString(&line);

        if (command == QStringLiteral("tracked")) {
            emit tracked();

        } else if (command == QStringLiteral("track")) {
            QString id = popString(&line);
            if (id.isEmpty() || line.isEmpty()) {
                printf("Malformed 'track' command, try 'help'\n");
                continue;
            }
            track(id, line);

        } else if (command == QStringLiteral("untrack")) {
            if (line.isEmpty()) {
                printf("Malformed 'untrack' command, try 'help'\n");
                continue;
            }
            untrack(line);

        } else if (command == QStringLiteral("help")) {
            printHelp();

        } else if (command == QStringLiteral("quit")) {
            emit quit();
            break;

        } else {
            printf("Unknown command, try 'help'\n");
        }
    }
}

void MonitorInputParser::printHelp()
{
    printf("Usage:\n"
           "\n"
           "command [args]?\n"
           "\n"
           "Accepted commands:\n"
           "\n"
           "    tracked                 Lists the currently tracked directories\n"
           "    track [id] [path]       Begins tracking [path] and gives it the name [id]\n"
           "    untrack [id]            Stops tracking the path named with [id]\n"
           "                            ([id] cannot have spaces\n"
           "\n"
           "    server [ip] [port]      Sets the server\n"
           "    connect                 Connects to the server"
           "\n"
           "    quit                    Quits the application\n"
           "    help                    Prints this help\n"
           "\n");
}


class MonitorManager : public QObject
{
    Q_OBJECT
public:
    MonitorManager();

public slots:
    void track(const QString &id, const QString &path);
    void untrack(const QString &id);
    void tracked();

private:
    MonitorInputParser m_parser;
    FileTracker m_tracker;
};

MonitorManager::MonitorManager()
{
    connect(&m_parser, SIGNAL(quit()), QCoreApplication::instance(), SLOT(quit()));

    connect(&m_parser, SIGNAL(track(QString,QString)), this, SLOT(track(QString,QString)), Qt::BlockingQueuedConnection);
    connect(&m_parser, SIGNAL(untrack(QString)), this, SLOT(untrack(QString)), Qt::BlockingQueuedConnection);
    connect(&m_parser, SIGNAL(tracked()), this, SLOT(tracked()), Qt::BlockingQueuedConnection);

    m_parser.start();
}

void MonitorManager::track(const QString &id, const QString &path)
{
    bool ok = m_tracker.track(id, path);
    printf(" - %s: '%s' -> '%s'\n",
           ok ? "tracking" : "failed to set up tracking",
           qPrintable(id),
           qPrintable(path));
}

void MonitorManager::untrack(const QString &id)
{
    bool ok = m_tracker.untrack(id);
    printf(" - %s: '%s'\n",
           ok ? "stopped tracking" : "failed to stop tracking",
           qPrintable(id));
}

void MonitorManager::tracked()
{
    QHash<QString, FileTracker::Entry> set = m_tracker.trackingSet();
    printf("Currently tracking\n"
           "    Id                             Path\n");
    for (QHash<QString, FileTracker::Entry>::const_iterator it = set.constBegin();
         it != set.constEnd(); ++it) {
        printf("    %-30s %s\n", qPrintable(it.key()), qPrintable(it.value().path));
    }
}


int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    MonitorManager manager;

    return app.exec();
}

#include "monitormain.moc"
