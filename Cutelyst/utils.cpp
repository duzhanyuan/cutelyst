/*
 * Copyright (C) 2015-2018 Daniel Nicoletti <dantti12@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "utils.h"

#include <QTextStream>
#include <QVector>

using namespace Cutelyst;

QByteArray buildTableDivision(const QVector<int> &columnsSize)
{
    QByteArray buffer;
    QTextStream out(&buffer, QIODevice::WriteOnly);
    for (int i = 0; i < columnsSize.size(); ++i) {
        if (i) {
            out << '+';
        } else {
            out << '.';
        }
        out << QByteArray().fill('-', columnsSize[i] + 2).data();
    }
    out << '.';

    return buffer;
}

QByteArray Utils::buildTable(const QVector<QStringList> &table, const QStringList &headers, const QString &title)
{
    QByteArray buffer;
    QVector<int> columnsSize;

    if (!headers.isEmpty()) {
        for (const QString &header : headers) {
            columnsSize.push_back(header.size());
        }
    } else {
        for (const QStringList &rows : table) {
            if (columnsSize.empty()) {
                for (const QString &row : rows) {
                    columnsSize.push_back(row.size());
                }
            } else if (rows.size() != columnsSize.size()) {
                qFatal("Incomplete table");
            }
        }
    }

    for (const QStringList &row : table) {
        if (row.size() > columnsSize.size()) {
            qFatal("Incomplete table");
            break;
        }

        for (int i = 0; i < row.size(); ++i) {
            columnsSize[i] = qMax(columnsSize[i], row[i].size());
        }
    }

    // printing
    QTextStream out(&buffer, QIODevice::WriteOnly);
    out.setFieldAlignment(QTextStream::AlignLeft);
    QByteArray div = buildTableDivision(columnsSize);

    if (!title.isEmpty()) {
        out << title << endl;
    }

    // Top line
    out << div << endl;

    if (!headers.isEmpty()) {
        // header titles
        for (int i = 0; i < headers.size(); ++i) {
            out << "| ";

            out.setFieldWidth(columnsSize[i]);
            out << headers[i];

            out.setFieldWidth(0);
            out << ' ';
        }
        out << '|' << endl;

        // header bottom line
        out << div << endl;
    }

    for (const QStringList &row : table) {
        // content table
        for (int i = 0; i < row.size(); ++i) {
            out << "| ";

            out.setFieldWidth(columnsSize[i]);
            out << row[i];

            out.setFieldWidth(0);
            out << ' ';
        }
        out << '|' << endl;
    }

    // table bottom line
    out << div;

    return buffer;
}

QString Utils::decodePercentEncoding(QString *s)
{
    if (s->isEmpty()) {
        return *s;
    }

    QByteArray ba = s->toLatin1();

    char *data = ba.data();
    const char *inputPtr = data;

    const int len = ba.count();
    bool skipUtf8 = true;
    int outlen = 0;
    for (int i = 0 ; i < len; ++i, ++outlen) {
        const char c = inputPtr[i];
        if (c == '%' && i + 2 < len) {
            int a = inputPtr[++i];
            int b = inputPtr[++i];

            if (a >= '0' && a <= '9') a -= '0';
            else if (a >= 'a' && a <= 'f') a = a - 'a' + 10;
            else if (a >= 'A' && a <= 'F') a = a - 'A' + 10;

            if (b >= '0' && b <= '9') b -= '0';
            else if (b >= 'a' && b <= 'f') b  = b - 'a' + 10;
            else if (b >= 'A' && b <= 'F') b  = b - 'A' + 10;

            *data++ = (char)((a << 4) | b);
            skipUtf8 = false;
        } else if (c == '+') {
            *data++ = ' ';
        } else {
            *data++ = c;
        }
    }

    if (skipUtf8) {
        return *s;
    }

    return QString::fromUtf8(ba.data(), outlen);
}

QString Utils::decodePercentEncoding(QByteArray *ba)
{
    if (ba->isEmpty())
        return QString();

    char *data = ba->data();
    const char *inputPtr = data;

    int len = ba->count();
    bool skipUtf8 = true;
    int outlen = 0;
    for (int i = 0; i < len; ++i, ++outlen) {
        const char c = inputPtr[i];
        if (c == '%' && i + 2 < len) {
            int a = inputPtr[++i];
            int b = inputPtr[++i];

            if (a >= '0' && a <= '9') a -= '0';
            else if (a >= 'a' && a <= 'f') a = a - 'a' + 10;
            else if (a >= 'A' && a <= 'F') a = a - 'A' + 10;

            if (b >= '0' && b <= '9') b -= '0';
            else if (b >= 'a' && b <= 'f') b  = b - 'a' + 10;
            else if (b >= 'A' && b <= 'F') b  = b - 'A' + 10;

            *data++ = (char)((a << 4) | b);
            skipUtf8 = false;
        } else if (c == '+') {
            *data++ = ' ';
        } else {
            *data++ = c;
        }
    }

    if (skipUtf8) {
        return QString::fromLatin1(ba->data(), outlen);
    } else {
        return QString::fromUtf8(ba->data(), outlen);
    }
}
