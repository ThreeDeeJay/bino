/*
 * This file is part of Bino, a 3D video player.
 *
 * Copyright (C) 2024
 * Martin Lambers <marlam@marlam.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>


class UrlLoader : public QObject
{
    Q_OBJECT

private:
    QUrl _url;
    QNetworkAccessManager _netAccMgr;
    QByteArray _data;
    bool _done;

private slots:
    void urlLoaded(QNetworkReply* reply);

public:
    UrlLoader(const QUrl& url);
    virtual ~UrlLoader();

    const QByteArray& load();
};
