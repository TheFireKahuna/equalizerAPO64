/*
	This file is part of EqualizerAPO, a system-wide equalizer.
	Copyright (C) 2024  Jonas Dahlinger

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "stdafx.h"
#include "UpdateChecker.h"

UpdateChecker::UpdateChecker(QWidget* parent, const QJsonDocument& doc)
	: QDialog(parent)
{
	ui.setupUi(this);

	QJsonObject docObj = doc.object();
	downloadUrl = docObj.value("download-url").toString();

	QJsonArray versionsArray = docObj.value("versions").toArray();
	QString html = "<style>\n.date{font-size:small;font-style:italic;color:gray;}\nul{margin:5px;}\nli{margin:2px;}\n</style>\n";
	bool first = true;
	for (QJsonValue versionValue : versionsArray)
	{
		QJsonObject versionObj = versionValue.toObject();
		QString version = versionObj.value("version").toString();
		if (first)
		{
			newestVersion = version;
			first = false;
		}
		QDate date = QDate::fromString(versionObj.value("date").toString(), Qt::ISODate);
		html.append(QString("<div><b>%0 </b><span class=\"date\">(%1)</span></div>").arg(version).arg(QLocale().toString(date, QLocale::ShortFormat)));
		html.append("<ul>");
		QJsonArray infoArray = versionObj.value("info").toArray();
		for (QJsonValue v : infoArray)
			html.append("<li>" + v.toString() + "</li>");
		html.append("</ul>");
	}

	ui.versionBrowser->setHtml(html);
	ui.versionBrowser->document()->adjustSize();

	adjustSize();

	// workaround for Qt 6 to not initially have scrollbars despite correct dialog size
	ui.versionBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	QTimer::singleShot(0, [&] {ui.versionBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded); });

	connect(ui.goButton, &QPushButton::pressed, this, &UpdateChecker::goToWebsite);
	connect(ui.laterButton, &QPushButton::pressed, this, &UpdateChecker::remindMeLater);
	connect(ui.skipButton, &QPushButton::pressed, this, &UpdateChecker::skipThisVersion);
}

UpdateChecker::~UpdateChecker()
{
}

void UpdateChecker::goToWebsite()
{
	QDesktopServices::openUrl(downloadUrl);

	QSettings settings(QString::fromWCharArray(UPDATE_CHECKER_REGPATH), QSettings::NativeFormat);
	settings.remove("skipVersion");
	accept();
}

void UpdateChecker::remindMeLater()
{
	QSettings settings(QString::fromWCharArray(UPDATE_CHECKER_REGPATH), QSettings::NativeFormat);
	settings.remove("skipVersion");
	reject();
}

void UpdateChecker::skipThisVersion()
{
	QSettings settings(QString::fromWCharArray(UPDATE_CHECKER_REGPATH), QSettings::NativeFormat);
	settings.setValue("skipVersion", newestVersion);
	reject();
}
