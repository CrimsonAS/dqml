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

#include "dqmlfiletracker.h"

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QDateTime>

DQmlFileTracker::DQmlFileTracker(QObject *parent) :
    QObject(parent)
{
    connect(&m_watcher, SIGNAL(directoryChanged(QString)), this, SLOT(onDirChange(QString)));

    m_suffixes << QStringLiteral("qml");
    m_suffixes << QStringLiteral("js");
    m_suffixes << QStringLiteral("png");
    m_suffixes << QStringLiteral("jpg");
    m_suffixes << QStringLiteral("jpeg");
    m_suffixes << QStringLiteral("gif");
}

bool DQmlFileTracker::track(const QString &id, const QString &path)
{
    QFileInfo i(path);
    if (!i.exists()) {
        qCDebug(DQML_LOG) << "path does not exist" << path;
        return false;
    }
    if (!i.isDir()) {
        qCDebug(DQML_LOG) << "path is not a directory" << path;
        return false;
    }
    QString cp = i.canonicalFilePath();
    qCDebug(DQML_LOG) << "tracking" << id << cp;
    m_set[id] = createEntry(i);
    m_watcher.addPath(cp);
    return true;
}

bool DQmlFileTracker::untrack(const QString &id)
{
    qCDebug(DQML_LOG) << "untracking" << id;
    if (m_set.contains(id)) {
        QString path = m_set.take(id).path;
        m_watcher.removePath(path);
        return true;
    }
    qCWarning(DQML_LOG) << "unknown id";
    return false;
}

QHash<QString, DQmlFileTracker::Entry> DQmlFileTracker::trackingSet() const
{
    return m_set;
}

QString DQmlFileTracker::idFromPath(const QString &path) const
{
    for (QHash<QString, Entry>::const_iterator it = m_set.constBegin();
         it != m_set.constEnd(); ++it) {
        if (it.value().path == path)
            return it.key();
    }
    return QString();
}

void DQmlFileTracker::onDirChange(const QString &path)
{
    qCDebug(DQML_LOG) << "change in directory" << path;
    QString id = idFromPath(path);
    if (id.isEmpty()) {
        untrack(id);
        qCDebug(DQML_LOG) << " - no entry, cancel tracking...";
        return;
    }

    Entry &entry = m_set[id];
    QDir dir(path);
    QDirIterator iterator(dir);
    QHash<QString, quint64> currentContent;
    while (iterator.hasNext()) {
        iterator.next();
        QFileInfo i = iterator.fileInfo();
        if (i.isFile() && m_suffixes.contains(i.suffix().toLower())) {
            QString name = i.fileName();
            quint64 time = i.lastModified().toMSecsSinceEpoch();
            currentContent[name] = time;
        }
    }

    QSet<QString> allPaths = QSet<QString>::fromList(currentContent.keys()).unite(QSet<QString>::fromList(entry.content.keys()));
    foreach (QString p, allPaths) {
        bool was = entry.content.contains(p);
        bool is = currentContent.contains(p);
        if (was && is) {
            // If file was there before and is still there, check match the last modified
            // timestamp and emit fileChange if it has been modified..
            if (currentContent.value(p) > entry.content.value(p)) {
                qCDebug(DQML_LOG) << " - changed:" << id << p;
                emit fileChanged(id, path, p);
            }
        } else if (is) {
            // File is there now, but wasn't before -> added..
            qCDebug(DQML_LOG) << " - added:" << id << p;
            emit fileAdded(id, path, p);
        } else if (was) {
            // file was there, but isn't anymore -> removed
            qCDebug(DQML_LOG) << " - removed:" << id << p;
            emit fileRemoved(id, path, p);
        }
    }

    // use the new content set from now on..
    entry.content = currentContent;
}

DQmlFileTracker::Entry DQmlFileTracker::createEntry(const QFileInfo &info)
{
    Entry e;
    e.path = info.canonicalFilePath();
    Q_ASSERT(info.isDir());
    QDir dir(e.path);
    QDirIterator iterator(dir);
    while (iterator.hasNext()) {
        iterator.next();
        QFileInfo i = iterator.fileInfo();
        if (i.isFile() && m_suffixes.contains(i.suffix().toLower())) {
            QString name = i.fileName();
            quint64 time = i.lastModified().toMSecsSinceEpoch();
            qCDebug(DQML_LOG) << " - tracking file" << name << time;
            e.content[name] = time;
        } else {
            qCDebug(DQML_LOG) << " - ignoring" << i.fileName();
        }
    }
    return e;
}
