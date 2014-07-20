/*
 * Copyright (C) 2013 Daniel Nicoletti <dantti12@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "dispatchtypepath.h"

#include "common.h"
#include "controller.h"

#include <QStringBuilder>
#include <QBuffer>
#include <QDebug>

using namespace Cutelyst;

DispatchTypePath::DispatchTypePath(QObject *parent) :
    DispatchType(parent)
{
}

void DispatchTypePath::list() const
{
    QString buffer;
    QTextStream out(&buffer, QIODevice::WriteOnly);

    out << "Loaded Path actions:" << endl;
    QString pathTitle("Path");
    QString privateTitle("Private");
    int pathLength = pathTitle.length();
    int privateLength = privateTitle.length();

    QStringList keys = m_paths.keys();
    keys.sort();
    Q_FOREACH (const QString &path, keys) {
        Q_FOREACH (Action *action, m_paths.value(path)) {
            QString _path = QLatin1Char('/') % path;
            QString args = action->attributes().value("Args");
            if (args.isEmpty()) {
                _path.append(QLatin1String("/..."));
            } else {
                for (int i = 0; i < action->numberOfArgs(); ++i) {
                    _path.append(QLatin1String("/*"));
                }
            }
            _path.replace(m_multipleSlashes, QLatin1String("/"));
            pathLength = qMax(pathLength, _path.length() + 1);

            QString privateName = action->privateName();
            if (!privateName.startsWith(QLatin1String("/"))) {
                privateName.prepend(QLatin1String("/"));
            }
            privateLength = qMax(privateLength, privateName.length());
        }
    }

    out << "." << QString().fill(QLatin1Char('-'), pathLength).toUtf8().data()
        << "+" << QString().fill(QLatin1Char('-'), privateLength).toUtf8().data()
        << "." << endl;
    out << "|" << pathTitle.leftJustified(pathLength).toUtf8().data()
        << "|" << privateTitle.leftJustified(privateLength).toUtf8().data()
        << "|" << endl;
    out << "." << QString().fill(QLatin1Char('-'), pathLength).toUtf8().data()
        << "+" << QString().fill(QLatin1Char('-'), privateLength).toUtf8().data()
        << "." << endl;

    Q_FOREACH (const QString &path, keys) {
        Q_FOREACH (Action *action, m_paths.value(path)) {
            QString _path = QLatin1Char('/') % path;
            if (!action->attributes().contains("Args")) {
                _path.append(QLatin1String("/..."));
            } else {
                for (int i = 0; i < action->numberOfArgs(); ++i) {
                    _path.append(QLatin1String("/*"));
                }
            }
            _path.replace(m_multipleSlashes, QLatin1String("/"));

            QString privateName = action->privateName();
            if (!privateName.startsWith(QLatin1String("/"))) {
                privateName.prepend(QLatin1String("/"));
            }

            out << "|" << _path.leftJustified(pathLength).toUtf8().data()
                << "|" << privateName.leftJustified(privateLength).toUtf8().data()
                << "|" << endl;
        }
    }

    out << "." << QString().fill(QLatin1Char('-'), pathLength).toUtf8().data()
        << "+" << QString().fill(QLatin1Char('-'), privateLength).toUtf8().data()
        << ".";

    qCDebug(CUTELYST_DISPATCHER) << buffer.toUtf8().data();
}

bool DispatchTypePath::match(Context *ctx, const QString &path) const
{
    QString _path = path;
    if (_path.isEmpty()) {
        _path = QLatin1Char('/');
    }

    const ActionList &actions = m_paths.value(_path);
    Q_FOREACH (Action *action, actions) {
        if (action->match(ctx)) {
            setupMatchedAction(ctx, action, _path);
            return true;
        }
    }
    return false;
}

bool DispatchTypePath::registerAction(Action *action)
{
    bool ret = false;
    QMultiHash<QByteArray, QByteArray> attributes = action->attributes();
    QMultiHash<QByteArray, QByteArray>::iterator i = attributes.find("Path");
    while (i != attributes.end() && i.key() == "Path") {
        if (registerPath(i.value(), action)) {
            ret = true;
        }

        ++i;
    }

    // We always register valid actions
    return ret;
}

QByteArray DispatchTypePath::uriForAction(Action *action, const QStringList &captures) const
{
    QByteArray path = action->attributes().value("Path");
    if (!path.isNull()) {
        if (path.isEmpty()) {
            return QByteArray("/", 1);
        } else {
            return path;
        }
    }
    return QByteArray();
}

bool actionLessThan(Action *a1, Action *a2)
{
    return a1->numberOfArgs() < a2->numberOfArgs();
}

bool DispatchTypePath::registerPath(const QString &path, Action *action)
{
    QString _path = path;
    if (_path.startsWith(QLatin1Char('/'))) {
        _path.remove(0, 1);
    }
    if (_path.isEmpty()) {
        // TODO when we try to match a path
        // it comes without a leading / so
        // when would this be used?
        _path = QLatin1Char('/');
    }

    if (m_paths.contains(_path)) {
        ActionList actions = m_paths.value(_path);
        Q_FOREACH (const Action *regAction, actions) {
            if (regAction->numberOfArgs() == action->numberOfArgs()) {
                qCWarning(CUTELYST_DISPATCHER) << "Not registering Action"
                                               << action->name()
                                               << "of controller"
                                               << action->controller()->objectName()
                                               << "because it conflicts with "
                                               << regAction->name();
                return false;
            }
        }

        actions << action;
        qSort(actions.begin(), actions.end(), actionLessThan);
        m_paths[_path] = actions;
    } else {
        m_paths.insert(_path, ActionList() << action);
    }
    return true;
}
